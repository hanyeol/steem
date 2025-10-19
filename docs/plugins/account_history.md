# account_history Plugin

Memory-based plugin that tracks and indexes all operations affecting specific accounts for fast history queries.

## Overview

The `account_history` plugin maintains a complete history of all operations that impact specified accounts. It stores this data in memory (shared memory file) for fast retrieval, making it essential for wallets, block explorers, and applications that need to display account activity.

**Plugin Type**: State (Tracking)
**Dependencies**: [chain](chain.md)
**Memory**: High (10-50GB+ for full history)
**Default**: Disabled (enable explicitly)
**Alternative**: [account_history_rocksdb](account_history_rocksdb.md) (disk-based)

## Purpose

- **Account Activity Tracking**: Record all operations affecting accounts
- **Transaction History**: Provide complete transaction logs per account
- **Wallet Support**: Enable wallet interfaces to display history
- **Block Explorer Backend**: Power account history pages
- **Audit Trail**: Maintain comprehensive account activity records
- **API Queries**: Support `get_account_history` API calls

## How It Works

### Operation Tracking

The plugin monitors all operations and identifies which accounts are "impacted":

**Impacted Account Detection**:
- Direct participants (from/to in transfers)
- Voters and content creators
- Witnesses and delegators
- Any account referenced in operation fields

**Tracked Operations** (configurable):
- Financial: `transfer`, `transfer_to_vesting`, `withdraw_vesting`
- Content: `comment`, `vote`, `delete_comment`
- Account: `account_create`, `account_update`
- Witness: `witness_update`, `account_witness_vote`
- Market: `limit_order_create`, `fill_order`
- And many more...

### Storage Structure

**Two-level indexing**:

1. **operation_object**: Stores operation details
   - Transaction ID
   - Block number
   - Operation type and data
   - Timestamp

2. **account_history_object**: Maps accounts to operations
   - Account name
   - Sequence number (per-account counter)
   - Reference to operation_object

## Configuration

### Config File Options

```ini
# Enable the plugin
plugin = account_history

# Track all accounts (default if not specified)
# Or track specific account ranges:
account-history-track-account-range = ["alice", "alice"]
account-history-track-account-range = ["bob", "carol"]

# Whitelist specific operations (only track these)
account-history-whitelist-ops = transfer vote comment

# OR blacklist operations (track all except these)
account-history-blacklist-ops = custom_json follow_operation

# Disable automatic pruning (keep all history)
history-disable-pruning = true
```

### Command Line Options

```bash
# Basic usage - track all accounts
steemd --plugin=account_history

# Track specific account range
steemd --plugin=account_history \
  --account-history-track-account-range='["alice","alice"]' \
  --account-history-track-account-range='["bob","charlie"]'

# Whitelist operations
steemd --plugin=account_history \
  --account-history-whitelist-ops='transfer vote comment'

# Disable pruning
steemd --plugin=account_history \
  --history-disable-pruning=true
```

### Configuration Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `account-history-track-account-range` | array | (all) | Account ranges to track as ["from","to"] pairs |
| `account-history-whitelist-ops` | list | (all) | Only track these operation types |
| `account-history-blacklist-ops` | list | (none) | Ignore these operation types |
| `history-disable-pruning` | bool | false | Keep all history (vs 30 days/items) |

**Note**: `track-account-range`, `history-whitelist-ops`, and `history-blacklist-ops` are deprecated aliases.

## Account Tracking

### Track All Accounts

**Default behavior** - no account range specified:

```ini
plugin = account_history
# Tracks ALL accounts on the blockchain
```

**Memory Impact**: Very high (20-50GB+)
**Use Case**: Full node, block explorer

### Track Specific Accounts

**Single account**:
```ini
account-history-track-account-range = ["alice", "alice"]
```

**Account range** (inclusive):
```ini
account-history-track-account-range = ["alice", "bob"]
# Tracks: alice, alice1, alice2, ..., bob, bob1, etc.
```

**Multiple ranges**:
```ini
account-history-track-account-range = ["alice", "alice"]
account-history-track-account-range = ["bob", "bob"]
account-history-track-account-range = ["charlie", "david"]
```

**Memory Impact**: Proportional to tracked accounts
**Use Case**: Exchange (own accounts), wallet service

### Range Format

Ranges use **lexicographical ordering**:

```
["a", "c"] includes: a, aa, ab, b, ba, bb, c
["alice", "alice"] includes: alice only
["alice", "alicez"] includes: alice, alice1, alice2, ..., alicez
```

## Operation Filtering

### Whitelist Mode

**Only track specified operations**:

```ini
# Only track transfers and votes
account-history-whitelist-ops = transfer vote
account-history-whitelist-ops = transfer_to_vesting withdraw_vesting
```

**Common Whitelist Examples**:

**Financial operations only**:
```ini
account-history-whitelist-ops = transfer transfer_to_vesting withdraw_vesting \
  claim_reward_balance transfer_to_savings transfer_from_savings
```

**Content operations only**:
```ini
account-history-whitelist-ops = comment vote delete_comment comment_options
```

### Blacklist Mode

**Track all EXCEPT specified operations**:

