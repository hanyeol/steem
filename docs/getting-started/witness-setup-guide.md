# Witness Node Setup Guide

Complete guide to setting up a new witness node and connecting it to an existing Steem blockchain network.

## Overview

This guide covers:
- Building and configuring a witness node
- Connecting to an existing blockchain network
- Registering as a witness
- Best practices for witness operations

**Prerequisites**:
- Steem blockchain is already running (mainnet or private network)
- Access to seed node addresses for P2P connection
- Witness account created on the blockchain
- Witness signing key pair generated

## Architecture Overview

```
Existing Network          Your New Witness Node
┌───────────────┐         ┌──────────────────┐
│  Seed Nodes   │◄────────┤   P2P Plugin     │
│  (P2P sync)   │         │                  │
└───────────────┘         │   Chain Plugin   │
                          │  (block storage) │
┌───────────────┐         │                  │
│ Other Witness │◄────────┤ Witness Plugin   │
│    Nodes      │         │ (block signing)  │
└───────────────┘         └──────────────────┘
```

## Step 1: Build Witness Node

### Standard Build

Build `steemd` binary for witness operation (do NOT use `LOW_MEMORY_NODE` or testnet builds):

```bash
cd /path/to/steem
git submodule update --init --recursive

# Create build directory
mkdir -p build && cd build

# Configure build for production witness
cmake -DCMAKE_BUILD_TYPE=Release \
      -DLOW_MEMORY_NODE=OFF \
      -DBUILD_STEEM_TESTNET=OFF \
      ..

# Build steemd
make -j$(nproc) steemd

# Verify binary
./programs/steemd/steemd --version
```

### Build Notes

- **DO NOT use** `LOW_MEMORY_NODE=ON` - witnesses need full consensus validation
- **DO NOT use** `BUILD_STEEM_TESTNET=ON` unless connecting to testnet
- Use `Release` build type for optimal performance
- `steemd` binary location: `build/programs/steemd/steemd`

## Step 2: Configure Witness Node

### Create Data Directory

```bash
# Create data directory
mkdir -p ~/witness_node_data_dir

# Create subdirectories
mkdir -p ~/witness_node_data_dir/blockchain
mkdir -p ~/witness_node_data_dir/logs
```

### Generate Default Configuration

```bash
# Generate default config.ini
./programs/steemd/steemd --data-dir=~/witness_node_data_dir --dump-config > ~/witness_node_data_dir/config.ini
```

### Edit Configuration File

Edit `~/witness_node_data_dir/config.ini` with the following settings:

```ini
# ==================== Logging Configuration ====================

log-console-appender = {"appender":"stderr","stream":"std_error"}
log-logger = {"name":"default","level":"info","appender":"stderr"}
log-logger = {"name":"p2p","level":"warn","appender":"stderr"}

# ==================== Plugin Configuration ====================

# Essential plugins for witness node
plugin = chain p2p webserver witness

# Minimal API plugins (recommended for security)
plugin = database_api witness_api

# Optional: Add if you need transaction broadcasting
# plugin = network_broadcast_api

# ==================== Storage Configuration ====================

# Shared memory file location
shared-file-dir = blockchain

# Shared memory size (54GB recommended for witness)
shared-file-size = 54G

# Flush interval (0 = flush every block, recommended for witnesses)
flush-state-interval = 0

# ==================== Network Configuration ====================

# P2P endpoint - listening for peer connections
# Format: IP:PORT (0.0.0.0 = listen on all interfaces)
p2p-endpoint = 0.0.0.0:2001

# Seed nodes - REPLACE with your network's seed nodes
p2p-seed-node = seed1.example.com:2001
p2p-seed-node = seed2.example.com:2001
p2p-seed-node = 192.168.1.100:2001

# Maximum P2P connections (10-30 recommended for witness)
p2p-max-connections = 20

# ==================== Witness Configuration ====================

# Enable block production even if chain is stale (FALSE for production)
enable-stale-production = false

# Minimum witness participation (33% recommended)
required-participation = 33

# Witness account name - REPLACE with your witness account
witness = "your-witness-account"

# Witness signing private key - REPLACE with your private key
# IMPORTANT: Keep this key secure! Use separate signing key, not active key
private-key = 5K... # Your witness signing key (WIF format)

# ==================== API Configuration (Optional) ====================

# WebServer HTTP endpoint (comment out to disable API)
# webserver-http-endpoint = 127.0.0.1:8090

# WebServer WebSocket endpoint (comment out to disable)
# webserver-ws-endpoint = 127.0.0.1:8090

# ==================== Performance Tuning ====================

# Use all available cores for signature verification
# (auto-detected if not specified)
# webserver-thread-pool-size = 4
```

