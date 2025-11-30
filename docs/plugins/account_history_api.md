# account_history_api Plugin

Query account transaction history and operation details on the Steem blockchain.

## Overview

The `account_history_api` plugin provides API endpoints to retrieve the complete operation history for accounts, including transfers, posts, votes, and all other blockchain operations. This is essential for wallets, explorers, and applications that need to display account activity.

**Plugin Type**: API (Read-only)
**Dependencies**: [account_history](account_history.md) or [account_history_rocksdb](account_history_rocksdb.md), [json_rpc](json_rpc.md)
**State Plugin**: account_history or account_history_rocksdb
**Memory**: Large (10-50GB for account_history, minimal for rocksdb variant)
**Default**: Not enabled

## Purpose

- **Transaction History**: Retrieve complete operation history for accounts
- **Operation Lookup**: Find operations in specific blocks
- **Transaction Details**: Get full transaction data by ID
- **Virtual Operations**: Access system-generated operations (rewards, interest, etc.)
- **Wallet Support**: Display transaction history in wallets
- **Blockchain Explorer**: Power block and transaction explorers

## Configuration

### Enable Plugin

```ini
# In config.ini - use one of the account history implementations
plugin = account_history account_history_api

# OR use RocksDB-based implementation (recommended for production)
plugin = account_history_rocksdb account_history_api
```

### Command Line

```bash
# Memory-based (small chains or testing)
steemd --plugin="account_history account_history_api"

# RocksDB-based (production recommended)
steemd --plugin="account_history_rocksdb account_history_api"
```

### Track Specific Accounts

```ini
# Track only specific accounts (reduces memory usage)
account-history-track-account-range = ["alice","bob"]

# Track all accounts except specific ones
account-history-blacklist-ops = ["op1","op2"]
```

## API Methods

### get_ops_in_block

Retrieve all operations that occurred in a specific block.

**Parameters**:
```json
{
  "block_num": 12345678,
  "only_virtual": false
}
```

**Returns**:
```json
{
  "ops": [
    {
      "trx_id": "0000000000000000000000000000000000000000",
      "block": 12345678,
      "trx_in_block": 0,
      "op_in_trx": 0,
      "virtual_op": 0,
      "timestamp": "2023-01-15T12:34:56",
      "op": ["vote", {
        "voter": "alice",
        "author": "bob",
        "permlink": "test-post",
        "weight": 10000
      }]
    }
  ]
}
```

**Parameters**:
- `block_num`: Block number to query
- `only_virtual`: If true, only return virtual operations (system-generated)

### get_transaction

Retrieve a transaction by its ID.

**Parameters**:
```json
{
  "id": "abc123..."
}
```

**Returns**: Full annotated signed transaction with operations and metadata.

### get_account_history

Get operation history for a specific account.

**Parameters**:
```json
{
  "account": "alice",
  "start": 1000,
  "limit": 100
}
```

**Returns**:
```json
{
  "history": {
    "1000": {
      "trx_id": "...",
      "block": 12345678,
      "trx_in_block": 0,
      "op_in_trx": 0,
      "virtual_op": 0,
      "timestamp": "2023-01-15T12:34:56",
      "op": ["transfer", {
        "from": "alice",
        "to": "bob",
        "amount": "10.000 STEEM",
        "memo": "Payment"
      }]
    }
  }
}
```

**Parameters**:
- `account`: Account name to query
- `start`: Starting index (-1 for most recent, or specific sequence number)
- `limit`: Maximum number of operations to return (max 1000)

### enum_virtual_ops

Enumerate virtual operations within a block range.

**Parameters**:
```json
{
  "block_range_begin": 10000000,
  "block_range_end": 10000100
}
```

**Returns**:
```json
{
  "ops": [
    {
      "trx_id": "0000000000000000000000000000000000000000",
      "block": 10000001,
      "trx_in_block": 65535,
      "virtual_op": 1,
      "timestamp": "2023-01-15T12:34:56",
      "op": ["producer_reward", {
        "producer": "witness1",
        "vesting_shares": "123.456789 VESTS"
      }]
    }
  ],
  "next_block_range_begin": 10000100
}
```

## Usage Examples

### Get Recent Account Operations

Retrieve the 10 most recent operations for an account:

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "account_history_api.get_account_history",
  "params": {
    "account": "alice",
    "start": -1,
    "limit": 10
  },
  "id": 1
}' | jq
```

### Get All Operations in a Block

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "account_history_api.get_ops_in_block",
  "params": {
    "block_num": 50000000,
    "only_virtual": false
  },
  "id": 1
}' | jq
```

### Get Only Virtual Operations

