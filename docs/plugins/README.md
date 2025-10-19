# Steem Plugins Documentation

Complete reference for all Steem blockchain plugins.

## Overview

Steem uses a plugin architecture to provide modular functionality. Plugins are divided into two categories:

- **State Plugins**: Track and store blockchain state (require chain replay when enabled/disabled)
- **API Plugins**: Expose state plugin data via JSON-RPC APIs (can be enabled/disabled without replay)

## Plugin Categories

### Core Plugins

Essential plugins required for basic node operation.

| Plugin | Type | Description | Documentation |
|--------|------|-------------|---------------|
| [chain](chain.md) | State | Core blockchain database and validation | [→](chain.md) |
| [p2p](p2p.md) | State | Peer-to-peer networking and block propagation | [→](p2p.md) |
| [webserver](webserver.md) | State | HTTP/WebSocket server for API access | [→](webserver.md) |
| [json_rpc](json_rpc.md) | State | JSON-RPC 2.0 request handling | [→](json_rpc.md) |

**Default plugins** (always enabled): `chain`, `p2p`, `webserver`

### State-Tracking Plugins

Plugins that index and store additional blockchain data.

| Plugin | Memory | Description | Documentation |
|--------|--------|-------------|---------------|
| [account_history](account_history.md) | ~200GB | Track all account transactions (memory-based) | [→](account_history.md) |
| [account_history_rocksdb](account_history_rocksdb.md) | ~70GB | Track account transactions (disk-based, RocksDB) | [→](account_history_rocksdb.md) |
| [account_by_key](account_by_key.md) | ~5GB | Index accounts by public key | [→](account_by_key.md) |
| [tags](tags.md) | ~80GB | Index content by tags and categories | [→](tags.md) |
| [follow](follow.md) | ~60GB | Track follow relationships and user feeds | [→](follow.md) |
| [market_history](market_history.md) | ~10GB | Internal market OHLCV data and order books | [→](market_history.md) |
| [witness](witness.md) | Minimal | Block production and witness scheduling | [→](witness.md) |
| [reputation](reputation.md) | ~5GB | Calculate and track user reputation scores | [→](reputation.md) |

### API Plugins

Plugins that expose data via JSON-RPC endpoints.

| Plugin | Depends On | Description | Documentation |
|--------|------------|-------------|---------------|
| [database_api](database_api.md) | chain | Core blockchain data queries | [→](database_api.md) |
| [account_history_api](account_history_api.md) | account_history / account_history_rocksdb | Account transaction history queries | [→](account_history_api.md) |
| [account_by_key_api](account_by_key_api.md) | account_by_key | Lookup accounts by public key | [→](account_by_key_api.md) |
| [tags_api](tags_api.md) | tags | Content and tag queries | [→](tags_api.md) |
| [follow_api](follow_api.md) | follow | Follow relationships and feed queries | [→](follow_api.md) |
| [market_history_api](market_history_api.md) | market_history | Market data and trading history | [→](market_history_api.md) |
| [witness_api](witness_api.md) | witness | Witness information and schedules | [→](witness_api.md) |
| [network_broadcast_api](network_broadcast_api.md) | chain | Transaction broadcasting | [→](network_broadcast_api.md) |
| [condenser_api](condenser_api.md) | Multiple | Legacy API for backward compatibility | [→](condenser_api.md) |
| [block_api](block_api.md) | chain | Block data queries | [→](block_api.md) |
| [chain_api](chain_api.md) | chain | Chain-level queries | [→](chain_api.md) |
| [reputation_api](reputation_api.md) | reputation | Reputation score queries | [→](reputation_api.md) |

### Utility Plugins

Special-purpose plugins for development, testing, and monitoring.

| Plugin | Purpose | Documentation |
|--------|---------|---------------|
| [debug_node](debug_node.md) | Development and testing utilities | [→](debug_node.md) |
| [debug_node_api](debug_node_api.md) | Debug API endpoints | [→](debug_node_api.md) |
| [test_api](test_api.md) | Testing utilities | [→](test_api.md) |
| [block_data_export](block_data_export.md) | Export block data to files | [→](block_data_export.md) |
| [block_log_info](block_log_info.md) | Block log analysis tool | [→](block_log_info.md) |
| [stats_export](stats_export.md) | Export blockchain statistics | [→](stats_export.md) |
| [statsd](statsd.md) | StatsD metrics integration | [→](statsd.md) |
| [smt_test](smt_test.md) | Smart Media Token testing | [→](smt_test.md) |

## Plugin Configuration

### Enabling Plugins

Plugins are enabled in `config.ini` using the `plugin` option:

