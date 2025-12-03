# account_history_rocksdb Plugin

Disk-based plugin that tracks all operations affecting accounts using RocksDB for efficient storage with low memory footprint.

## Overview

The `account_history_rocksdb` plugin provides the same functionality as [account_history](account_history.md) but stores data on disk using RocksDB instead of in memory. This trades some query speed for dramatically reduced RAM requirements, making it ideal for resource-constrained nodes or archive nodes storing complete history.

**Plugin Type**: State (Tracking)
**Dependencies**: [chain](chain.md)
**Memory**: Low (1-2GB)
**Disk**: High (20-100GB+)
**Default**: Disabled (enable explicitly)
**Alternative**: [account_history](account_history.md) (memory-based)

## Purpose

- **Low-Memory History Tracking**: Track account history with minimal RAM
- **Archive Node Support**: Store complete blockchain history efficiently
- **Resource Optimization**: Enable history on nodes with limited memory
- **Long-Term Storage**: Maintain historical data without memory constraints
- **Production Scalability**: Support large-scale deployments

## How It Works

### Storage Architecture

**RocksDB Storage** (key-value database):
- **Persistent disk storage**: Data survives restarts
- **Compression**: Automatic data compression
- **Column families**: Separate indexes for efficient queries
- **LSM tree structure**: Optimized for write-heavy workloads

**Column Families**:
1. `current_lib` - Last irreversible block number
2. `operation_by_id` - Operations indexed by ID
3. `operation_by_block` - Operations indexed by block number
4. `account_history_info_by_name` - Account metadata
5. `ah_operation_by_id` - Account history entries

### Data Flow

```
Operation occurs
    ↓
Identify impacted accounts
    ↓
Store in volatile memory (until irreversible)
    ↓
On irreversible block
    ↓
Write to RocksDB
    ↓
Remove from volatile memory
```

**Volatile Storage**: Operations stored in memory until block is irreversible
**Persistent Storage**: Irreversible operations written to RocksDB
**Pruning**: Automatic pruning maintains manageable database size

## Configuration

### Config File Options

```ini
# Enable the plugin
plugin = account_history_rocksdb

# Database location (default: $DATA_DIR/blockchain/account-history-rocksdb-storage)
account-history-rocksdb-path = /path/to/rocksdb/storage

# Track specific account ranges
account-history-rocksdb-track-account-range = ["alice", "alice"]
account-history-rocksdb-track-account-range = ["bob", "charlie"]

# Whitelist operations (only track these)
account-history-rocksdb-whitelist-ops = transfer vote comment

# Blacklist operations (track all except these)
account-history-rocksdb-blacklist-ops = custom_json follow_operation
```

### Command Line Options

```bash
# Basic usage - track all accounts
steemd --plugin=account_history_rocksdb

# Custom database path
steemd --plugin=account_history_rocksdb \
  --account-history-rocksdb-path=/ssd/rocksdb-data

# Track specific accounts
steemd --plugin=account_history_rocksdb \
  --account-history-rocksdb-track-account-range='["alice","alice"]'

# Immediate import (vs during replay)
steemd --plugin=account_history_rocksdb \
  --account-history-rocksdb-immediate-import

# Import up to specific block
steemd --plugin=account_history_rocksdb \
  --account-history-rocksdb-immediate-import \
  --account-history-rocksdb-stop-import-at-block=1000000
```

### Configuration Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `account-history-rocksdb-path` | path | blockchain/account-history-rocksdb-storage | RocksDB database location |
| `account-history-rocksdb-track-account-range` | array | (all) | Account ranges to track |
| `account-history-rocksdb-whitelist-ops` | list | (all) | Only track these operations |
| `account-history-rocksdb-blacklist-ops` | list | (none) | Ignore these operations |
| `account-history-rocksdb-immediate-import` | bool | false | Import data immediately on startup |
| `account-history-rocksdb-stop-import-at-block` | number | 0 | Stop import at this block (0=no limit) |

## Account Tracking

### Track All Accounts (Default)

```ini
plugin = account_history_rocksdb
# Tracks ALL accounts
```

**Disk Impact**: Large (50-100GB+)
**Use Case**: Block explorer, public API node

### Track Specific Accounts

**Single account**:
```ini
account-history-rocksdb-track-account-range = ["alice", "alice"]
```

**Multiple accounts**:
```ini
account-history-rocksdb-track-account-range = ["alice", "alice"]
account-history-rocksdb-track-account-range = ["bob", "bob"]
account-history-rocksdb-track-account-range = ["exchange", "exchange"]
```