Virtual operations are system-generated (rewards, interest, fills):

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "account_history_api.get_ops_in_block",
  "params": {
    "block_num": 50000000,
    "only_virtual": true
  },
  "id": 1
}' | jq
```

### Lookup Transaction by ID

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "account_history_api.get_transaction",
  "params": {
    "id": "abc123def456..."
  },
  "id": 1
}' | jq
```

### Paginate Through Account History

```python
import requests
import json

def get_all_account_operations(account, limit_per_page=1000):
    """Retrieve all operations for an account with pagination."""
    all_ops = []
    start = -1

    while True:
        payload = {
            "jsonrpc": "2.0",
            "method": "account_history_api.get_account_history",
            "params": {
                "account": account,
                "start": start,
                "limit": limit_per_page
            },
            "id": 1
        }

        response = requests.post("http://localhost:8090", json=payload)
        result = response.json()

        if "result" not in result or not result["result"]["history"]:
            break

        history = result["result"]["history"]
        all_ops.extend(history.values())

        # Get the lowest sequence number for next page
        min_seq = min(int(k) for k in history.keys())

        if min_seq == 0:
            break

        start = min_seq - 1

    return all_ops

# Example usage
operations = get_all_account_operations("alice", limit_per_page=100)
print(f"Total operations: {len(operations)}")
```

### Filter Operations by Type

```python
def get_transfers_only(account, limit=100):
    """Get only transfer operations for an account."""
    payload = {
        "jsonrpc": "2.0",
        "method": "account_history_api.get_account_history",
        "params": {
            "account": account,
            "start": -1,
            "limit": limit
        },
        "id": 1
    }

    response = requests.post("http://localhost:8090", json=payload)
    result = response.json()

    transfers = []
    if "result" in result:
        for seq, op_obj in result["result"]["history"].items():
            op_type = op_obj["op"][0]
            if op_type == "transfer":
                transfers.append(op_obj)

    return transfers
```

### Find Reward Operations

```bash
# Find author rewards in block range
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "account_history_api.enum_virtual_ops",
  "params": {
    "block_range_begin": 50000000,
    "block_range_end": 50000100
  },
  "id": 1
}' | jq '.result.ops[] | select(.op[0] == "author_reward")'
```

## Operation Types

### User Operations

Operations initiated by users:
- `transfer` - Send STEEM/SBD
- `transfer_to_vesting` - Power up
- `withdraw_vesting` - Power down
- `vote` - Upvote/downvote content
- `comment` - Post or comment
- `custom_json` - Custom data operations

### Virtual Operations

System-generated operations:
- `author_reward` - Content author rewards
- `curation_reward` - Curation rewards
- `comment_reward` - Comment rewards
- `producer_reward` - Witness/producer rewards
- `interest` - SBD interest payments
- `fill_order` - Market order fills
- `fill_vesting_withdraw` - Vesting withdrawal execution

## Performance Considerations

### Memory Usage

**account_history** (in-memory):
- Full mainnet: 30-50 GB RAM
- Partial tracking: 1-10 GB RAM

**account_history_rocksdb** (disk-based):
- RAM: ~2-4 GB
- Disk: 100-200 GB
- Recommended for production

### Query Performance

- **get_account_history**: 10-100ms depending on limit
- **get_ops_in_block**: 1-50ms depending on block size
- **get_transaction**: 1-10ms
- **enum_virtual_ops**: 50-500ms depending on range

### Best Practices

**Limit Queries**: Use limit parameter to avoid large responses
**Cache Results**: Cache operation history in your application
**Use RocksDB**: For production nodes, use account_history_rocksdb
**Track Selectively**: Only track accounts you need if memory is limited

### Pagination Strategy

```javascript
// Efficient pagination pattern
async function getAllOperations(account) {
    const operations = [];
    let start = -1;
    let hasMore = true;

    while (hasMore) {
        const response = await fetch('http://localhost:8090', {
            method: 'POST',
            body: JSON.stringify({
                jsonrpc: '2.0',
                method: 'account_history_api.get_account_history',
                params: { account, start, limit: 1000 },
                id: 1
            })
        });

        const result = await response.json();
        const history = result.result.history;

        if (Object.keys(history).length === 0) {
            hasMore = false;
        } else {
            operations.push(...Object.values(history));
            const indices = Object.keys(history).map(Number);
            start = Math.min(...indices) - 1;

            if (start < 0) hasMore = false;
        }
    }

    return operations;
}
```

## Common Use Cases

### Wallet Transaction History