```ini
# Enable state-tracking plugins
plugin = account_history tags follow market_history

# Enable corresponding API plugins
plugin = account_history_api tags_api follow_api market_history_api
```

Or via command line:

```bash
steemd --plugin=account_history --plugin=account_history_api
```

### Plugin Dependencies

Many plugins have dependencies that are automatically loaded:

```
tags_api (API)
  ↓ requires
tags (State)
  ↓ requires
chain (Core)
```

If you enable `tags_api`, both `tags` and `chain` are automatically loaded.

### Important Rules

1. **State plugins require chain replay**: Enabling/disabling state-tracking plugins requires replaying the blockchain
   ```bash
   steemd --replay-blockchain
   ```

2. **API plugins can be toggled freely**: API plugins can be enabled/disabled without replay

3. **Memory implications**: Each state plugin increases memory requirements

4. **Build requirements**: Some plugins require special build flags:
   - `debug_node`, `debug_node_api`, `test_api`: Require `BUILD_STEEM_TESTNET=ON`

## Plugin Selection by Use Case

### Witness Node (Block Production)

**Minimal setup for producing blocks:**

```ini
plugin = chain p2p webserver witness
plugin = database_api witness_api
```

**Memory**: ~54GB

### Full API Node

**Complete API access for applications:**

```ini
# State plugins
plugin = webserver p2p json_rpc witness
plugin = account_by_key tags follow market_history account_history_rocksdb

# API plugins
plugin = database_api account_by_key_api network_broadcast_api
plugin = tags_api follow_api market_history_api witness_api
plugin = condenser_api block_api account_history_api
```

**Memory**: ~260GB+

### Exchange Node

**Optimized for tracking deposits/withdrawals:**

```ini
plugin = webserver p2p json_rpc account_history_rocksdb
plugin = database_api account_history_api network_broadcast_api condenser_api
```

**Memory**: ~70GB

### Seed Node (P2P Only)

**Minimal node for network connectivity:**

```ini
plugin = chain p2p
```

**Memory**: ~4GB (requires `LOW_MEMORY_NODE=ON` build)

### Content Platform Node

**Optimized for social media applications:**

```ini
plugin = webserver p2p json_rpc tags follow account_by_key
plugin = database_api tags_api follow_api account_by_key_api network_broadcast_api condenser_api
```

**Memory**: ~200GB

## Performance Considerations

### Memory Usage

Total memory = Base (chain) + Sum(enabled state plugins)

| Component | Memory |
|-----------|--------|
| Base (chain, p2p) | ~24GB |
| account_history | ~200GB |
| account_history_rocksdb | ~70GB |
| tags | ~80GB |
| follow | ~60GB |
| market_history | ~10GB |
| account_by_key | ~5GB |
| reputation | ~5GB |

**Recommendation**: Use `account_history_rocksdb` instead of `account_history` to save ~130GB RAM.

### API Performance

- **Read locks expire after 1 second** - design API calls to complete in <250ms
- **Use webserver thread pool** - configure `webserver-thread-pool-size` (default: 32)
- **Monitor query performance** - long-running queries can block other requests

### Disk I/O

Plugins with high disk I/O:
- `account_history_rocksdb` - RocksDB writes
- `block_data_export` - Writes block data to files
- `stats_export` - Writes statistics periodically

**Recommendation**: Use SSD storage for RocksDB-based plugins.

## Migration Guide

### Adding State Plugins

```bash
# 1. Stop node
sudo systemctl stop steemd

# 2. Update config.ini - add plugins
vim witness_node_data_dir/config.ini

# 3. Replay blockchain (REQUIRED)
steemd --replay-blockchain --data-dir=witness_node_data_dir
```

### Removing State Plugins

```bash
# 1. Update config.ini - remove plugins
vim witness_node_data_dir/config.ini

# 2. Restart (no replay needed when removing)
sudo systemctl restart steemd
```

### Adding/Removing API Plugins

```bash
# 1. Update config.ini
vim witness_node_data_dir/config.ini

# 2. Restart (no replay needed)
sudo systemctl restart steemd
```

## Plugin Development

For creating custom plugins, see:
- [Plugin Development Guide](../development/plugin.md) - How to create plugins
- [newplugin.py](../../programs/util/newplugin.py) - Auto-generate plugin skeleton
- [example_api_plugin](../../example_plugins/example_api_plugin/) - Example implementation

## Additional Resources

- [Node Types Guide](../operations/node-types-guide.md) - Pre-configured node setups
- [System Architecture](../technical-reference/system-architecture.md) - Overall system design
- [API Notes](../technical-reference/api-notes.md) - API usage guidelines
- [Configuration Files](../../contrib/) - Example config.ini files

## License

See [LICENSE.md](../../LICENSE.md)
