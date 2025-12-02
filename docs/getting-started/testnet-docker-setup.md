# Testnet Fullnode Setup with Docker

This guide explains how to run a Steem testnet fullnode using Docker with a custom configuration.

## Prerequisites

- Docker installed
- hanyeol/steem Docker image built
- Basic understanding of Steem blockchain

## Directory Structure

```
running-steemd/
├── witness_node_data_dir/
│   ├── blockchain/          # Chain state (auto-created)
│   └── config.ini          # Witness configuration
└── config.ini              # Base configuration
```

## Step 1: Create Directory Structure

```bash
mkdir -p running-steemd/witness_node_data_dir/blockchain
```

## Step 2: Create Configuration Files

### running-steemd/config.ini

Create the base configuration file:

```ini
# Console appender definition json: {"appender", "stream"}
log-console-appender = {"appender":"stderr","stream":"std_error"}

# File appender definition json:  {"appender", "file"}
log-file-appender = {"appender":"p2p","file":"logs/p2p/p2p.log"}

# Logger definition json: {"name", "level", "appender"}
log-logger = {"name":"default","level":"info","appender":"stderr"}
log-logger = {"name":"p2p","level":"info","appender":"stderr"}

# Plugin(s) to enable, may be specified multiple times
plugin = webserver p2p json_rpc witness account_by_key account_history tags follow market_history

plugin = database_api witness_api account_by_key_api account_history_api network_broadcast_api tags_api follow_api market_history_api block_api chain_api reputation_api

# the location of the chain shared memory files (absolute path or relative to application data dir)
shared-file-dir = "blockchain"

# Size of the shared memory file. Default: 54G
shared-file-size = 54G

# flush shared memory changes to disk every N blocks
flush-state-interval = 0

# Set the maximum size of cached feed for an account
follow-max-feed-size = 500

# Track market history by grouping orders into buckets of equal size measured in seconds specified as a JSON array of numbers
market-history-bucket-size = [15,60,300,3600,86400]

# How far back in time to track history for each bucket size, measured in the number of buckets (default: 5760)
market-history-buckets-per-size = 5760

# User agent to advertise to peers
p2p-user-agent = Graphene Reference Implementation

# The local IP and port to listen for incoming http connections.
webserver-http-endpoint = 0.0.0.0:8090

# The local IP and port to listen for incoming websocket connections.
webserver-ws-endpoint = 0.0.0.0:8091

# Number of threads used to handle queries. Default: 32.
webserver-thread-pool-size = 256

# Enable block production, even if the chain is stale.
enable-stale-production = true

# Percent of witnesses (0-99) that must be participating in order to produce blocks
required-participation = 0

# name of witness controlled by this node (e.g. initwitness )
witness = "genesis"

# WIF PRIVATE KEY to be used by witnesses
private-key = 5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n
```

### running-steemd/witness_node_data_dir/config.ini

Create the witness-specific configuration file with the same content as above, or use the sed commands:

```bash
# Copy base config to witness directory
cp running-steemd/config.ini running-steemd/witness_node_data_dir/config.ini

# Enable witness settings
sed -i '' 's/enable-stale-production = false/enable-stale-production = true/' running-steemd/witness_node_data_dir/config.ini
sed -i '' 's/# required-participation =/required-participation = 0/' running-steemd/witness_node_data_dir/config.ini
sed -i '' 's/# witness =/witness = "genesis"/' running-steemd/witness_node_data_dir/config.ini
sed -i '' 's/# private-key =/private-key = 5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n/' running-steemd/witness_node_data_dir/config.ini
sed -i '' 's/# webserver-http-endpoint =/webserver-http-endpoint = 0.0.0.0:8090/' running-steemd/witness_node_data_dir/config.ini
sed -i '' 's/# webserver-ws-endpoint =/webserver-ws-endpoint = 0.0.0.0:8091/' running-steemd/witness_node_data_dir/config.ini
```

## Step 3: Run Docker Container

Run the testnet fullnode using the testnet binary:

```bash
docker run -d \
  --name steem-testnet \
  -p 8090:8090 \
  -p 8091:8091 \
  -p 9876:9876 \
  -e IS_TESTNET=1 \
  -v "$(pwd)/running-steemd/witness_node_data_dir:/var/lib/steemd" \
  -v "$(pwd)/running-steemd/config.ini:/var/lib/steemd/config.ini" \
  hanyeol/steem \
  /usr/local/steemd-test/bin/steemd --data-dir=/var/lib/steemd
```

### Port Mappings

- **8090**: HTTP API endpoint
- **8091**: WebSocket API endpoint
- **9876**: P2P endpoint (not used by testnet binary, which uses 1776)

## Step 4: Verify Testnet is Running

Check the logs to verify the testnet is producing blocks:

```bash
docker logs steem-testnet --tail 50
```

