# chain Plugin

Core blockchain database and validation engine for the Steem network.

## Overview

The `chain` plugin is the heart of every Steem node. It manages the blockchain database, validates blocks and transactions, executes operations, and maintains consensus with the network.

**Plugin Type**: Core (Always Required)
**Dependencies**: None (base plugin)
**Memory**: ~24GB base + growth over time
**Default**: Always enabled (cannot be disabled)

## Purpose

- **Blockchain Database**: Manage the shared memory database containing all blockchain state
- **Block Validation**: Validate incoming blocks and transactions
- **Consensus**: Ensure compliance with consensus rules and hardforks
- **Operation Execution**: Execute blockchain operations (transfers, votes, posts, etc.)
- **State Management**: Maintain current blockchain state (accounts, balances, content, etc.)
- **Hardfork Management**: Track and enforce protocol upgrades

## Database Objects

The chain plugin manages these core data structures:

### Account Objects
- `account_object` - User accounts
- `account_authority_object` - Account permissions (owner/active/posting)
- `account_metadata_object` - Account profile data

### Content Objects
- `comment_object` - Posts and comments
- `comment_vote_object` - Votes on content
- `comment_content_object` - Post/comment body text

### Financial Objects
- `vesting_delegation_object` - Steem Power delegations
- `withdraw_vesting_route_object` - Power down routes
- `savings_withdraw_object` - Savings withdrawals
- `escrow_object` - Escrow agreements
- `decline_voting_rights_request_object` - Decline voting rights requests

### Witness Objects
- `witness_object` - Witness information
- `witness_vote_object` - Witness votes
- `witness_schedule_object` - Block production schedule

### Global Objects
- `dynamic_global_property_object` - Current blockchain state
- `feed_history_object` - SBD price feed history
- `hardfork_property_object` - Hardfork status
- `block_summary_object` - Recent block IDs

### Market Objects
- `limit_order_object` - Internal market orders
- `convert_request_object` - SBD → STEEM conversions

## Configuration

### Config File Options

```ini
# Shared memory file location
shared-file-dir = blockchain

# Shared memory file size (grows over time)
shared-file-size = 54G

# Block log location
# block-log-dir = blockchain

# Checkpoint validation
# checkpoint =

# Flush state to disk interval (0 = only at shutdown)
flush-state-interval = 0

# Enable plugins that modify state
# Required when using state-tracking plugins
# plugin = account_history tags follow
```

### Command Line Options

```bash
steemd \
  --shared-file-dir=/mnt/ssd/blockchain \
  --shared-file-size=100G \
  --replay-blockchain  # Replay from block log
```

### Important Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `shared-file-dir` | blockchain | Directory for shared memory files |
| `shared-file-size` | 54G | Maximum shared memory size |
| `flush-state-interval` | 0 | Blocks between state flushes (0 = shutdown only) |
| `checkpoint` | (none) | Enforce specific block IDs at block numbers |

## Files and Storage

### Shared Memory Files

Located in `shared-file-dir/`:

```
blockchain/
├── shared_memory.bin      # Main state database (mmap)
├── shared_memory.meta     # Metadata
└── block_log/
    ├── block_log          # Immutable block history
    └── block_log.index    # Block index
```

**shared_memory.bin**:
- Memory-mapped file (mmap)
- Contains all blockchain state
- Can be backed up while node is running
- Grows over time (currently ~24GB base)

**block_log**:
- Append-only file of all blocks
- Immutable and reproducible
- Used for replay
- Currently ~27GB+ (continuously growing)

### Database Technology

**Chainbase**: Custom memory-mapped multi-index database
- **Copy-on-write**: Supports undo/redo for transaction atomicity
- **Multi-index**: Fast lookups by multiple keys
- **Read/Write Locks**: Thread-safe concurrent access

## Operations

The chain plugin executes these blockchain operations:

### Account Operations
- `account_create_operation` - Create new account
- `account_update_operation` - Update account
- `account_witness_vote_operation` - Vote for witness
- `recover_account_operation` - Account recovery

### Content Operations
- `comment_operation` - Create post/comment
- `vote_operation` - Vote on content
- `delete_comment_operation` - Delete post/comment
- `comment_options_operation` - Set post options

### Financial Operations
- `transfer_operation` - Transfer STEEM/SBD
- `transfer_to_vesting_operation` - Power up
- `withdraw_vesting_operation` - Power down
- `transfer_to_savings_operation` - Transfer to savings
- `transfer_from_savings_operation` - Withdraw from savings
- `claim_reward_balance_operation` - Claim rewards

### Witness Operations
- `witness_update_operation` - Update witness
- `feed_publish_operation` - Publish SBD price feed
- `witness_set_properties_operation` - Set witness properties

### Market Operations
- `limit_order_create_operation` - Create market order
- `limit_order_cancel_operation` - Cancel market order
- `convert_operation` - Convert SBD to STEEM

**Total**: 60+ operation types (see [steem_operations.hpp](../../src/core/protocol/include/steem/protocol/steem_operations.hpp))

## Consensus Rules

### Block Production

- **Block Interval**: 3 seconds
- **Witness Schedule**: 21 witnesses in round-robin order
- **Irreversibility**: After 15 witnesses confirm (~45 seconds)

### Resource Limits

- **Maximum Block Size**: 64KB (compressed)
- **Operations per Block**: ~100-200 (depends on operation types)
- **Resource Credits**: Rate limiting system for bandwidth/compute

