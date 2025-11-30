# witness Plugin

Block production, witness scheduling, and bandwidth management for Steem blockchain witnesses.

## Overview

The `witness` plugin enables block production for elected witnesses and manages network bandwidth allocation. It implements the witness scheduling algorithm, block production timing, transaction validation, and rate limiting mechanisms.

**Plugin Type**: Core (Block Production)
**Dependencies**: [chain](chain.md), [p2p](p2p.md)
**Memory**: Moderate (~200MB)
**Default**: Disabled

## Purpose

- **Block Production**: Produce blocks according to witness schedule
- **Bandwidth Management**: Track and enforce bandwidth limits per account
- **Transaction Validation**: Validate transactions before inclusion
- **Network Reserve Ratio**: Dynamically adjust network capacity
- **Witness Scheduling**: Determine block production timing
- **Security Checks**: Prevent private key exposure and common mistakes

## Configuration

### Config File Options

```ini
# Enable witness plugin
plugin = witness

# Witness account name(s)
witness = "your-witness-name"
witness = "backup-witness-name"

# Private signing key(s) in WIF format
private-key = 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3

# Enable production even if chain is stale
enable-stale-production = false

# Required witness participation (0-99%)
required-participation = 33
```

### Command Line Options

```bash
steemd \
  --plugin=witness \
  --witness="your-witness-name" \
  --private-key=5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3 \
  --required-participation=33
```

### Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `witness` | (none) | Witness account name (multiple allowed) |
| `private-key` | (none) | WIF private key for signing blocks (multiple allowed) |
| `enable-stale-production` | false | Produce blocks even if chain appears stale |
| `required-participation` | 33 | Minimum witness participation % (0-99) |

## Block Production

### Production Schedule

**Steem uses a round-robin witness schedule**:
- 21 slots per round (63 seconds)
- Top 20 witnesses (by votes) produce 1 block each
- 1 slot for backup witness (randomly selected)
- 3-second block intervals

**Example schedule**:
```
Slot 1:  witness-1  (00:00)
Slot 2:  witness-2  (00:03)
Slot 3:  witness-3  (00:06)
...
Slot 20: witness-20 (00:57)
Slot 21: backup-witness (01:00)
Round repeats...
```

### Production Conditions

The plugin checks multiple conditions before producing a block:

#### 1. Synced
```
Condition: Node must be synced with network
Check: Next block time >= current time
Result: Wait until synced before enabling production
```

#### 2. Scheduled Time
```
Condition: Current time matches a block slot
Check: slot = get_slot_at_time(now)
Result: Only produce when scheduled
```

#### 3. Witness Turn
```
Condition: This witness is scheduled for current slot
Check: scheduled_witness in configured witnesses
Result: Only produce when it's your turn
```

#### 4. Private Key Available
```
Condition: Have private key for witness signing key
Check: signing_key in private_keys map
Result: Must have correct key to sign block
```

#### 5. Participation Threshold
```
Condition: Network participation above threshold
Check: participation_rate >= required_participation
Default: 33% (prevents minority fork)
Result: Stop production on network splits
```

#### 6. Timing Tolerance
```
Condition: Within 500ms of scheduled time
Check: abs(scheduled_time - now) <= 500ms
Result: Prevents late block production
```

### Production Loop

**Block production timing**:

```
Every second:
  1. Check if genesis time reached
  2. Evaluate production conditions
  3. If all conditions met:
     a. Generate block
     b. Sign with private key
     c. Broadcast to P2P network
  4. Log result
  5. Schedule next check
```

**Precision**:
- Wakes up at second boundaries
- 50ms minimum sleep time
- Sub-second timing accuracy
- Handles clock drift

## Bandwidth Management

### Bandwidth Types

**Two bandwidth pools per account**:

1. **Forum Bandwidth**: Regular transactions and operations
2. **Market Bandwidth**: Market operations (10x cost multiplier)

### Bandwidth Calculation

**Algorithm**:
```
new_bandwidth = old_bandwidth * (1 - elapsed/window) + transaction_size

available_bandwidth = (account_vests / total_vests) * max_virtual_bandwidth

has_bandwidth = available_bandwidth > new_bandwidth
```