Expected output:
```
------------------------------------------------------

            STARTING TEST NETWORK

------------------------------------------------------
genesis public key: TST6LLegbAgLAy28EHrffBVuANFWcFgmqRMW13wBmTExqFE9SCkg4
genesis private key: 5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n
blockchain version: 0.0.0
------------------------------------------------------

********************************
*                              *
*   ------- NEW CHAIN ------   *
*   -   Welcome to Steem!  -   *
*   ------------------------   *
*                              *
********************************

Generated block #1 with timestamp 2025-11-23T01:28:42 at time 2025-11-23T01:28:42
Generated block #2 with timestamp 2025-11-23T01:28:45 at time 2025-11-23T01:28:45
Generated block #3 with timestamp 2025-11-23T01:28:48 at time 2025-11-23T01:28:48
...
```

## Step 5: Monitor Logs in Real-time

```bash
docker logs -f steem-testnet
```

## Important Notes

### Testnet vs Mainnet Binary

The Docker image contains three binaries:
- `/usr/local/steemd-low/bin/steemd` - Low memory node
- `/usr/local/steemd-high/bin/steemd` - Full API node
- `/usr/local/steemd-test/bin/steemd` - Test node (`BUILD_STEEM_TESTNET=ON`)

**You must use the test binary** for the genesis account and private key to work correctly.

### Genesis Account Credentials

- **Account Name**: `genesis`
- **Public Key**: `TST6LLegbAgLAy28EHrffBVuANFWcFgmqRMW13wBmTExqFE9SCkg4`
- **Private Key (WIF)**: `5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n`

The private key is derived from `fc::sha256::hash(std::string("init_key"))` and is the same for all testnet instances.

### Configuration File Behavior

The Docker entrypoint script (`/usr/local/bin/steemd-entrypoint.sh`) automatically copies configuration files from `/etc/steemd/` to the data directory. To use a custom config:
1. Mount your config file to `/var/lib/steemd/config.ini` (overrides after copy)
2. Or use environment variables like `STEEMD_WITNESS_NAME` and `STEEMD_PRIVATE_KEY`

In this setup, we override the command entirely to use the testnet binary directly.

## Container Management

### Stop the container
```bash
docker stop steem-testnet
```

### Start the container
```bash
docker start steem-testnet
```

### Remove the container
```bash
docker rm -f steem-testnet
```

### Clean blockchain data (start fresh)
```bash
rm -rf running-steemd/witness_node_data_dir/blockchain/*
```

## API Access

Once running, you can access the APIs at:
- **HTTP**: `http://localhost:8090`
- **WebSocket**: `ws://localhost:8091`

Example API call:
```bash
curl -X POST http://localhost:8090 \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "method": "database_api.get_dynamic_global_properties",
    "params": {},
    "id": 1
  }'
```

## Troubleshooting

### "No witnesses configured" Error

If you see this error, it means:
1. The config.ini file is not being read correctly
2. Or you're using the mainnet binary instead of testnet binary

Solution: Make sure you're using `/usr/local/steemd-test/bin/steemd`

### "I don't have the private key" Error

If you see errors like:
```
Not producing block because I don't have the private key for STM8GC13uCZbP44HzMLV6zPZGwVQ8Nt4Kji8PapsPiNq1BK153XTX
```

This means you're running the **mainnet binary** instead of the testnet binary. The mainnet genesis public key is `STM8GC...` while testnet is `TST6LL...`.

Solution: Use the correct command with `/usr/local/steemd-test/bin/steemd`

### Config File Gets Overwritten

The entrypoint script copies config files on startup. To prevent this:
- Override the command to run `steemd` directly (as shown in Step 3)
- Or set the appropriate environment variables instead of relying on config.ini

## Advanced Configuration

### Fullnode Configuration

The configuration above includes the most common plugins for a fullnode. The enabled plugins are:

**State Plugins:**
- `witness` - Block production
- `account_by_key` - Account lookup by public key
- `account_history` - Track account transaction history
- `tags` - Tag and trending support
- `follow` - Follow/unfollow and feed support
- `market_history` - Market data and price history

**API Plugins:**
- `database_api` - Core blockchain data queries
- `witness_api` - Witness-related queries
- `account_by_key_api` - Account lookup API
- `account_history_api` - Account history queries
- `network_broadcast_api` - Transaction broadcast
- `tags_api` - Tag and content queries
- `follow_api` - Social features API
- `market_history_api` - Market data API
- `block_api` - Block data queries
- `chain_api` - Chain-level operations
- `reputation_api` - Reputation calculations

### Available API Plugins

The following API plugins are available in this build:

- `account_by_key_api` - Account lookup by public key
- `account_history_api` - Account transaction history
- `block_api` - Block data queries
- `chain_api` - Chain-level operations (consider adding)
- `database_api` - Core blockchain data
- `debug_node_api` - Debug operations (testnet only)
- `follow_api` - Social features
- `market_history_api` - Market data
- `network_broadcast_api` - Transaction broadcast
- `reputation_api` - Reputation calculations (consider adding)
- `tags_api` - Tag and content queries
- `test_api` - Testing utilities (testnet only)
- `witness_api` - Witness operations

**Note:** Bridge API and condenser_api are not available in this build. If you need legacy API compatibility, you may need to use an older version or implement a separate bridge service.

### Adjusting Memory Settings

For lower memory usage, reduce the shared file size:

```ini
shared-file-size = 8G
```

### Enabling Debug Logging

Change log level to debug:

```ini
log-logger = {"name":"default","level":"debug","appender":"stderr"}
```
