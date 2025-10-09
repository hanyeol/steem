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

Build `steemd` for your target network. For detailed build instructions including dependency installation and platform-specific requirements, see [Building Steem](building.md).

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

For complete build instructions including system requirements, dependency installation, and troubleshooting, refer to [docs/building.md](building.md).

### System Requirements

- **RAM**: 4GB minimum
- **Disk**: 10GB available for blockchain data
- **Network**: P2P port accessible (default: 2001)

## Genesis Configuration

### Chain Parameters

Genesis parameters are defined in [`libraries/protocol/include/steem/protocol/config.hpp`](../libraries/protocol/include/steem/protocol/config.hpp).

#### Testnet Configuration

When `BUILD_STEEM_TESTNET=ON` is set:

```cpp
// Chain identity
#define STEEM_CHAIN_ID_NAME "testnet"
#define STEEM_CHAIN_ID (fc::sha256::hash(STEEM_CHAIN_ID_NAME))
#define STEEM_ADDRESS_PREFIX "TST"

// Genesis time
#define STEEM_GENESIS_TIME (fc::time_point_sec(1451606400))

// Initial supply (250 million STEEM for testnet)
#define STEEM_INIT_SUPPLY (int64_t(250) * int64_t(1000000) * int64_t(1000))

// Initial miner
#define STEEM_INIT_MINER_NAME "initminer"
#define STEEM_NUM_INIT_MINERS 1

// Initial public key (auto-generated from init_key for testnet)
#define STEEM_INIT_PRIVATE_KEY (fc::ecc::private_key::regenerate(fc::sha256::hash(std::string("init_key"))))
```

#### Mainnet Configuration

For mainnet (`BUILD_STEEM_TESTNET=OFF`, default):

```cpp
// Chain identity
#define STEEM_CHAIN_ID_NAME ""
#define STEEM_CHAIN_ID fc::sha256()
#define STEEM_ADDRESS_PREFIX "STM"

// Genesis time (March 24, 2016 for Steem mainnet)
#define STEEM_GENESIS_TIME (fc::time_point_sec(1458835200))

// No initial supply (mining-based distribution for original Steem)
#define STEEM_INIT_SUPPLY int64_t(0)

// Initial public key (fixed for mainnet)
#define STEEM_INIT_PUBLIC_KEY_STR "STM8GC13uCZbP44HzMLV6zPZGwVQ8Nt4Kji8PapsPiNq1BK153XTX"
```

**Important for Custom Chains**: If you're launching a new mainnet-based chain (not testnet), you should:
1. Modify `STEEM_CHAIN_ID_NAME` to make it unique
2. Consider setting `STEEM_INIT_SUPPLY` to a non-zero value if you want pre-mined tokens
3. Generate your own `STEEM_INIT_PUBLIC_KEY_STR` or use `STEEM_INIT_PRIVATE_KEY` approach

### Custom Chain ID

To create a unique blockchain, modify `STEEM_CHAIN_ID_NAME`:

```cpp
// In config.hpp
#define STEEM_CHAIN_ID_NAME "mychain-v1"
```

**Important**: Chain ID must be unique to prevent replay attacks between chains.

## Genesis Initialization Process

### Genesis State Creation

When `steemd` starts with an empty database, it automatically creates the genesis state via [`database::init_genesis()`](../libraries/chain/database.cpp):

#### 1. System Accounts

Four system accounts are created:

```cpp
// Miner rewards account
create< account_object >( [&]( account_object& a ) {
    a.name = "miners";  // STEEM_MINER_ACCOUNT
});

// Null account (for burning tokens)
create< account_object >( [&]( account_object& a ) {
    a.name = "null";  // STEEM_NULL_ACCOUNT
});

// Temporary account (for temporary operations)
create< account_object >( [&]( account_object& a ) {
    a.name = "temp";  // STEEM_TEMP_ACCOUNT
});
```

#### 2. Initial Miner Account

The genesis miner account receives the initial supply:

```cpp
create< account_object >( [&]( account_object& a ) {
    a.name = "initminer";  // STEEM_INIT_MINER_NAME
    a.memo_key = init_public_key;
    a.balance = asset(init_supply, STEEM_SYMBOL);  // All initial tokens
});

// Authorities (owner/active/posting)
create< account_authority_object >( [&]( account_authority_object& auth ) {
    auth.account = "initminer";
    auth.owner.add_authority(init_public_key, 1);
    auth.owner.weight_threshold = 1;
    auth.active = auth.owner;
    auth.posting = auth.active;
});
```

#### 3. Initial Witness

Genesis witness for block production:

```cpp
create< witness_object >( [&]( witness_object& w ) {
    w.owner = "initminer";
    w.signing_key = init_public_key;
    w.schedule = witness_object::miner;
});
```

#### 4. Global Properties

Blockchain starting state:

