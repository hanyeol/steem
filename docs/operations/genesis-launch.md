# Genesis Launch Guide

How to launch a new Steem blockchain from genesis (block 0).

## Overview

This guide covers the complete process of launching a new Steem blockchain, including:
- Configuring genesis parameters
- Creating the initial witness
- Starting block production
- Bootstrapping the network
- Adding additional witnesses

**Use Cases**:
- Private testnet deployment
- Development environment
- Blockchain forks
- Custom Steem-based chains

## Prerequisites

### Build Requirements

Build `steemd` for your target network. For detailed build instructions including dependency installation and platform-specific requirements, see [Building Steem](../getting-started/building.md).

#### For Testnet (Development/Testing)

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_STEEM_TESTNET=ON \
      ..
make -j$(nproc) steemd
```

#### For Mainnet (Production/Custom Chain)

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_STEEM_TESTNET=OFF \
      ..
make -j$(nproc) steemd
```

**Note**: Both testnet and mainnet builds can launch genesis chains. The key difference is in genesis parameters:
- **Testnet**: 250M initial supply, auto-generated keys, "TST" prefix
- **Mainnet**: 0 initial supply, fixed public key, "STM" prefix

For complete build instructions including system requirements, dependency installation, and troubleshooting, refer to [Building Steem](../getting-started/building.md).

### System Requirements

- **RAM**: 4GB minimum
- **Disk**: 10GB available for blockchain data
- **Network**: P2P port accessible (default: 2001)

## Genesis Configuration

### Chain Parameters

Genesis parameters are configured at build time in `src/core/protocol/include/steem/protocol/config.hpp`.

#### Testnet Configuration

When building with `BUILD_STEEM_TESTNET=ON`:

- **Chain ID**: "testnet"
- **Address Prefix**: "TST"
- **Genesis Time**: 2016-01-01 00:00:00 UTC
- **Initial Supply**: 250,000,000 STEEM
- **Initial Account**: "genesis"
- **Private Key**: Auto-generated from "init_key" seed
  - WIF: `5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n`
  - Public Key: `TST6LLegbAgLAy28EHrffBVuANFWcFgmqRMW13wBmTExqFE9SCkg4`

#### Mainnet Configuration

For mainnet builds (`BUILD_STEEM_TESTNET=OFF`, default):

- **Chain ID**: "" (empty)
- **Address Prefix**: "STM"
- **Genesis Time**: 2016-03-24 16:00:00 UTC
- **Initial Supply**: 0 STEEM
- **Initial Public Key**: `STM8GC13uCZbP44HzMLV6zPZGwVQ8Nt4Kji8PapsPiNq1BK153XTX`

**Important for Custom Chains**:
- Modify `STEEM_CHAIN_ID_NAME` to make it unique (prevents replay attacks)
- Set `STEEM_INIT_SUPPLY` if you want pre-mined tokens
- Generate your own keys for the initial witness

## Genesis Initialization Process

When `steemd` starts with an empty database, it automatically creates the genesis state with:

- **System Accounts**: `null` (burn), `temp` (temporary operations)
- **Initial Account**: `genesis` receives all initial supply (250M STEEM for testnet)
- **Initial Witness**: `genesis` configured for block production
- **Global Properties**: Initial blockchain state and parameters
- **Hardfork State**: Genesis hardfork timestamp
- **Reward Funds**: Post and curation reward pools

## Launching Your Genesis Chain

### Step 1: Prepare Configuration

Create `config.ini` for your genesis node:

```ini
# Logging
log-console-appender = {"appender":"stderr","stream":"std_error"}
log-logger = {"name":"default","level":"info","appender":"stderr"}

# Required plugins
plugin = chain p2p webserver witness

# API plugins (optional, for development)
plugin = database_api witness_api

# Network settings
p2p-endpoint = 0.0.0.0:2001
webserver-http-endpoint = 0.0.0.0:8090
webserver-ws-endpoint = 0.0.0.0:8090

# Witness configuration
enable-stale-production = true
required-participation = 0
witness = "genesis"

# Private key for testnet genesis
# Generated from sha256("init_key")
private-key = 5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n
```