### Configuration Notes

**Critical Settings**:
- `p2p-seed-node`: Add at least 2-3 reliable seed nodes from your network
- `witness`: Your registered witness account name
- `private-key`: Witness signing key (NOT your active/owner key)
- `enable-stale-production`: Must be `false` for production
- `flush-state-interval`: Set to `0` for witnesses (flush every block)

**Security Best Practices**:
- Disable API endpoints (`webserver-http-endpoint`, `webserver-ws-endpoint`) for production witnesses
- Firewall all ports except P2P port (2001)
- Use a dedicated signing key separate from account keys
- Store config.ini with restrictive permissions: `chmod 600 config.ini`

## Step 3: Obtain Seed Node Information

### From Network Operators

Contact network administrators to obtain:
- Seed node addresses (IP:port or domain:port)
- Network ID/chain ID (for verification)
- Genesis block hash (for verification)

### From Existing Nodes

If you have access to an existing node:

```bash
# Check network info
curl -X POST http://existing-node:8090 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_config"}' \
  | jq -r '.result.GRAPHENE_CHAIN_ID'

# Check active peers
curl -X POST http://existing-node:8090 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"network_broadcast_api.get_info"}' \
  | jq
```

### Public Mainnet Seeds

For Steem mainnet, use public seed nodes:

```ini
p2p-seed-node = seed.steemnodes.com:2001
p2p-seed-node = seed.steem.com:2001
p2p-seed-node = seed1.blockbrothers.io:2001
```

## Step 4: Initial Blockchain Sync

### Start Node for First Time

```bash
# Start steemd (will sync from P2P network)
./programs/steemd/steemd --data-dir=~/witness_node_data_dir
```

### Sync Progress

Initial sync downloads blockchain from seed nodes. Monitor progress:

```bash
# Watch sync status (in another terminal)
watch -n 5 'curl -s -X POST http://localhost:8090 \
  -H "Content-Type: application/json" \
  -d "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"database_api.get_dynamic_global_properties\"}" \
  | jq -r ".result | \"\(.head_block_number) / \(.time)\""'
```

**Sync Time Estimates**:
- Small network (<10K blocks): Minutes to hours
- Steem mainnet (>50M blocks): Days to weeks depending on network speed

### Monitor Logs

```bash
# Tail logs
tail -f ~/witness_node_data_dir/logs/steemd.log

# Look for:
# - "Syncing blockchain" messages
# - P2P connection status
# - Block number increases
```

### Verify Sync Completion

Node is synced when:
1. Block number matches network head block
2. Block timestamp is recent (within 3-5 seconds)
3. Logs show "Syncing blockchain" stops

```bash
# Check if synced
curl -s -X POST http://localhost:8090 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_dynamic_global_properties"}' \
  | jq -r '.result | "Block: \(.head_block_number)\nTime: \(.time)"'
```

## Step 5: Register as Witness

### Prerequisites

Before registering:
- Witness node is fully synced
- Witness account exists on blockchain
- Signing key pair generated

### Generate Signing Key

**IMPORTANT**: The witness signing key is **separate from your account keys** (owner/active/posting). This key is dedicated solely to signing blocks.

**Why separate?**
- Account active key: Used for witness registration and settings (keep offline when possible)
- Signing key: Used 24/7 for block signing (stored in config.ini on the server)
- Security: If signing key is compromised, account funds are safe

If you don't have a signing key yet:

```bash
# Using cli_wallet
./programs/cli_wallet/cli_wallet --server-rpc-endpoint=ws://localhost:8090

# In cli_wallet prompt - Generate NEW signing key (NOT your account key!)
>>> suggest_brain_key
{
  "brain_priv_key": "SOME LONG BRAIN KEY PHRASE...",
  "wif_priv_key": "5K...",  # ← This goes in config.ini (private-key)
  "pub_key": "STM..."       # ← This goes in update_witness call
}

# Save both keys securely (SEPARATE from your account keys!)
# Exit cli_wallet
>>> exit
```

### Register Witness on Blockchain

Use `cli_wallet` to broadcast witness registration:

```bash
# Start cli_wallet connected to a full node with database_api and network_broadcast_api
./programs/cli_wallet/cli_wallet --server-rpc-endpoint=ws://full-node:8090

# Unlock wallet
>>> set_password "your-wallet-password"
>>> unlock "your-wallet-password"

# Import your witness account ACTIVE key (NOT the signing key!)
# This is needed to authorize the update_witness operation
>>> import_key your-witness-account 5K...active-private-key...

# Register witness (or update existing witness)
# Note: "STM...public-signing-key..." is the PUBLIC key from suggest_brain_key above
>>> update_witness "your-witness-account" "https://yourwebsite.com" "STM...public-signing-key..." {"account_creation_fee":"3.000 STEEM","maximum_block_size":65536,"sbd_interest_rate":1000} true

# Verify registration
>>> get_witness "your-witness-account"
```

### Update config.ini with Signing Key

After registration, add the signing private key to your witness node's `config.ini`:

```ini
witness = "your-witness-account"
private-key = 5K...wif-priv-key-from-suggest_brain_key...
```

### Restart Witness Node

```bash
# Stop node (Ctrl+C or kill process)
pkill -INT steemd

# Restart with witness configuration
./programs/steemd/steemd --data-dir=~/witness_node_data_dir
```

## Step 6: Verify Witness Operation

### Check Witness Status

```bash
# Query witness object
curl -s -X POST http://localhost:8090 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_witness","params":{"account":"your-witness-account"}}' \
  | jq

# Expected fields:
# - "signing_key": Should match your public key
# - "votes": Total votes for your witness
# - "total_missed": Missed blocks count
# - "last_confirmed_block_num": Last produced block
```

### Monitor Block Production

Your witness will produce blocks when:
1. Your witness is in the active witness set (top 21 + backup witnesses scheduled)
2. Your turn arrives in the witness schedule
3. Node is synced and running

```bash
# Watch for block production in logs
tail -f ~/witness_node_data_dir/logs/steemd.log | grep "Generated block"

# You should see:
# Generated block #12345678 with timestamp 2024-01-15T12:00:00 by your-witness-account
```

### Check Witness Schedule

```bash
# Get current witness schedule
curl -s -X POST http://localhost:8090 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_witness_schedule"}' \
  | jq -r '.result.current_shuffled_witnesses'

# If your witness account appears in the list, you'll produce blocks soon
```

## Step 7: Production Deployment

### Run as System Service (Linux)

Create systemd service file `/etc/systemd/system/steemd.service`:

```ini
[Unit]
Description=Steem Witness Node
After=network.target

[Service]
Type=simple
User=steem
Group=steem
WorkingDirectory=/home/steem/steem
ExecStart=/home/steem/steem/build/programs/steemd/steemd --data-dir=/home/steem/witness_node_data_dir
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal
SyslogIdentifier=steemd

# Security settings
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=full
ProtectHome=true

# Resource limits
LimitNOFILE=65536
LimitNPROC=4096

[Install]
WantedBy=multi-user.target
```

Enable and start service:

```bash
# Reload systemd
sudo systemctl daemon-reload

# Enable service (start on boot)
sudo systemctl enable steemd

# Start service
sudo systemctl start steemd

# Check status
sudo systemctl status steemd

# View logs
sudo journalctl -u steemd -f
```

### Backup Configuration

Create a backup witness on separate infrastructure:

```bash
# Copy config.ini to backup server
scp ~/witness_node_data_dir/config.ini backup-server:~/witness_node_data_dir/

# Sync blockchain data (optional - can sync from P2P)
rsync -avz --progress ~/witness_node_data_dir/blockchain/ \
  backup-server:~/witness_node_data_dir/blockchain/

# Start backup witness (same config, different server)
# Only ONE instance should have witness plugin enabled at a time
```