```cpp
create< dynamic_global_property_object >( [&]( dynamic_global_property_object& p ) {
    p.current_witness = "initminer";
    p.time = STEEM_GENESIS_TIME;
    p.recent_slots_filled = fc::uint128::max_value();
    p.participation_count = 128;
    p.current_supply = asset(init_supply, STEEM_SYMBOL);
    p.virtual_supply = p.current_supply;
    p.maximum_block_size = STEEM_MAX_BLOCK_SIZE;
});
```

#### 5. Initial Hardfork State

```cpp
create< hardfork_property_object >( [&](hardfork_property_object& hpo ) {
    hpo.processed_hardforks.push_back(STEEM_GENESIS_TIME);
});
```

#### 6. Reward Funds

```cpp
create< reward_fund_object >( [&](reward_fund_object& rfo ) {
    rfo.name = STEEM_POST_REWARD_FUND_NAME;
    rfo.last_update = head_block_time();
    rfo.content_constant = STEEM_CONTENT_CONSTANT_HF0;
    rfo.percent_curation_rewards = STEEM_1_PERCENT * 25;
    rfo.percent_content_rewards = STEEM_100_PERCENT;
    rfo.reward_balance = asset(0, STEEM_SYMBOL);
});
```

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
plugin = database_api witness_api condenser_api

# Network settings
p2p-endpoint = 0.0.0.0:2001
webserver-http-endpoint = 0.0.0.0:8090
webserver-ws-endpoint = 0.0.0.0:8090

# Witness configuration
enable-stale-production = true
required-participation = 0
witness = "initminer"

# Private key for testnet initminer
# Generated from sha256("init_key")
private-key = 5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n
```

**Getting the init private key**:

For testnet, the private key is deterministically generated:

```cpp
// In C++ code
fc::ecc::private_key init_key = fc::ecc::private_key::regenerate(
    fc::sha256::hash(std::string("init_key"))
);
std::string wif_key = graphene::utilities::key_to_wif(init_key);
// Result: 5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n
```

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
info  witness: Witness initminer starting block production
info  witness: Generated block #1 with timestamp 2016-01-01T00:00:03
```

### Step 4: Query Genesis State

Using `cli_wallet` or API calls:

```bash
# Connect to node
./programs/cli_wallet/cli_wallet -s ws://127.0.0.1:8090

# Check initminer account
>>> get_account initminer

# Check witness
>>> get_witness initminer

# Check blockchain info
>>> info
```

Expected output:

```json
{
  "head_block_num": 1,
  "head_block_id": "0000000109b3667c257e7171da61984da0d3279f",
  "time": "2016-01-01T00:00:03",
  "current_witness": "initminer",
  "total_supply": "250000000.000 STEEM",
  "current_supply": "250000000.000 STEEM"
}
```

## Adding Witnesses

### Create New Witness Account

```bash
# In cli_wallet
>>> create_account initminer "witness1" "" true

# Fund the account
>>> transfer initminer witness1 "1000.000 STEEM" "Initial funding" true

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
# Vote with initminer account
>>> vote_for_witness initminer witness1 true true
```

### Add to Config

Add witness to `config.ini`:

```ini
witness = "initminer"
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
plugin = database_api condenser_api
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
Genesis Node (initminer)
    ├── Port 2001: P2P
    ├── Port 8090: API
    └── Witness: initminer

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
| Initial distribution | initminer | Mining-based | How tokens enter circulation |

### Witness Parameters

| Parameter | Value | Description |
|-----------|-------|-------------|
| `STEEM_MAX_WITNESSES` | 21 | Maximum active witnesses |
| `STEEM_MAX_VOTED_WITNESSES_HF17` | 20 | Elected witnesses post-HF17 |
| `STEEM_MAX_RUNNER_WITNESSES_HF17` | 1 | Backup witnesses post-HF17 |
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
1. `witness = "initminer"` in config
2. `private-key` matches initminer's key
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

## Advanced Topics

### Custom Genesis Supply

Modify `STEEM_INIT_SUPPLY` in `config.hpp`:

```cpp
// 1 billion initial supply
#define STEEM_INIT_SUPPLY (int64_t(1000) * int64_t(1000000) * int64_t(1000))
```

Rebuild:
```bash
cd build
make -j$(nproc) steemd
```

### Multiple Init Miners

Increase `STEEM_NUM_INIT_MINERS`:

```cpp
#define STEEM_NUM_INIT_MINERS 21
```

Genesis will create `initminer`, `initminer1`, ..., `initminer20`.

### Custom Genesis Time

Set specific genesis timestamp:

```cpp
// January 1, 2025 00:00:00 UTC
#define STEEM_GENESIS_TIME (fc::time_point_sec(1735689600))
```

**Important**: Genesis time affects:
- Block timestamps
- Cashout windows
- Vesting periods
- All time-based protocol features

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
- [Config Reference](../contrib/testnet.config.ini)
- [Protocol Config](../libraries/protocol/include/steem/protocol/config.hpp)
- [Database Implementation](../libraries/chain/database.cpp)

## License

See [LICENSE.md](../LICENSE.md)