**Parameters**:
- `window`: 7 days (STEEM_BANDWIDTH_AVERAGE_WINDOW_SECONDS)
- `precision`: STEEM_BANDWIDTH_PRECISION
- Exponential decay over window
- Proportional to vesting shares (Steem Power)

### Reserve Ratio

**Dynamic network capacity adjustment**:

```
Every 20 blocks (60 seconds):
  1. Calculate average block size
  2. Compare to target (25% of max)
  3. Adjust reserve ratio:
     - Above target: decrease ratio (reduce capacity)
     - Below target: increase ratio (increase capacity)
  4. Update max_virtual_bandwidth
```

**Purpose**:
- Automatically adjust to network usage
- Prevent spam while maximizing throughput
- React quickly to usage spikes
- Gradually increase capacity when underutilized

## Transaction Validation

### Pre-Transaction Checks

**Bandwidth enforcement**:
```cpp
For each signing authority:
  1. Calculate transaction size
  2. Update account bandwidth
  3. Check bandwidth limit
  4. Throw exception if exceeded
  5. Apply market multiplier if needed
```

### Operation Validation

**Security checks**:

1. **Comment Depth Limit**:
   - Maximum depth: STEEM_SOFT_MAX_COMMENT_DEPTH
   - Prevents deep comment chains
   - Protects against spam attacks

2. **Memo Field Validation**:
   - Detects private keys in memos
   - Checks against owner/active/posting/memo keys
   - Prevents accidental key exposure
   - Warns user to change keys

3. **Beneficiary Limits**:
   - Maximum 8 beneficiaries per comment
   - Prevents beneficiary spam
   - Ensures efficient processing

4. **Custom Operation Limits**:
   - One custom operation per account per block
   - Prevents custom_json spam
   - Applies to custom, custom_json, custom_binary

## Key Management

### Private Key Configuration

**WIF format** (Wallet Import Format):
```ini
# Single witness
private-key = 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3

# Multiple witnesses (different keys)
private-key = 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
private-key = 5JWcdkhSPdha3BvSNKvYVKKb82EKDFKnEGQDZpAGKmYGTdU4Sqz
```

### Key Security

**Best practices**:

1. **Dedicated Signing Key**:
   - Use separate key for block signing
   - Not your active or owner key
   - Set via `witness_update_operation`

2. **Key Storage**:
   - Store keys in config file (protected permissions)
   - Or use environment variables
   - Never commit keys to version control

3. **Key Rotation**:
   - Update signing key via witness_update
   - Update config with new private key
   - Restart witness node

4. **Backup Witnesses**:
   - Configure multiple witness accounts
   - Different keys for each
   - Automatic failover if scheduled

## Use Cases

### Active Witness

**Purpose**: Produce blocks as elected witness

```ini
# Core plugins
plugin = chain p2p witness

# Witness configuration
witness = "your-witness-name"
private-key = 5K...your-signing-key...

# Network participation threshold
required-participation = 33

# P2P for block broadcast
p2p-endpoint = 0.0.0.0:2001
p2p-seed-node = seed.steemit.com:2001

# Disable unnecessary APIs
# (witness doesn't need public API)
```

**System requirements**:
- Reliable internet connection (>10 Mbps upload)
- Low latency (<100ms to peers)
- 99.9%+ uptime
- NTP time synchronization

### Backup Witness

**Purpose**: Standby witness (lower ranking)

```ini
plugin = chain p2p witness

witness = "backup-witness"
private-key = 5K...backup-key...

# Same configuration as active witness
# Produces blocks when randomly selected
# ~1 block per 21 rounds (every ~63 seconds)
```

### Witness Failover

**Purpose**: Multiple witness accounts for redundancy

```ini
plugin = chain p2p witness

# Primary witness
witness = "primary-witness"
private-key = 5K...primary-key...

# Backup on same node
witness = "backup-witness"
private-key = 5K...backup-key...

# Node produces for whichever is scheduled
# Automatic failover if one disabled
```

### Development/Testnet Witness

**Purpose**: Block production on private testnet

```ini
plugin = chain p2p witness

witness = "initwitness"
private-key = 5K...testnet-key...

# Enable stale production for testnet
enable-stale-production = true

# Lower participation requirement
required-participation = 0

# Private network
p2p-seed-node = 10.0.1.100:2001
```

## Monitoring

### Production Status

