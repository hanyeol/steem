# steemd

Main Steem blockchain node daemon.

## Overview

`steemd` is the core Steem blockchain node that:
- Validates and processes blocks
- Maintains blockchain state
- Participates in P2P network
- Provides JSON-RPC API access
- Can produce blocks (witness mode)

## Building

```bash
cd build
make -j$(nproc) steemd
```

The binary will be at `programs/steemd/steemd`.

## Running

### First Run

```bash
# Generate default config
./steemd

# This creates witness_node_data_dir/config.ini
# Edit config.ini, then run again
```

### With Custom Config

```bash
./steemd --data-dir=/path/to/data
```

### Command-Line Options

```bash
./steemd --help
```

Common options:
- `--data-dir` - Data directory path
- `--replay-blockchain` - Replay from block log
- `--resync-blockchain` - Delete state and resync
- `--plugin` - Enable specific plugins

## Node Types

### Witness Node (Block Producer)

Minimal configuration for block production:

```ini
plugin = chain p2p witness

# Witness account
witness = "your-witness-name"

# Private signing key
private-key = WIF_PRIVATE_KEY

# P2P endpoint
p2p-endpoint = 0.0.0.0:2001

# Seed nodes
p2p-seed-node = seed1.steemit.com:2001
```

### API Node (Read-Only)

Serve API requests without block production:

```ini
plugin = chain p2p webserver json_rpc
plugin = database_api condenser_api

# HTTP/WebSocket endpoint
webserver-http-endpoint = 127.0.0.1:8090
webserver-ws-endpoint = 127.0.0.1:8090

# P2P
p2p-endpoint = 0.0.0.0:2001
p2p-seed-node = seed1.steemit.com:2001
```

### Full Node (Content Platform Backend)

All features for content websites:

```ini
plugin = chain p2p webserver json_rpc
plugin = witness account_history follow tags market_history
plugin = database_api account_history_api follow_api tags_api market_history_api condenser_api

# Shared memory (adjust for full node)
shared-file-size = 260G

# API endpoints
webserver-http-endpoint = 127.0.0.1:8090
webserver-thread-pool-size = 256

# P2P
p2p-endpoint = 0.0.0.0:2001
p2p-seed-node = seed1.steemit.com:2001
```

## Key Configuration Options

### Blockchain Data

```ini
# Shared memory location
shared-file-dir = blockchain

# Shared memory size
shared-file-size = 54G

# Block log location (default: data-dir/blockchain/block_log)
```

### Plugins

```ini
# Enable plugins
plugin = chain p2p webserver json_rpc witness

# API plugins
plugin = database_api condenser_api
```

### Network

```ini
# P2P listen address
p2p-endpoint = 0.0.0.0:2001

# Seed nodes for discovery
p2p-seed-node = seed1.steemit.com:2001
p2p-seed-node = seed2.steemit.com:2001

# Max connections
p2p-max-connections = 100
```

### API Server

```ini
# HTTP endpoint (use 127.0.0.1 for security)
webserver-http-endpoint = 127.0.0.1:8090

# WebSocket endpoint
webserver-ws-endpoint = 127.0.0.1:8090

# Thread pool for requests
webserver-thread-pool-size = 256

# Public APIs
public-api = database_api condenser_api
```

## Maintenance Operations

### Replay Blockchain

Rebuild state from block log:

```bash
./steemd --replay-blockchain
```

Use when:
- Changing state-tracking plugins
- State corruption
- Software upgrades requiring replay

### Resync Blockchain

Delete state and re-download:

```bash
./steemd --resync-blockchain
```

**Warning**: Deletes all state, downloads entire blockchain.

### Validate Database

Check database integrity:

```bash
./steemd --validate-database
```

## Monitoring

### Logs

Default log location: `witness_node_data_dir/logs/`

Configure logging in `config.ini`:

```ini
log-logger = {"name":"default","level":"info","appender":"stderr"}
log-logger = {"name":"p2p","level":"warn","appender":"p2p"}
```

### Metrics

Use `statsd` plugin for monitoring:

```ini
plugin = statsd
statsd-endpoint = 127.0.0.1:8125
```

Integrates with Grafana, DataDog, etc.

### Health Check

Check node status via API:

```bash
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"database_api.get_dynamic_global_properties",
  "params":{},
  "id":1
}' http://localhost:8090
```

## Performance Tuning

### Memory

```ini
# Use SSD or RAM disk for shared memory
shared-file-dir = /mnt/ramdisk/blockchain

# Adequate size for node type
shared-file-size = 54G  # Witness
shared-file-size = 260G # Full node
```

### Network

```ini
# Increase connections for better sync
p2p-max-connections = 200

# Multiple seed nodes
p2p-seed-node = seed1.steemit.com:2001
p2p-seed-node = seed2.steemit.com:2001
```

### Virtual Memory (Linux)

For initial sync and replay:

```bash
echo 75 | sudo tee /proc/sys/vm/dirty_background_ratio
echo 1000 | sudo tee /proc/sys/vm/dirty_expire_centisecs
echo 80 | sudo tee /proc/sys/vm/dirty_ratio
echo 30000 | sudo tee /proc/sys/vm/dirty_writeback_centisecs
```

## Docker

### Using Official Image

```bash
# Basic node
docker run -d -p 2001:2001 -p 8090:8090 \
  --name steemd steemit/steem

# Full node
docker run -d -p 2001:2001 -p 8090:8090 \
  --env USE_WAY_TOO_MUCH_RAM=1 \
  --env USE_FULL_WEB_NODE=1 \
  --name steemd-full steemit/steem
```

### Building Docker Image

```bash
docker build -t my-steemd .
```

## Troubleshooting

### Out of Memory

```
Error: bad_alloc: std::bad_alloc
```

**Solution**: Increase `shared-file-size` in config.

### Database Locked

```
Error: database is locked
```

**Solution**: Another steemd instance is running. Stop it first.

### Cannot Sync

**Symptoms**: Stuck at old block number

**Solutions**:
1. Check P2P connections
2. Try different seed nodes
3. Check system time is synchronized
4. Check firewall allows P2P port

### API Not Responding

**Solutions**:
1. Check webserver plugin is enabled
2. Verify endpoint in config
3. Check firewall rules
4. Review logs for errors

## Security

### API Security

```ini
# Bind to localhost only
webserver-http-endpoint = 127.0.0.1:8090

# Use nginx for public access with TLS
```

### Private Keys

```ini
# Witness signing key (separate from account keys)
private-key = WIF_PRIVATE_KEY

# Never share private keys
# Use dedicated signing key
# Backup securely
```

### Firewall

```bash
# Allow P2P
sudo ufw allow 2001/tcp

# Block API from public (use reverse proxy)
sudo ufw deny 8090/tcp
```

## System Requirements

### Minimum (Witness Node)
- CPU: 2 cores
- RAM: 8GB
- Disk: 100GB SSD
- Network: 100 Mbps

### Recommended (Full Node)
- CPU: 4+ cores
- RAM: 32GB+
- Disk: 500GB+ NVMe SSD
- Network: 1 Gbps

### Storage Growth
- Block log: ~30GB (growing)
- Shared memory: 54GB (witness) to 260GB+ (full node)
- Estimate ~10GB growth per year

## Additional Resources

- [Building Instructions](../../doc/building.md)
- [Configuration Examples](../../contrib/)
- [Plugin Documentation](../../libraries/plugins/)
- [Testing Guide](../../doc/testing.md)