**Account ranges**:
```ini
# Track all accounts from "user1" to "user9999"
account-history-rocksdb-track-account-range = ["user1", "user9999"]
```

**Disk Impact**: Proportional to tracked accounts
**Use Case**: Exchange, wallet service, specific monitoring

## Operation Filtering

### Whitelist Mode

**Only track specified operations**:

```ini
# Track only financial operations
account-history-rocksdb-whitelist-ops = transfer transfer_to_vesting \
  withdraw_vesting claim_reward_balance
```

**Common Whitelists**:

**Financial only**:
```ini
account-history-rocksdb-whitelist-ops = transfer transfer_to_vesting \
  withdraw_vesting transfer_to_savings transfer_from_savings \
  claim_reward_balance
```

**Content only**:
```ini
account-history-rocksdb-whitelist-ops = comment vote delete_comment \
  comment_options
```

### Blacklist Mode

**Track all EXCEPT specified operations**:

```ini
# Exclude high-volume, low-value operations
account-history-rocksdb-blacklist-ops = custom_json
```

### Operation Names

Format: Use C++ type name **with** `steem::protocol::` prefix (added automatically):

```
transfer              ✓ Correct (automatically becomes steem::protocol::transfer_operation)
custom_json           ✓ Correct
transfer_operation    ✗ Wrong (don't add _operation suffix)
```

## Automatic Pruning

### Default Pruning Policy

**Always enabled** - maintains database at reasonable size:

**Retention Limits**:
- Last **30 operations** per account (minimum)
- Last **30 days** of operations (minimum)
- **Union of both** (keeps whichever is more)

**Example**:
```
Account with 100 ops/day: ~30 days = 3000 operations retained
Account with 1 op/week:   30 operations = ~30 weeks retained
```

### Pruning Algorithm

```
On each new operation:
  If account has > 30 operations AND oldest > 30 days old:
    Remove oldest operation
    Update account metadata
```

**Benefits**:
- Prevents unbounded growth
- Maintains consistent performance
- Automatic - no configuration needed

**Note**: Unlike `account_history`, pruning **cannot be disabled** in RocksDB version.

## Database Management

### Database Location

**Default**: `$DATA_DIR/blockchain/account-history-rocksdb-storage/`

**Custom Location**:
```ini
# Absolute path
account-history-rocksdb-path = /mnt/ssd/account-history

# Relative path (relative to data directory)
account-history-rocksdb-path = rocksdb/account-history
```

**Best Practice**: Place on SSD for optimal performance

### Database Files

```
account-history-rocksdb-storage/
├── 000003.log          # Write-ahead log
├── CURRENT             # Current manifest
├── IDENTITY            # Database identity
├── LOCK                # Lock file
├── LOG                 # RocksDB log
├── MANIFEST-000002     # Database manifest
├── OPTIONS-000005      # RocksDB options
└── *.sst               # Sorted string table files (data)
```

### Database Size

**Typical Sizes**:

| Configuration | Approximate Size |
|--------------|------------------|
| All accounts, all operations | 80-150 GB |
| All accounts, financial ops only | 30-60 GB |
| 100 accounts, all operations | 500 MB - 2 GB |
| Single account, all operations | 5-50 MB |

**Growth Rate**: ~100-200 MB per day (full node)

## Import Modes

### Replay Import (Default)

**Automatic during blockchain replay**:

```bash
# Enable plugin and replay
steemd --replay-blockchain
```

**Behavior**:
- Imports data as blocks are replayed
- Optimized for batch operations
- Write buffer flushing optimized
- Recommended method

### Immediate Import

**Import existing blockchain data on startup**:

```bash
steemd --account-history-rocksdb-immediate-import
```

**Use Cases**:
- Adding plugin to existing synced node
- Rebuilding database without full replay
- Testing/debugging

**Performance**: Slower than replay import

### Partial Import

**Import up to specific block**:

```bash
steemd --account-history-rocksdb-immediate-import \
  --account-history-rocksdb-stop-import-at-block=5000000
```

**Use Cases**:
- Testing
- Partial history nodes
- Development

## RocksDB Configuration

### Automatic Optimization

Plugin uses optimized RocksDB settings:

```cpp
options.IncreaseParallelism();          // Use multiple threads
options.OptimizeLevelStyleCompaction(); // Optimize for LSM tree
options.max_open_files = 750;           // Limit file descriptors
```

### Column Family Comparators