**Note**: For testnet, the genesis private key is always `5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n`

### Step 2: Launch Genesis Node

```bash
# Create data directory
mkdir -p witness_node_data_dir

# Copy config
cp config.ini witness_node_data_dir/

# Start steemd (will create genesis on first run)
./programs/steemd/steemd \
    --data-dir=witness_node_data_dir \
    2>&1 | tee genesis.log
```

### Step 3: Verify Genesis

Check the logs for genesis initialization:

```
info  database: Open database in /path/to/witness_node_data_dir/blockchain
info  database: Creating genesis state
info  database: Genesis state created successfully
info  witness: Witness genesis starting block production
info  witness: Generated block #1 with timestamp 2016-01-01T00:00:03
```

### Step 4: Query Genesis State

Using `cli_wallet` or API calls:

```bash
# Connect to node
./programs/cli_wallet/cli_wallet -s ws://127.0.0.1:8090

# Check genesis account
>>> get_account genesis

# Check witness
>>> get_witness genesis

# Check blockchain info
>>> info
```

Expected output:

```json
{
  "head_block_num": 1,
  "head_block_id": "0000000109b3667c257e7171da61984da0d3279f",
  "time": "2016-01-01T00:00:03",
  "current_witness": "genesis",
  "total_supply": "250000000.000 STEEM",
  "current_supply": "250000000.000 STEEM"
}
```

## Adding Witnesses

### Create New Witness Account

```bash
# In cli_wallet
>>> create_account genesis "witness1" "" true

# Fund the account
>>> transfer genesis witness1 "1000.000 STEEM" "Initial funding" true

# Generate witness keys
>>> suggest_brain_key

# Update witness
>>> update_witness "witness1" \
    "https://witness1.example.com" \
    "STM..." \  # signing key from suggest_brain_key
    {"account_creation_fee": "0.100 STEEM", "maximum_block_size": 65536} \
    true
```

### Vote for Witness

```bash
# Vote with genesis account
>>> vote_for_witness genesis witness1 true true
```

### Add to Config

Add witness to `config.ini`:

```ini
witness = "genesis"
witness = "witness1"

# Add private key for witness1
private-key = 5K...  # WIF from suggest_brain_key
```

### Restart Node

```bash
# Restart steemd to pick up new witness
pkill steemd
./programs/steemd/steemd --data-dir=witness_node_data_dir
```

## Multi-Node Network

### Bootstrap Additional Nodes

#### Node 2 Configuration

```ini
# config.ini for node 2
p2p-endpoint = 0.0.0.0:2001
p2p-seed-node = 127.0.0.1:2001  # Point to genesis node

# Sync-only node (no witness)
enable-stale-production = false

# API access
webserver-http-endpoint = 0.0.0.0:8091
webserver-ws-endpoint = 0.0.0.0:8091

plugin = chain p2p webserver
plugin = database_api
```

#### Launch Node 2

```bash
mkdir -p node2_data
cp config_node2.ini node2_data/config.ini
./programs/steemd/steemd --data-dir=node2_data
```

Node 2 will sync from genesis node and replay all blocks.

### Network Topology

```
Genesis Node (genesis)
    ├── Port 2001: P2P
    ├── Port 8090: API
    └── Witness: genesis

Peer Node 2
    ├── Port 2001: P2P (connects to genesis)
    ├── Port 8091: API
    └── Sync only

Witness Node 3
    ├── Port 2001: P2P (connects to genesis)
    ├── Port 8092: API
    └── Witness: witness1
```

## Genesis Parameters Reference

### Time Constants

| Parameter | Testnet | Mainnet | Description |
|-----------|---------|---------|-------------|
| `STEEM_GENESIS_TIME` | 2016-01-01 00:00:00 | 2016-03-24 16:00:00 | Genesis block timestamp |
| `STEEM_BLOCK_INTERVAL` | 3 seconds | 3 seconds | Time between blocks |
| `STEEM_BLOCKS_PER_DAY` | 28,800 | 28,800 | Blocks produced daily |

### Supply & Distribution