### Validation Rules

1. **Signature Validation**: All transactions must be properly signed
2. **Balance Checks**: Sufficient funds for transfers
3. **Authority Checks**: Correct permission level (owner/active/posting)
4. **Replay Protection**: Transaction expiration (1 hour max)
5. **Nonce/Sequence**: Prevent duplicate transactions

## Hardforks

The chain plugin manages protocol upgrades through hardforks:

**Current Hardforks**: HF0 through HF26+ (see [hardfork.d/](../../src/core/protocol/hardfork.d/))

### Hardfork Activation

```cpp
// Check current hardfork
if (db.has_hardfork(STEEM_HARDFORK_0_20))
{
   // HF20+ features enabled
}
```

### Notable Hardforks

- **HF20**: Resource Credits (RC) system
- **HF21**: Economic changes (curve, downvotes)
- **HF23**: Governance improvements

## Performance

### Read Operations

**Fast** (single index lookup):
- Get account by name: `O(log n)`
- Get block by number: `O(1)`
- Get comment by permlink: `O(log n)`

**Slow** (iteration):
- List all accounts: `O(n)` - use pagination
- Complex queries: Use state-tracking plugins instead

### Write Operations

All modifications wrapped in transactions:
```cpp
db.modify(account, [&](account_object& a) {
   a.balance += asset(1000, STEEM_SYMBOL);
});
```

**Atomicity**: All-or-nothing (full rollback on failure)

### Optimization Tips

1. **SSD Storage**: Use SSD for shared memory directory
2. **RAM Disk**: Put shared memory on tmpfs for maximum performance
   ```bash
   mount -t tmpfs -o size=100G tmpfs /mnt/ramdisk
   steemd --shared-file-dir=/mnt/ramdisk/blockchain
   ```
3. **Flush Interval**: Set to 0 for witnesses (consistency > performance)

## Replay

### Full Replay

Rebuild state from block log (required after certain changes):

```bash
steemd --replay-blockchain --data-dir=/path/to/data
```

**When Required**:
- Adding/removing state-tracking plugins
- Upgrading from very old version
- Fixing database corruption

**Duration**: Several hours to days (depends on hardware)

### Resync from Network

Delete state and re-download from P2P:

```bash
rm -rf /path/to/data/blockchain/shared_memory.*
steemd --data-dir=/path/to/data
```

**Duration**: Faster than replay (no operation re-execution)

## Monitoring

### Key Metrics

```bash
# Check sync status
curl -s http://localhost:8090 \
  -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_dynamic_global_properties"}' \
  | jq -r '.result.head_block_number'

# Compare with network
# Should match: https://api.steemit.com
```

### Health Indicators

- **Head Block Number**: Should increase every 3 seconds
- **Last Irreversible Block**: Should be ~15 blocks behind head
- **Missed Blocks**: Should be 0 for witnesses
- **P2P Connections**: Should have 8-30 active peers

### Database Size

```bash
# Check shared memory size
ls -lh /path/to/blockchain/shared_memory.bin

# Check block log size
ls -lh /path/to/blockchain/block_log/block_log
```

## API Access

The chain plugin is accessed indirectly through API plugins:

- [database_api](database_api.md) - Read blockchain state
- [network_broadcast_api](network_broadcast_api.md) - Submit transactions
- [block_api](block_api.md) - Query blocks

**Direct access**: Not recommended (use API plugins)

## Troubleshooting

### Database Corruption

**Symptoms**:
```
assertion failed: _db.get_segment_manager()->check_sanity()
Database dirty flag set
```

**Solution**: Replay blockchain
```bash
steemd --replay-blockchain
```

### Out of Shared Memory

**Symptoms**:
```
bad_alloc exception
Out of shared memory
```

**Solution**: Increase shared-file-size
```ini
shared-file-size = 100G  # Was 54G
```

### Fork Resolution

**Symptoms**:
```
Block does not link to known chain
Switching to fork
```

**Normal**: Network automatically resolves forks
**Action**: Monitor and ensure sync completes

### Slow Sync

**Symptoms**: Block processing < 100 blocks/sec

**Solutions**:
1. Use SSD storage
2. Increase system resources
3. Check network bandwidth
4. Disable unnecessary plugins

## Build Options

### Standard Build

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) steemd
```

### Low Memory Node

Consensus-only node (no state tracking):

```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DLOW_MEMORY_NODE=ON \
      ..
```

**Result**: ~4GB shared memory (vs ~24GB+)

### Testnet Build

For development/testing:

```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_STEEM_TESTNET=ON \
      ..
```

## Related Documentation

- [database_api](database_api.md) - Query blockchain data
- [System Architecture](../technical-reference/system-architecture.md) - Overall design
- [Operations](../../src/core/protocol/include/steem/protocol/steem_operations.hpp) - All operation types
- [Node Types](../operations/node-types-guide.md) - Configuration by use case

## Source Code

- **Database**: [src/core/chain/database.cpp](../../src/core/chain/database.cpp)
- **Operations**: [src/core/chain/*_evaluator.cpp](../../src/core/chain/)
- **Objects**: [src/core/chain/include/steem/chain/*_object.hpp](../../src/core/chain/include/steem/chain/)
- **Plugin**: [src/plugins/chain/chain_plugin.cpp](../../src/plugins/chain/chain_plugin.cpp)

## License

See [LICENSE.md](../../LICENSE.md)