```python
def format_wallet_history(account):
    """Format account history for wallet display."""
    payload = {
        "jsonrpc": "2.0",
        "method": "account_history_api.get_account_history",
        "params": {"account": account, "start": -1, "limit": 50},
        "id": 1
    }

    response = requests.post("http://localhost:8090", json=payload)
    result = response.json()

    transactions = []
    for seq, op_obj in result["result"]["history"].items():
        op_type, op_data = op_obj["op"]

        tx = {
            "type": op_type,
            "timestamp": op_obj["timestamp"],
            "block": op_obj["block"],
            "data": op_data
        }

        transactions.append(tx)

    return transactions
```

### Block Explorer

```javascript
// Display all operations in a block
async function getBlockOperations(blockNum) {
    const response = await fetch('http://localhost:8090', {
        method: 'POST',
        body: JSON.stringify({
            jsonrpc: '2.0',
            method: 'account_history_api.get_ops_in_block',
            params: { block_num: blockNum, only_virtual: false },
            id: 1
        })
    });

    const result = await response.json();
    return result.result.ops;
}
```

### Audit Trail

```bash
# Generate CSV of all transfers for an account
generate_transfer_audit() {
    local account=$1

    echo "Timestamp,From,To,Amount,Memo"

    curl -s http://localhost:8090 -d "{
        \"jsonrpc\": \"2.0\",
        \"method\": \"account_history_api.get_account_history\",
        \"params\": {\"account\": \"$account\", \"start\": -1, \"limit\": 1000},
        \"id\": 1
    }" | jq -r '.result.history[] |
        select(.op[0] == "transfer") |
        [.timestamp, .op[1].from, .op[1].to, .op[1].amount, .op[1].memo] |
        @csv'
}
```

## Troubleshooting

### Plugin Not Responding

**Problem**: API calls return "method not found"

**Solution**: Ensure both state and API plugins are enabled:
```ini
plugin = account_history account_history_api
```

### Incomplete History

**Problem**: Some operations are missing from account history

**Causes**:
1. Account tracking not enabled for that account
2. History started after account was created
3. Specific operation types filtered out

**Solution**:
```bash
# Replay blockchain with full tracking
steemd --replay-blockchain \
  --plugin="account_history account_history_api"
```

### High Memory Usage

**Problem**: Node using too much RAM

**Solution**: Switch to RocksDB implementation:
```ini
plugin = account_history_rocksdb account_history_api
```

### Slow Queries

**Problem**: API calls taking too long

**Solutions**:
- Reduce limit parameter
- Use pagination instead of large single queries
- Add caching layer
- Consider upgrading hardware (SSD, more RAM)

## Security Considerations

### Rate Limiting

Implement rate limiting for public nodes:
```nginx
limit_req_zone $binary_remote_addr zone=history_limit:10m rate=5r/s;

location / {
    limit_req zone=history_limit burst=10;
    proxy_pass http://localhost:8090;
}
```

### Resource Limits

Prevent resource exhaustion:
- Maximum limit is capped at 1000 operations per query
- Queries complete within read lock timeout (1 second)
- Consider additional application-level limits

### Privacy Considerations

All operations are public blockchain data. Account history reveals:
- Transfer amounts and recipients
- Voting patterns
- Content creation activity
- Timing of operations

## Integration Examples

### TypeScript/JavaScript

```typescript
interface OperationObject {
    trx_id: string;
    block: number;
    trx_in_block: number;
    op_in_trx: number;
    virtual_op: number;
    timestamp: string;
    op: [string, any];
}

async function getAccountHistory(
    account: string,
    start: number = -1,
    limit: number = 100
): Promise<Map<number, OperationObject>> {
    const response = await fetch('http://localhost:8090', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
            jsonrpc: '2.0',
            method: 'account_history_api.get_account_history',
            params: { account, start, limit },
            id: 1
        })
    });

    const result = await response.json();
    return new Map(Object.entries(result.result.history));
}
```

## Related Documentation

- [account_history Plugin](account_history.md) - In-memory state plugin
- [account_history_rocksdb Plugin](account_history_rocksdb.md) - RocksDB state plugin
- [database_api](database_api.md) - Core blockchain data queries
- [block_api](block_api.md) - Block data queries

## Source Code

- **API Definition**: [src/plugins/apis/account_history_api/include/steem/plugins/account_history_api/account_history_api.hpp](../../src/plugins/apis/account_history_api/include/steem/plugins/account_history_api/account_history_api.hpp)
- **Implementation**: [src/plugins/apis/account_history_api/account_history_api_plugin.cpp](../../src/plugins/apis/account_history_api/account_history_api_plugin.cpp)

## License

See [LICENSE.md](../../LICENSE.md)