### Firewall Configuration

```bash
# Allow P2P port (2001)
sudo ufw allow 2001/tcp

# Block API ports (if enabled in config)
sudo ufw deny 8090/tcp

# Enable firewall
sudo ufw enable
sudo ufw status
```

## Troubleshooting

### Node Not Syncing

**Symptoms**: Block number not increasing, no P2P connections

**Solutions**:
```bash
# Check P2P connections
grep "p2p" ~/witness_node_data_dir/logs/steemd.log

# Verify seed nodes are reachable
nc -zv seed1.example.com 2001

# Try different seed nodes
# Edit config.ini and add more p2p-seed-node entries

# Check firewall
sudo ufw status
```

### Witness Not Producing Blocks

**Symptoms**: Witness registered but no blocks produced

**Checklist**:
1. **Is witness in active set?**
   ```bash
   # Check witness schedule
   curl -X POST http://localhost:8090 -H "Content-Type: application/json" \
     -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_witness_schedule"}' \
     | jq -r '.result.current_shuffled_witnesses' | grep "your-witness-account"
   ```

2. **Is signing key correct?**
   ```bash
   # Verify signing key in witness object matches config.ini
   curl -X POST http://localhost:8090 -H "Content-Type: application/json" \
     -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_witness","params":{"account":"your-witness-account"}}' \
     | jq -r '.result.signing_key'
   ```

3. **Is node fully synced?**
   ```bash
   # Check block timestamp is recent
   curl -X POST http://localhost:8090 -H "Content-Type: application/json" \
     -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_dynamic_global_properties"}' \
     | jq -r '.result.time'
   ```

4. **Is witness plugin enabled?**
   ```bash
   grep "plugin = witness" ~/witness_node_data_dir/config.ini
   ```

### High Memory Usage

**Symptoms**: OOM kills, swap usage

**Solutions**:
```bash
# Check shared memory size
ls -lh ~/witness_node_data_dir/blockchain/shared_memory.bin

# Reduce shared-file-size if too large (54G recommended for witness)
# Edit config.ini:
# shared-file-size = 54G

# Restart node with --replay-blockchain to rebuild shared memory
./programs/steemd/steemd --data-dir=~/witness_node_data_dir --replay-blockchain
```

### Missed Blocks

**Symptoms**: `total_missed` increasing in witness object

**Common Causes**:
- Network latency (slow P2P connection)
- Node not synced
- Clock skew (server time incorrect)
- Insufficient CPU/disk I/O

**Solutions**:
```bash
# Check system time sync
timedatectl status

# Enable NTP if not synced
sudo timedatectl set-ntp true

# Check disk I/O (should use SSD)
iostat -x 5

# Check P2P latency
grep "p2p" ~/witness_node_data_dir/logs/steemd.log | grep "latency"

# Restart node if out of sync
sudo systemctl restart steemd
```

## Best Practices

### Security

1. **Signing Key Isolation**
   - Generate dedicated signing key (do NOT use active/owner keys)
   - Store signing key only in config.ini on witness server
   - Use hardware wallet/HSM for active/owner keys

2. **Network Security**
   - Firewall all ports except P2P (2001)
   - Disable API endpoints in production witness
   - Use VPN for remote access
   - DDoS protection for P2P endpoint

3. **Access Control**
   - Restrict SSH access (key-based auth only)
   - Use separate user account for steemd process
   - `chmod 600` on config.ini (contains private key)

### Reliability

1. **Redundancy**
   - Run primary + backup witness on separate infrastructure
   - Only enable witness plugin on ONE instance at a time
   - Automatic failover via monitoring scripts

2. **Monitoring**
   - Alert on missed blocks
   - Monitor block production time
   - Track P2P connection count
   - Monitor system resources (CPU, RAM, disk)

3. **Maintenance**
   - Schedule upgrades during low-traffic periods
   - Test upgrades on backup witness first
   - Keep block log backups before major upgrades

### Performance