```ini
# Track everything except custom_json
account-history-blacklist-ops = custom_json
```

**Common Blacklist Examples**:

**Exclude high-volume operations**:
```ini
account-history-blacklist-ops = custom_json follow_operation reblog_operation
```

### Operation Names

Use the C++ type name without `_operation` suffix:

```
transfer              ✓ Correct
transfer_operation    ✗ Wrong
```

**Common Operation Names**:
- `transfer`, `transfer_to_vesting`, `withdraw_vesting`
- `comment`, `vote`, `delete_comment`
- `account_create`, `account_update`, `account_witness_vote`
- `custom_json`, `custom_binary`
- `witness_update`, `feed_publish`
- `limit_order_create`, `limit_order_cancel`

**Full list**: See [steem_operations.hpp](../../libraries/protocol/include/steem/protocol/steem_operations.hpp)

## History Pruning

### Automatic Pruning (Default)

**Retention Policy** (when pruning enabled):
- Keep last **30 operations** per account
- Keep last **30 days** of history
- Whichever is **MORE** (union, not intersection)

**Example**:
- Account with 100 ops/day: Keeps ~30 days (3000 operations)
- Account with 1 op/month: Keeps 30 operations (~2.5 years)

### Disable Pruning

**Keep all history**:
```ini
history-disable-pruning = true
```

**Memory Impact**: Grows unbounded with blockchain history
**Disk Impact**: Shared memory file grows continuously

**Use Cases**:
- Block explorers (need full history)
- Compliance/audit nodes
- Archive nodes

## Memory Requirements

### Estimation Formula

**Per-account, per-operation**:
```
Memory per entry ≈ 200-500 bytes
```

**Example Calculations**:

**Track 1 account, 1000 operations**:
```
1000 ops × 300 bytes = 300 KB
```

**Track 100 accounts, avg 10,000 operations each**:
```
100 × 10,000 × 300 bytes = 300 MB
```

**Track all accounts (full node)**:
```
Millions of accounts × thousands of ops = 20-50+ GB
```

### Memory Optimization

**Reduce memory usage**:

1. **Track fewer accounts**:
   ```ini
   account-history-track-account-range = ["myexchange", "myexchange"]
   ```

2. **Enable pruning** (default):
   ```ini
   # Remove line: history-disable-pruning = true
   ```

3. **Filter operations**:
   ```ini
   account-history-whitelist-ops = transfer transfer_to_vesting withdraw_vesting
   ```

4. **Use RocksDB alternative**:
   ```ini
   # Switch to disk-based storage
   plugin = account_history_rocksdb
   ```

## API Integration

### account_history_api

Access history through `account_history_api` plugin:

```json
{
  "jsonrpc": "2.0",
  "method": "condenser_api.get_account_history",
  "params": ["alice", -1, 10],
  "id": 1
}
```

**Parameters**:
- `account`: Account name
- `from`: Starting sequence number (-1 for most recent)
- `limit`: Maximum operations to return

**Response**:
```json
{
  "result": [
    [0, {
      "trx_id": "abc123...",
      "block": 12345678,
      "trx_in_block": 3,
      "op_in_trx": 0,
      "virtual_op": 0,
      "timestamp": "2024-01-15T12:00:00",
      "op": ["transfer", {
        "from": "alice",
        "to": "bob",
        "amount": "10.000 STEEM",
        "memo": "Payment"
      }]
    }]
  ]
}
```

## Performance Characteristics

### Query Performance

**Account history lookup**: O(log n) + O(m)
- n = total operations in database
- m = operations to retrieve

**Typical Performance**:
- Single operation: < 1ms
- 100 operations: 5-20ms
- 1000 operations: 50-200ms

### Write Performance

**Per block** (28,800 blocks/day):
- ~10-100 operations affecting accounts
- ~10-50 µs per operation to index

**Impact**: Minimal on block processing

### Replay Time

**With pruning enabled**:
- Moderate impact on replay speed
- Prunes as it goes

**With pruning disabled**:
- Significant impact on replay speed
- Memory allocation overhead increases over time

## Use Cases

### Block Explorer

**Configuration**:
```ini
plugin = account_history
# Track all accounts
history-disable-pruning = true
```

**Purpose**: Display complete account history for all users

### Exchange Integration

**Configuration**:
```ini
plugin = account_history
account-history-track-account-range = ["exchange-wallet", "exchange-wallet"]
account-history-track-account-range = ["exchange-hot", "exchange-hot"]
account-history-whitelist-ops = transfer transfer_to_vesting withdraw_vesting
```

**Purpose**: Track only exchange accounts and financial operations

### Wallet Backend

**Configuration**:
```ini
plugin = account_history
account-history-track-account-range = ["user1", "user1"]
account-history-track-account-range = ["user2", "user2"]
# ... add each wallet user
```

**Purpose**: Provide history for wallet users

### Archive Node

**Configuration**:
```ini
plugin = account_history
history-disable-pruning = true
# No operation filtering
# No account filtering
```

**Purpose**: Preserve complete blockchain history

## Monitoring

### Log Messages

**Initialization**:
```
Account History: whitelisting ops ["transfer","vote"]
Account History: blacklisting ops ["custom_json"]
```

