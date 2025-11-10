# Contrib Directory

This directory contains configuration files, scripts, and deployment tools for running Steem nodes.

## Configuration Files

Example configuration files for different node types. Copy and modify as needed.

### Node Type Configurations

#### `docker.config.ini`
- **Purpose**: Basic consensus node for Docker environments
- **Memory**: 54GB shared memory
- **Plugins**: `witness`, `database_api`, `witness_api`
- **Use Case**: Optimized for P2P and witness node operation

#### `fullnode.config.ini`
- **Purpose**: Full-featured node for supporting content websites
- **Memory**: 260GB shared memory
- **Plugins**: `account_by_key`, `tags`, `follow`, `market_history`, and more
- **APIs**: `database_api`, `account_by_key_api`, `tags_api`, `follow_api`, `market_history_api`, etc.
- **Use Case**: Backend for social media platforms

#### `fullnode.opswhitelist.config.ini`
- **Purpose**: Optimized full node that records only specific operations
- **Feature**: Selective operation tracking via `account-history-whitelist-ops`
- **Benefit**: Reduced memory and disk usage

#### `ahnode.config.ini`
- **Purpose**: Account history specialized node
- **Memory**: 54GB shared memory
- **Plugins**: Focused on `account_history` and `account_history_api`
- **Use Case**: Account transaction history query service

#### `broadcaster.config.ini`
- **Purpose**: Transaction broadcast dedicated node
- **Memory**: 24GB shared memory (low footprint)
- **Plugins**: Optimized for `network_broadcast_api`
- **Use Case**: Focused on transaction submission with minimal resources

#### `testnet.config.ini`
- **Purpose**: Private testnet node
- **Memory**: 30GB shared memory
- **Plugins**: Full functionality including `account_history` and `witness`
- **Features**:
  - `enable-stale-production = true` - Produce blocks even on stale chain
  - `required-participation = 0` - No participation requirements
  - For development and testing

#### `fastgen.config.ini`
- **Purpose**: Fast block generation configuration
- **Feature**: Lightweight settings optimized for block production performance

## Deployment Scripts

### Docker & PaaS

#### `steemdentrypoint.sh`
Docker container entrypoint script

#### `steemd.run`
Runit/daemontools style service run script

#### `startpaassteemd.sh`
- Launch steemd in PaaS environments like AWS Elastic Beanstalk
- Download/upload blockchain snapshots from/to S3 bucket
- Environment variables:
  - `USE_PAAS=true`
  - `S3_BUCKET=<bucket-name>`
  - `SYNC_TO_S3=true` (snapshot upload mode)

#### `paas-sv-run.sh`
Supervisor script for PaaS environments

#### `sync-sv-run.sh`
S3 sync service run script

### Health Check

#### `healthcheck.sh`
Node health check script
- Verify blockchain sync status
- Measure time difference from latest block
- Return HTTP 200/503 response

#### `healthcheck.conf.template`
Health check configuration template

### NGINX

#### `steemd.nginx.conf`
- NGINX reverse proxy configuration for steemd
- WebSocket connection proxy
- Load balancing (for multicore readonly mode)
- Provides `/health` endpoint

### Testnet

#### `testnetinit.sh`
Private testnet initialization script
- Generate genesis block
- Configure initial witness accounts
- Setup test environment

## Usage Examples

### 1. Starting a Basic Node
```bash
# Copy configuration file
cp contrib/docker.config.ini witness_node_data_dir/config.ini

# Run node
./programs/steemd/steemd
```

### 2. Starting a Full Node
```bash
cp contrib/fullnode.config.ini witness_node_data_dir/config.ini
./programs/steemd/steemd
```

### 3. Starting a Testnet
```bash
# Requires testnet build option
# cmake -DBUILD_STEEM_TESTNET=ON ..

cp contrib/testnet.config.ini witness_node_data_dir/config.ini
./contrib/testnetinit.sh
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

## Notes

- Node must be restarted after configuration changes
- Changing state-tracking plugins may require chain replay
- For production, consider placing `shared-file-dir` on SSD or ramdisk