| Parameter | Testnet | Mainnet | Description |
|-----------|---------|---------|-------------|
| `STEEM_INIT_SUPPLY` | 250,000,000 STEEM | 0 STEEM | Initial token supply |

### Witness Parameters

| Parameter | Value | Description |
|-----------|-------|-------------|
| `STEEM_MAX_WITNESSES` | 21 | Maximum active witnesses |
| `STEEM_MAX_VOTED_WITNESSES` | 20 | Elected witnesses |
| `STEEM_MAX_RUNNER_WITNESSES` | 1 | Backup witnesses |
| `STEEM_HARDFORK_REQUIRED_WITNESSES` | 17 | Witnesses needed for hardfork |

## Troubleshooting

### Genesis Already Exists

**Error**: `Database already initialized`

**Solution**: Delete existing blockchain data:

```bash
rm -rf witness_node_data_dir/blockchain
./programs/steemd/steemd --data-dir=witness_node_data_dir
```

### No Blocks Produced

**Error**: Node starts but doesn't produce blocks

**Check**:
1. `witness = "genesis"` in config
2. `private-key` matches genesis's key
3. `enable-stale-production = true` is set
4. System time is correct

**Debug**:
```bash
# Check witness plugin loaded
grep "witness_plugin" genesis.log

# Check witness scheduled
grep "starting block production" genesis.log
```

### Chain ID Mismatch

**Error**: `Remote chain ID does not match`

**Cause**: Trying to connect nodes with different `STEEM_CHAIN_ID_NAME`

**Solution**: Ensure all nodes are built with same `config.hpp` settings

### Port Already in Use

**Error**: `Address already in use`

**Solution**:
```bash
# Find process using port
lsof -i :2001

# Kill old steemd instance
pkill steemd

# Or use different port
p2p-endpoint = 0.0.0.0:2002
```

## Docker Deployment

### Docker Build for Genesis

Build a custom Docker image with genesis configuration:

```bash
# Build from repository root
docker build -t steem:latest .

# Or build with testnet flag
docker build --build-arg BUILD_STEP=2 -t steem:testnet .
```

### Configuration File Location

When running `steemd` in Docker, the configuration file should be located at:

**Default location**: `/var/lib/steemd/config.ini`

This is the directory specified by the `HOME` environment variable (default: `/var/lib/steemd`).

#### Providing config.ini to Docker Containers

Copy your `config.ini` into the volume before starting the container:

```bash
# Create volume
docker volume create steem-data

# Copy config.ini into volume
docker run --rm \
  -v steem-data:/var/lib/steemd \
  -v $(pwd)/config.ini:/tmp/config.ini:ro \
  alpine \
  cp /tmp/config.ini /var/lib/steemd/config.ini

# Start container
docker run -d \
  --name steem-genesis \
  -v steem-data:/var/lib/steemd \
  -p 2001:2001 \
  -p 8090:8090 \
  steem:testnet
```

#### Verifying Configuration

Check if config.ini is properly loaded:

```bash
# View config in running container
docker exec steem-genesis cat /var/lib/steemd/config.ini

# Check which config file is being used
docker exec steem-genesis ps aux | grep steemd
```

### Storage Configuration

Docker containers require proper storage allocation for blockchain data.

#### Storage Requirements

| Node Type | Shared Memory | Block Log | Total Minimum | Recommended |
|-----------|---------------|-----------|---------------|-------------|
| **Genesis Node** | 8GB | 1GB | 10GB | 20GB |
| **Full Node (API)** | 56GB | 27GB+ | 100GB | 150GB |
| **Witness Node** | 16GB | 27GB+ | 50GB | 80GB |
| **Seed Node (Low Memory)** | 4GB | 27GB+ | 35GB | 50GB |

#### Docker Volume Setup

```bash
# Create named volume for persistent storage
docker volume create steem-data

# Inspect volume
docker volume inspect steem-data

# Volume location: /var/lib/docker/volumes/steem-data/_data
```

#### Storage Options

**Option 1: Named Volume (Recommended)**