1. **Hardware**
   - SSD storage (NVMe preferred)
   - 64GB+ RAM
   - 4+ CPU cores
   - Low-latency network connection

2. **System Tuning**
   - Disable swap (use RAM only)
   - Increase file descriptor limits
   - Use deadline/noop I/O scheduler for SSDs

3. **Linux VM Tuning** (for initial sync/replay)
   ```bash
   echo    75 | sudo tee /proc/sys/vm/dirty_background_ratio
   echo  1000 | sudo tee /proc/sys/vm/dirty_expire_centisecs
   echo    80 | sudo tee /proc/sys/vm/dirty_ratio
   echo 30000 | sudo tee /proc/sys/vm/dirty_writeback_centisecs
   ```

### Price Feed Updates

Top 20 witnesses should publish SBD price feeds regularly:

```bash
# Using cli_wallet
>>> publish_feed "your-witness-account" {"base":"1.000 SBD","quote":"0.500 STEEM"} true
```

Set up automated script to update price feeds from external sources (e.g., CoinGecko, CMC).

## Monitoring and Metrics

### Essential Metrics

Monitor these metrics for witness health:

```bash
# 1. Missed blocks (should stay constant)
curl -s -X POST http://localhost:8090 -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_witness","params":{"account":"your-witness-account"}}' \
  | jq -r '.result.total_missed'

# 2. Block production
tail -f ~/witness_node_data_dir/logs/steemd.log | grep "Generated block"

# 3. P2P connections (should have 5-20 active peers)
grep "active peers" ~/witness_node_data_dir/logs/steemd.log | tail -1

# 4. Sync status (head_block_age should be <3 seconds)
curl -s -X POST http://localhost:8090 -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_dynamic_global_properties"}' \
  | jq -r '.result | "Block: \(.head_block_number)\nTime: \(.time)"'
```

### Alerting Setup

Create monitoring script `~/witness_monitor.sh`:

```bash
#!/bin/bash

WITNESS_ACCOUNT="your-witness-account"
ALERT_EMAIL="your-email@example.com"

# Check missed blocks
MISSED=$(curl -s -X POST http://localhost:8090 -H "Content-Type: application/json" \
  -d "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"database_api.get_witness\",\"params\":{\"account\":\"$WITNESS_ACCOUNT\"}}" \
  | jq -r '.result.total_missed')

MISSED_LAST=$(cat /tmp/witness_missed_last 2>/dev/null || echo 0)

if [ "$MISSED" -gt "$MISSED_LAST" ]; then
  echo "WARNING: Witness missed block! Total: $MISSED" | mail -s "Witness Alert" $ALERT_EMAIL
fi

echo $MISSED > /tmp/witness_missed_last

# Check sync status
BLOCK_TIME=$(curl -s -X POST http://localhost:8090 -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_dynamic_global_properties"}' \
  | jq -r '.result.time')

CURRENT_TIME=$(date -u +%s)
BLOCK_TIMESTAMP=$(date -d "$BLOCK_TIME" +%s)
BLOCK_AGE=$((CURRENT_TIME - BLOCK_TIMESTAMP))

if [ "$BLOCK_AGE" -gt 60 ]; then
  echo "WARNING: Node out of sync! Block age: ${BLOCK_AGE}s" | mail -s "Witness Alert" $ALERT_EMAIL
fi
```

Add to crontab:

```bash
# Run monitoring every 5 minutes
crontab -e
*/5 * * * * /home/steem/witness_monitor.sh
```

## Additional Resources

- [Node Modes Guide](node-modes-guide.md) - Different node configurations
- [Building Steem](building.md) - Detailed build instructions
- [CLI Wallet Guide](cli-wallet-guide.md) - Wallet usage
- [Hardfork Procedure](../operations/hardfork-procedure.md) - Upgrade procedures
- [DDoS Protection](../operations/ddos-protection.md) - Network security

## Getting Help

- GitHub Issues: [https://github.com/hanyeol/steem/issues](https://github.com/hanyeol/steem/issues)
- Developer Documentation: [docs/development/](../development/)
- Community Forums: Steem witness channels

## License

See [LICENSE.md](../../LICENSE.md)