**Check block production**:
```bash
# Watch logs for production
tail -f logs/stderr.txt | grep "Generated block"

# Example output:
# Generated block #12345678 with timestamp 2023-01-15T12:34:56 at time 2023-01-15T12:34:56.123
```

### Production Conditions

**Monitor production conditions**:
```
# Successful production
Generated block #N with timestamp T at time C

# Not synced
(no message - waiting for sync)

# Not your turn
(no message - normal)

# Missing private key
Not producing block because I don't have the private key for STM...

# Low participation
Not producing block because node appears to be on a minority fork with only X% witness participation

# Timing lag
Not producing block because node didn't wake up within 500ms of the slot time

# Consecutive blocks
Not producing block because the last block was generated by the same witness
```

### Missed Blocks

**Check witness stats**:
```bash
# Via API
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"database_api.find_witnesses",
  "params":{"owners":["your-witness-name"]}
}' | jq '.result.witnesses[0] | {
  total_missed: .total_missed,
  last_confirmed_block_num: .last_confirmed_block_num
}'
```

### Bandwidth Statistics

**Export bandwidth data**:
```ini
# Enable bandwidth export
plugin = witness block_data_export

block-data-export-file = /var/log/steem/bandwidth.log

# Data includes:
# - Reserve ratio updates
# - Bandwidth updates per account
# - Transaction sizes
```

## Troubleshooting

### Not Producing Blocks

**Problem**: Witness not producing despite being scheduled

**Check**:
```bash
# 1. Verify witness is scheduled
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "method":"database_api.get_witness_schedule"
}' | jq .

# 2. Check witness is in your config
grep "witness =" config.ini

# 3. Verify private key matches
# (public key derived from private key must match witness signing_key)

# 4. Check logs for production conditions
tail -f logs/stderr.txt | grep -E "(Generated|Not producing)"
```

**Solutions**:
1. Verify `witness = "name"` in config (quotes required)
2. Ensure private key matches witness signing key
3. Check `enable-stale-production` if testing
4. Verify network participation above threshold
5. Check NTP time synchronization: `ntpq -p`

### Private Key Mismatch

**Problem**: "I don't have the private key for..."

**Cause**: Private key doesn't match witness signing key

**Solution**:
```bash
# 1. Get witness signing key
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "method":"database_api.find_witnesses",
  "params":{"owners":["your-witness"]}
}' | jq -r '.result.witnesses[0].signing_key'

# 2. Derive public key from private key
# (use cli_wallet or key utility)

# 3. Update witness signing key if needed
# (via witness_update_operation)

# 4. Or update private-key in config to match
```

### Low Participation

**Problem**: "minority fork with only X% participation"

**Cause**: Network split or low witness participation

**Check**:
```bash
# Check participation rate
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "method":"database_api.get_witness_schedule"
}' | jq -r '.result | {
  participation: (.participation_count / 128 * 100),
  recent_slots: .recent_slots_filled
}'
```

**Solutions**:
1. Check P2P connectivity to witness network
2. Verify majority of witnesses are online
3. Lower `required-participation` temporarily (not recommended)
4. Check for network fork/consensus issues

### Bandwidth Errors

**Problem**: Users reporting "bandwidth limit exceeded"

**Cause**: Account vesting shares too low for transaction volume

**Check**:
```bash
# Via account bandwidth object (if exported)
# Or check user's vesting shares

curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "method":"database_api.find_accounts",
  "params":{"accounts":["username"]}
}' | jq '.result.accounts[0].vesting_shares'
```

**Solutions**:
1. User should power up more STEEM (increase vesting shares)
2. Wait for bandwidth to regenerate (7-day window)
3. Reduce transaction frequency
4. Check not hitting market bandwidth (10x multiplier)

### Clock Drift

**Problem**: "didn't wake up within 500ms of slot time"

**Cause**: System clock drift or CPU lag

**Solutions**:
```bash
# 1. Enable NTP
sudo systemctl enable ntp
sudo systemctl start ntp

# 2. Force time sync
sudo ntpdate -s time.nist.gov

# 3. Check CPU load
top
# If high load, consider hardware upgrade

# 4. Verify NTP peers
ntpq -p
```

## Performance Tuning

### Witness Node Optimization

**High-performance configuration**:

```ini
# Consensus-only plugins
plugin = chain p2p witness

# No public API (reduces load)
# webserver plugin disabled

# Optimize P2P
p2p-max-connections = 50
p2p-endpoint = 0.0.0.0:2001

# Connect to trusted witnesses
p2p-seed-node = witness1.example.com:2001
p2p-seed-node = witness2.example.com:2001

# Dedicated hardware
# - SSD for blockchain data
# - 16GB+ RAM
# - Multi-core CPU
# - Reliable network
```

### Network Configuration

**Optimal network setup**:
- Direct fiber connection (no WiFi)
- Multiple upstream providers (redundancy)
- Low latency to other witnesses (<50ms)
- Sufficient upload bandwidth (10+ Mbps)
- DDoS protection
- UPS backup power

### System Monitoring

**Critical metrics**:
- Block production success rate (>99%)
- Missed block count (minimize)
- Network latency to peers (<100ms)
- System time accuracy (<10ms drift)
- CPU usage (<50%)
- Memory usage (<80%)
- Disk I/O (not saturated)

## Security Considerations

### Key Protection

**Protect signing keys**:
```bash
# Restrict config file permissions
chmod 600 config.ini
chown steem:steem config.ini

# Use dedicated user
sudo -u steem steemd

# Consider hardware security module (HSM)
# for high-value witnesses
```

### Network Security

**Firewall configuration**:
```bash
# Allow P2P only
ufw allow 2001/tcp

# Restrict to known witnesses (optional)
ufw allow from witness-ip to any port 2001

# No public API exposure
# (run API nodes separately)
```

### Operational Security

**Best practices**:
1. Separate witness and API nodes
2. No public RPC on witness node
3. Monitor missed blocks (alert on increase)
4. Redundant internet connections
5. Regular security updates
6. Intrusion detection system
7. Log monitoring and alerting

### Disaster Recovery

**Backup procedures**:
```bash
# 1. Config backup
cp config.ini config.ini.backup

# 2. Private key backup (encrypted)
gpg -c config.ini

# 3. Witness failover plan
# - Backup witness node
# - Different datacenter
# - Same witness account and key
# - Quick activation procedure

# 4. Communication plan
# - Notify community if extended downtime
# - Update price feed if also running that
```

## Advanced Topics

### Block Production Skip Flags

**Development/testing only**:
```cpp
// Internal flags (not configurable)
skip_nothing = 0
skip_block_signature = 1
skip_merkle_check = 2
skip_transaction_signatures = 4
// ... (see database.hpp)
```

**Default**: `skip_nothing` (full validation)

### Write Lock Optimization

**When producing blocks**:
```cpp
// Set unlimited write lock hold time
chain_plugin.set_write_lock_hold_time(-1);
```

**Purpose**: Prevent write lock timeout during block production

### Reserve Ratio Algorithm

**Detailed calculation**:
```cpp
Every 20 blocks:
  avg_block_size = (99 * old_avg + current_size) / 100
  target = max_block_size / 4  // 25% target
  distance = (avg - target) * PRECISION / target

  if (distance > 0):  // Above target
    reserve_ratio -= reserve_ratio * distance / (distance + PRECISION)
    reserve_ratio = max(reserve_ratio, MIN_RATIO)
  else:  // Below target
    reserve_ratio += max(MIN_INCREMENT, reserve_ratio * distance / (distance - PRECISION))
    reserve_ratio = min(reserve_ratio, MAX_RATIO)

  max_bandwidth = max_block_size * reserve_ratio * window / interval
```

## Related Documentation

- [chain Plugin](chain.md) - Core blockchain database
- [p2p Plugin](p2p.md) - Network connectivity
- [Witness Guide](../operations/witness-guide.md) - Becoming a witness
- [Hardfork Procedure](../operations/hardfork-procedure.md) - Network upgrades

## Source Code

- **Implementation**: [src/plugins/witness/witness_plugin.cpp](../../src/plugins/witness/witness_plugin.cpp)
- **Header**: [src/plugins/witness/include/steem/plugins/witness/witness_plugin.hpp](../../src/plugins/witness/include/steem/plugins/witness/witness_plugin.hpp)
- **Objects**: [src/plugins/witness/include/steem/plugins/witness/witness_objects.hpp](../../src/plugins/witness/include/steem/plugins/witness/witness_objects.hpp)

## License

See [LICENSE.md](../../LICENSE.md)