```bash
docker run -d \
  --name steem-genesis \
  -v steem-data:/var/lib/steemd \
  -p 2001:2001 \
  -p 8090:8090 \
  steem:testnet
```

**Option 2: Bind Mount**

```bash
# Create host directory
mkdir -p /mnt/steem-data

docker run -d \
  --name steem-genesis \
  -v /mnt/steem-data:/var/lib/steemd \
  -p 2001:2001 \
  -p 8090:8090 \
  steem:testnet
```

**Option 3: tmpfs for Shared Memory (High Performance)**

```bash
docker run -d \
  --name steem-genesis \
  -v steem-data:/var/lib/steemd \
  --tmpfs /dev/shm:rw,size=16g \
  --shm-size=16g \
  -p 2001:2001 \
  -p 8090:8090 \
  steem:testnet
```

### Docker Environment Variables

Configure storage and node behavior via environment variables:

```bash
docker run -d \
  --name steem-genesis \
  -e HOME=/var/lib/steemd \
  -e STEEMD_SHARED_FILE_SIZE=8G \
  -e STEEMD_SHARED_FILE_DIR=/dev/shm \
  -v steem-data:/var/lib/steemd \
  --shm-size=16g \
  -p 2001:2001 \
  -p 8090:8090 \
  steem:testnet
```

#### Key Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `HOME` | `/var/lib/steemd` | Data directory path |
| `STEEMD_SHARED_FILE_SIZE` | `54G` | Shared memory file size |
| `STEEMD_SHARED_FILE_DIR` | `/var/lib/steemd/blockchain` | Shared memory location |
| `STEEMD_SEED_NODES` | (empty) | P2P seed nodes (whitespace-delimited) |

### Docker Compose Setup

Create `docker-compose.yml` for genesis network:

```yaml
version: '3.8'

services:
  genesis-node:
    image: steem:testnet
    container_name: steem-genesis
    ports:
      - "2001:2001"
      - "8090:8090"
    volumes:
      - genesis-data:/var/lib/steemd
      - ./config.ini:/var/lib/steemd/config.ini:ro
    environment:
      - HOME=/var/lib/steemd
      - STEEMD_SHARED_FILE_SIZE=8G
      - STEEMD_SHARED_FILE_DIR=/dev/shm
    shm_size: 16gb
    restart: unless-stopped
    networks:
      - steem-network

  witness-node:
    image: steem:testnet
    container_name: steem-witness1
    ports:
      - "2002:2001"
      - "8091:8090"
    volumes:
      - witness1-data:/var/lib/steemd
      - ./witness1-config.ini:/var/lib/steemd/config.ini:ro
    environment:
      - HOME=/var/lib/steemd
      - STEEMD_SHARED_FILE_SIZE=8G
      - STEEMD_SEED_NODES=genesis-node:2001
    shm_size: 16gb
    restart: unless-stopped
    depends_on:
      - genesis-node
    networks:
      - steem-network

  api-node:
    image: steem:testnet
    container_name: steem-api
    ports:
      - "2003:2001"
      - "8092:8090"
    volumes:
      - api-data:/var/lib/steemd
      - ./api-config.ini:/var/lib/steemd/config.ini:ro
    environment:
      - HOME=/var/lib/steemd
      - STEEMD_SHARED_FILE_SIZE=32G
      - STEEMD_SEED_NODES=genesis-node:2001
    shm_size: 48gb
    restart: unless-stopped
    depends_on:
      - genesis-node
    networks:
      - steem-network

volumes:
  genesis-data:
    driver: local
  witness1-data:
    driver: local
  api-data:
    driver: local

networks:
  steem-network:
    driver: bridge
```

Launch the network:

```bash
docker-compose up -d

# View logs
docker-compose logs -f genesis-node

# Scale witnesses
docker-compose up -d --scale witness-node=3
```

### Storage Monitoring

Monitor storage usage in containers:

```bash
# Check container disk usage
docker exec steem-genesis df -h /var/lib/steemd

# Check shared memory usage
docker exec steem-genesis df -h /dev/shm

# Monitor in real-time
watch -n 5 'docker exec steem-genesis du -sh /var/lib/steemd/*'
```

### Storage Optimization for Genesis