Custom comparators for efficient indexing:
- `by_id`: Integer comparison for operation IDs
- `by_block_num`: Block number comparison
- `by_account_name`: Account name comparison
- `ah_op_by_id`: Account history entry comparison

### Storage Version

**Version**: 1.0 (major.minor)
**Compatibility**: Database version is checked on open
**Migration**: Version mismatch requires rebuild

## Performance Characteristics

### Query Performance

**Account history lookup**:
- Single operation: 1-5 ms
- 100 operations: 50-200 ms
- 1000 operations: 500-2000 ms

**Slower than memory-based** but acceptable for most use cases

### Write Performance

**During replay**:
- Buffered writes (10 operations before flush)
- Batch optimization
- ~100-500 operations/second

**During normal operation**:
- Immediate writes (1 operation flush limit)
- Lower latency for irreversible blocks

### Resource Usage

| Resource | Usage |
|----------|-------|
| **RAM** | 1-2 GB |
| **Disk** | 20-150 GB |
| **CPU** | Low (compression, compaction) |
| **I/O** | Moderate (SSD recommended) |

## API Integration

### account_history_api

Access through same API as memory-based plugin:

```json
{
  "jsonrpc": "2.0",
  "method": "account_history_api.get_account_history",
  "params": ["alice", -1, 100],
  "id": 1
}
```

**Compatibility**: Drop-in replacement for `account_history` plugin

### Query Methods

**Plugin-specific methods** (if exposed):

```cpp
// Find account history
find_account_history_data(account, start, limit, processor)

// Find specific operation
find_operation_object(operation_id, &operation_object)

// Find operations in block
find_operations_by_block(block_num, processor)

// Enumerate virtual operations
enum_operations_from_block_range(start_block, end_block, processor)
```

## Monitoring

### Log Messages

**Initialization**:
```
Initializing account_history_rocksdb plugin
RocksDB opened successfully storage at location: `/path/to/db`
Loaded OperationObject seqId: 12345, AccountHistoryObject seqId: 67890
```

**Progress** (every 10,000 blocks during import):
```
RocksDb data import processed blocks: 50000, containing: 1523 transactions and 8234 operations.
  150 operations have been filtered out due to configured options.
  25 accounts have been filtered out due to configured options.
```

**Completion**:
```
RocksDB data import finished. Processed blocks: 100000, containing: ...
Performance report at block 100000. Elapsed time: 3600000 ms (real), 2800000 ms (cpu).
Memory usage: 1024 (current), 1536 (peak) kilobytes.
```

### Health Checks

**Good indicators**:
- Database opens successfully
- Consistent write performance
- Disk space not exhausted
- API queries respond promptly

**Warning signs**:
- Database corruption errors
- Slow query performance (> 5 seconds)
- Disk space filling rapidly
- High I/O wait times

### Database Maintenance

```bash
# Check database size
du -sh /path/to/rocksdb/storage

# Monitor disk usage
df -h /mount/point

# Check RocksDB logs
tail -f /path/to/rocksdb/storage/LOG
```

## Troubleshooting

### Database Corruption

**Problem**: Database won't open or returns errors

**Symptoms**:
```
RocksDB cannot open database
Data access failed: Corruption: ...
```

**Solutions**:

1. **Verify disk health**: Check for disk errors
2. **Restore from backup**: If backup available
3. **Rebuild database**:
   ```bash
   rm -rf /path/to/rocksdb/storage
   steemd --replay-blockchain
   ```

### Slow Queries

**Problem**: API calls taking too long

**Causes**:
- Disk I/O bottleneck
- HDD instead of SSD
- Database needs compaction

**Solutions**:

1. **Use SSD storage**:
   ```ini
   account-history-rocksdb-path = /ssd/rocksdb
   ```

2. **Reduce query size**: Request fewer operations

3. **Add indexes**: Ensure column families properly configured

4. **Monitor disk I/O**: Use `iostat`, `iotop`

### Disk Space Exhaustion

**Problem**: Database fills available disk

**Symptoms**:
```
No space left on device
Write failed: ...
```

**Solutions**:

1. **Increase disk space**: Add storage

2. **Reduce tracked accounts**:
   ```ini
   account-history-rocksdb-track-account-range = ["specific", "accounts"]
   ```

3. **Filter operations**:
   ```ini
   account-history-rocksdb-whitelist-ops = transfer vote
   ```

4. **Move database**:
   ```bash
   # Stop node
   mv /old/path /new/larger/disk/path
   # Update config with new path
   # Restart node
   ```

### Import Failures

**Problem**: Import stops or fails