**Operation Tracking** (no routine logs)

### Health Checks

**Good indicators**:
- Plugin loads without errors
- API queries return results promptly
- Memory usage stable (with pruning) or growing linearly (without)

**Warning signs**:
- Excessive memory growth
- Slow API responses (> 1 second)
- Out of memory errors

### Memory Monitoring

```bash
# Check shared memory file size
ls -lh witness_node_data_dir/blockchain/shared_mem.bin

# Monitor during operation
watch -n 5 'ls -lh witness_node_data_dir/blockchain/shared_mem.bin'
```

## Troubleshooting

### Out of Memory

**Problem**: Node crashes with OOM error

**Symptoms**:
```
std::bad_alloc
Out of memory
```

**Solutions**:

1. **Enable pruning**:
   ```ini
   # Remove or comment out
   # history-disable-pruning = true
   ```

2. **Reduce tracked accounts**:
   ```ini
   account-history-track-account-range = ["specific", "accounts"]
   ```

3. **Filter operations**:
   ```ini
   account-history-whitelist-ops = transfer vote
   ```

4. **Increase shared memory size**:
   ```ini
   shared-file-size = 64G  # Default: 54G
   ```

5. **Switch to RocksDB**:
   ```ini
   plugin = account_history_rocksdb
   ```

### Incomplete History

**Problem**: Missing operations in history

**Causes**:
- Plugin not enabled during replay
- Account range doesn't include account
- Operation filtered by whitelist/blacklist

**Solutions**:

1. **Verify plugin enabled during full replay**:
   ```bash
   steemd --replay-blockchain
   ```

2. **Check account range configuration**

3. **Verify operation not filtered**

4. **Replay from genesis with correct config**

### Slow API Queries

**Problem**: `get_account_history` taking too long

**Symptoms**:
- Queries timeout
- API responses > 1 second

**Causes**:
- Large history size
- Requesting too many operations
- Memory pressure

**Solutions**:

1. **Limit query size**:
   ```javascript
   // Request smaller batches
   get_account_history(account, from, 100)  // Instead of 1000
   ```

2. **Enable pruning** to reduce dataset

3. **Add more RAM**

4. **Use pagination** in applications

## Comparison: account_history vs account_history_rocksdb

| Feature | account_history | account_history_rocksdb |
|---------|----------------|------------------------|
| Storage | Memory (RAM) | Disk (RocksDB) |
| Speed | Very Fast | Fast |
| Memory Usage | High (10-50GB+) | Low (1-2GB) |
| Disk Usage | Minimal | High (20-100GB+) |
| Pruning | Optional | Built-in |
| Best For | Block explorers, High-performance APIs | Archive nodes, Resource-constrained |

**Recommendation**:
- Use `account_history` for **speed** (if you have RAM)
- Use `account_history_rocksdb` for **efficiency** (if RAM limited)

## Migration

### From account_history to account_history_rocksdb

1. **Stop node**
2. **Update config**:
   ```ini
   # Remove old plugin
   # plugin = account_history

   # Add new plugin
   plugin = account_history_rocksdb
   ```
3. **Replay blockchain**:
   ```bash
   steemd --replay-blockchain
   ```

### From account_history_rocksdb to account_history

1. **Stop node**
2. **Update config**:
   ```ini
   # Remove old plugin
   # plugin = account_history_rocksdb

   # Add new plugin
   plugin = account_history
   ```
3. **Ensure sufficient RAM** (20GB+ recommended)
4. **Replay blockchain**:
   ```bash
   steemd --replay-blockchain
   ```

## Best Practices

### For Node Operators

1. **Enable during initial sync**: Don't add mid-replay
2. **Monitor memory usage**: Set up alerts
3. **Use filtering wisely**: Balance coverage vs resources
4. **Regular backups**: Backup shared memory file
5. **Document configuration**: Track account ranges and filters

### For API Providers

1. **Implement pagination**: Don't request full history at once
2. **Cache results**: Account history rarely changes
3. **Rate limiting**: Protect against abusive queries
4. **Timeouts**: Set reasonable query timeouts (< 5 seconds)

### For Developers

1. **Batch queries**: Minimize API calls
2. **Handle missing history**: Account may not be tracked
3. **Respect limits**: Don't request excessive operations
4. **Error handling**: Handle timeout and memory errors gracefully

## Related Documentation

- [account_history_rocksdb Plugin](account_history_rocksdb.md) - Disk-based alternative
- [account_history_api Plugin](account_history_api.md) - RPC API interface
- [chain Plugin](chain.md) - Core blockchain database
- [Node Types Guide](../operations/node-types-guide.md) - Configuration by use case

## Source Code

- **Implementation**: [libraries/plugins/account_history/account_history_plugin.cpp](../../libraries/plugins/account_history/account_history_plugin.cpp)
- **Plugin Header**: [libraries/plugins/account_history/include/steem/plugins/account_history/account_history_plugin.hpp](../../libraries/plugins/account_history/include/steem/plugins/account_history/account_history_plugin.hpp)

## License

See [LICENSE.md](../../LICENSE.md)