For new genesis chains, optimize storage allocation:

```bash
# Minimal genesis node (development)
docker run -d \
  --name steem-genesis-dev \
  -v genesis-dev-data:/var/lib/steemd \
  -e STEEMD_SHARED_FILE_SIZE=2G \
  --shm-size=4g \
  -p 2001:2001 -p 8090:8090 \
  steem:testnet

# Production genesis node
docker run -d \
  --name steem-genesis-prod \
  -v genesis-prod-data:/var/lib/steemd \
  -e STEEMD_SHARED_FILE_SIZE=16G \
  --shm-size=24g \
  --restart=always \
  -p 2001:2001 -p 8090:8090 \
  steem:testnet
```

### Backup and Recovery

#### Backup Genesis State

```bash
# Stop container
docker stop steem-genesis

# Backup entire data volume
docker run --rm \
  -v steem-data:/source:ro \
  -v $(pwd):/backup \
  alpine \
  tar czf /backup/genesis-backup-$(date +%Y%m%d).tar.gz -C /source .

# Restart container
docker start steem-genesis
```

#### Restore from Backup

```bash
# Create new volume
docker volume create steem-genesis-restore

# Restore data
docker run --rm \
  -v steem-genesis-restore:/target \
  -v $(pwd):/backup \
  alpine \
  tar xzf /backup/genesis-backup-20250101.tar.gz -C /target

# Start container with restored volume
docker run -d \
  --name steem-genesis-restored \
  -v steem-genesis-restore:/var/lib/steemd \
  -p 2001:2001 -p 8090:8090 \
  steem:testnet
```

### Storage Performance Tuning

For optimal performance in production:

```bash
docker run -d \
  --name steem-genesis-optimized \
  -v steem-data:/var/lib/steemd \
  -e STEEMD_SHARED_FILE_DIR=/dev/shm \
  --tmpfs /dev/shm:rw,size=24g,mode=1777 \
  --shm-size=24g \
  --memory=32g \
  --cpus=4 \
  --restart=always \
  -p 2001:2001 -p 8090:8090 \
  steem:testnet
```

## Advanced Topics

### Custom Genesis Supply

To change the initial token supply, modify `STEEM_INIT_SUPPLY` in `src/core/protocol/include/steem/protocol/config.hpp` and rebuild.

Example: For 1 billion initial supply instead of 250 million, change the value accordingly and run `make -j$(nproc) steemd`.

### Multiple Genesis Witnesses

To create multiple initial witnesses, increase `STEEM_NUM_GENESIS_WITNESSES` in `config.hpp`. This will create accounts: `genesis`, `genesis1`, `genesis2`, etc.

### Custom Genesis Time

Genesis time can be customized by setting `STEEM_GENESIS_TIME` in `config.hpp`. Use a Unix timestamp (seconds since epoch).

**Important**: Genesis time affects all time-based protocol features including block timestamps, cashout windows, and vesting periods.

## Production Considerations

### Security

1. **Never reuse mainnet keys**: Generate unique keys for your chain
2. **Secure private keys**: Store witness keys in encrypted storage
3. **Network isolation**: Use firewalls for witness nodes
4. **Backup strategy**: Regularly backup `blockchain/` directory

### Performance

1. **SSD storage**: Use SSD for `blockchain/` directory
2. **Memory**: Allocate sufficient RAM for shared memory file
3. **CPU**: Ensure consistent block production (every 3 seconds)

### Monitoring

Monitor genesis node:

```bash
# Block production rate
tail -f genesis.log | grep "Generated block"

# Network connectivity
tail -f genesis.log | grep "p2p"

# Witness participation
curl -s http://127.0.0.1:8090 | jq '.participation'
```

## Additional Resources

- [Building Steem](building.md)
- [Testing Guide](testing.md)
- [Plugin Development](plugin.md)
- [Config Reference](../../configs/testnet.config.ini)
- [Protocol Config](../src/core/protocol/include/steem/protocol/config.hpp)
- [Database Implementation](../src/core/chain/database.cpp)

## License

See [LICENSE.md](../LICENSE.md)
