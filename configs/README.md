# Configuration Files

This directory contains example configuration files for different types of Steem nodes. Copy and modify these files as needed for your use case.

## Node Mode Configurations

### `witness.config.ini`
- **Purpose**: Witness/consensus node
- **Memory**: 54GB shared memory
- **Plugins**: `witness`, `database_api`, `witness_api`
- **Use Case**: Optimized for P2P and witness node operation

### `fullnode.config.ini`
- **Purpose**: Full-featured node for supporting content websites
- **Memory**: 260GB shared memory
- **Plugins**: `account_by_key`, `tags`, `follow`, `market_history`, and more
- **APIs**: `database_api`, `account_by_key_api`, `tags_api`, `follow_api`, `market_history_api`, etc.
- **Use Case**: Backend for social media platforms
- **Note**: Use `account-history-whitelist-ops` to reduce memory usage by tracking only specific operations

### `ahnode.config.ini`
- **Purpose**: Account history specialized node
- **Memory**: 54GB shared memory
- **Plugins**: Focused on `account_history` and `account_history_api`
- **Use Case**: Account transaction history query service

### `broadcast.config.ini`
- **Purpose**: Transaction broadcast node
- **Memory**: 24GB shared memory (low footprint)
- **Plugins**: Optimized for `network_broadcast_api`
- **Use Case**: Focused on transaction submission with minimal resources

### `testnet.config.ini`
- **Purpose**: Private testnet node
- **Memory**: 30GB shared memory
- **Plugins**: Full functionality including `account_history` and `witness`
- **Features**:
  - `enable-stale-production = true` - Produce blocks even on stale chain
  - `required-participation = 0` - No participation requirements
  - For development and testing

### `fastgen.config.ini`
- **Purpose**: Fast block generation configuration
- **Feature**: Lightweight settings optimized for block production performance

### `example.config.ini`
- **Purpose**: General example configuration
- **Use Case**: Template for custom configurations

## Usage Examples

### 1. Starting a Witness Node
```bash
# Copy configuration file
cp configs/witness.config.ini witness_node_data_dir/config.ini

# Run node
./programs/steemd/steemd
```

### 2. Starting a Full Node
```bash
cp configs/fullnode.config.ini witness_node_data_dir/config.ini
./programs/steemd/steemd
```

### 3. Starting a Testnet
```bash
# Requires testnet build option
# cmake -DBUILD_STEEM_TESTNET=ON ..

cp configs/testnet.config.ini witness_node_data_dir/config.ini
./contrib/steemd-testnet-bootstrap.sh
./programs/steemd/steemd
```

## Configuration Tips

### Memory Settings
- `shared-file-size`: Shared memory file size
  - Minimum (broadcaster): 24GB
  - Default (witness): 54GB
  - Full node: 260GB+
  - Actual requirements continuously grow

### Plugin Selection
```ini
# Essential plugins
plugin = chain p2p webserver

# json_rpc for API access
plugin = json_rpc

# Witness node
plugin = witness witness_api

# Full node (optional additions)
plugin = account_history account_history_api
plugin = tags tags_api
plugin = follow follow_api
plugin = market_history market_history_api
```

### Performance Tuning
```ini
# WebSocket thread pool size
webserver-thread-pool-size = 256

# Block flush interval (0 = disabled)
flush-state-interval = 0

# Market history buckets (in seconds)
market-history-bucket-size = [15,60,300,3600,86400]
```

## Important Notes

- Node must be restarted after configuration changes
- Changing state-tracking plugins may require chain replay
- For production, consider placing `shared-file-dir` on SSD or ramdisk
- See the [Node Types Guide](../docs/getting-started/node-types-guide.md) for detailed information

## Additional Resources

- [Node Types Guide](../docs/getting-started/node-types-guide.md) - Comprehensive guide to different node types
- [Quick Start Guide](../docs/getting-started/quick-start.md) - Get started quickly with Docker
- [Deployment Scripts](../contrib/) - Scripts for deploying and managing nodes