**Causes**:
- Block limit reached
- Disk space exhausted
- Database corruption
- System crash

**Solutions**:

1. **Check logs**: Examine error messages

2. **Verify disk space**: Ensure adequate space

3. **Resume replay**:
   ```bash
   # Replay will resume from last good block
   steemd --replay-blockchain
   ```

4. **Clean rebuild**:
   ```bash
   rm -rf /path/to/rocksdb/storage
   steemd --replay-blockchain
   ```

## Comparison: RocksDB vs Memory

| Feature | RocksDB | Memory |
|---------|---------|--------|
| **RAM Usage** | 1-2 GB | 20-50+ GB |
| **Disk Usage** | 20-150 GB | Minimal |
| **Query Speed** | 5-10x slower | Fastest |
| **Pruning** | Always on | Optional |
| **Persistence** | Survives restart | Requires shared memory |
| **Scalability** | Excellent | Limited by RAM |
| **Best For** | Archive, Production | Block Explorer, High Performance |

**Choose RocksDB when**:
- RAM is limited (< 32 GB)
- Running multiple nodes
- Cost optimization important
- Archive node requirements

**Choose Memory when**:
- RAM is abundant (64+ GB)
- Highest performance needed
- Block explorer backend
- Real-time APIs

## Migration

### From account_history to RocksDB

1. **Stop node**

2. **Update config**:
   ```ini
   # Remove
   # plugin = account_history

   # Add
   plugin = account_history_rocksdb
   ```

3. **Replay blockchain**:
   ```bash
   steemd --replay-blockchain
   ```

### From RocksDB to account_history

1. **Ensure sufficient RAM** (32+ GB recommended)

2. **Stop node**

3. **Update config**:
   ```ini
   # Remove
   # plugin = account_history_rocksdb

   # Add
   plugin = account_history
   ```

4. **Increase shared memory** (if needed):
   ```ini
   shared-file-size = 64G
   ```

5. **Replay blockchain**:
   ```bash
   steemd --replay-blockchain
   ```

## Best Practices

### For Node Operators

1. **Use SSD storage**: Dramatically improves performance
2. **Monitor disk space**: Set up alerts for 80% usage
3. **Regular backups**: Backup RocksDB directory
4. **Separate disk**: Use dedicated disk/partition if possible
5. **Enable during replay**: Don't add mid-sync

### For API Providers

1. **Implement caching**: Cache frequently requested data
2. **Pagination**: Limit operations per query (< 1000)
3. **Timeouts**: Set reasonable timeouts (5-10 seconds)
4. **Rate limiting**: Protect against abuse
5. **Monitor performance**: Track query latency

### For Developers

1. **Batch queries**: Minimize API calls
2. **Handle timeouts**: Implement retry logic
3. **Respect limits**: Don't request excessive data
4. **Cache results**: History rarely changes
5. **Error handling**: Handle database errors gracefully

## Hardware Recommendations

### Minimum Requirements

- **RAM**: 8 GB
- **Disk**: 200 GB SSD
- **CPU**: 4 cores

### Recommended Configuration

- **RAM**: 16 GB
- **Disk**: 500 GB NVMe SSD
- **CPU**: 8 cores
- **Network**: 100 Mbps

### Optimal Setup

- **RAM**: 32 GB
- **Disk**: 1 TB NVMe SSD (dedicated)
- **CPU**: 16 cores
- **Network**: 1 Gbps

## Related Documentation

- [account_history Plugin](account_history.md) - Memory-based alternative
- [account_history_api Plugin](account_history_api.md) - RPC API interface
- [chain Plugin](chain.md) - Core blockchain database
- [Node Modes Guide](../operations/node-modes-guide.md) - Configuration by use case

## Source Code

- **Implementation**: [src/plugins/account_history_rocksdb/account_history_rocksdb_plugin.cpp](../../src/plugins/account_history_rocksdb/account_history_rocksdb_plugin.cpp)
- **Objects**: [src/plugins/account_history_rocksdb/include/steem/plugins/account_history_rocksdb/account_history_rocksdb_objects.hpp](../../src/plugins/account_history_rocksdb/include/steem/plugins/account_history_rocksdb/account_history_rocksdb_objects.hpp)
- **Plugin Header**: [src/plugins/account_history_rocksdb/include/steem/plugins/account_history_rocksdb/account_history_rocksdb_plugin.hpp](../../src/plugins/account_history_rocksdb/include/steem/plugins/account_history_rocksdb/account_history_rocksdb_plugin.hpp)

## License

See [LICENSE.md](../../LICENSE.md)
