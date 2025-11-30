# block_api Plugin

Query block headers and full block data from the Steem blockchain.

## Overview

The `block_api` plugin provides API endpoints to retrieve block data from the blockchain. This is essential for blockchain explorers, synchronization tools, and applications that need to access raw block information.

**Plugin Type**: API (Read-only)
**Dependencies**: [chain](chain.md), [json_rpc](json_rpc.md)
**State Plugin**: None (reads directly from chain)
**Memory**: Minimal (~10MB)
**Default**: Not enabled

## Purpose

- **Block Retrieval**: Get full block data including all transactions and operations
- **Header Queries**: Retrieve just block headers for lightweight sync
- **Blockchain Exploration**: Power block explorers and analysis tools
- **Verification**: Validate block signatures and witness schedules
- **Synchronization**: Support custom blockchain sync implementations

## Configuration

### Enable Plugin

```ini
# In config.ini
plugin = block_api
```

### Command Line

```bash
steemd --plugin=block_api
```

### No Special Configuration

The `block_api` plugin requires no additional configuration. It reads directly from the blockchain database maintained by the chain plugin.

## API Methods

### get_block_header

Retrieve only the header of a block (lightweight query).

**Parameters**:
```json
{
  "block_num": 50000000
}
```

**Returns**:
```json
{
  "header": {
    "previous": "02faf07f...",
    "timestamp": "2023-01-15T12:34:56",
    "witness": "witness-account",
    "transaction_merkle_root": "0000000000000000000000000000000000000000",
    "extensions": []
  }
}
```

**Returns `null` if block does not exist.**

### get_block

Retrieve a full signed block including all transactions and operations.

**Parameters**:
```json
{
  "block_num": 50000000
}
```

**Returns**:
```json
{
  "block": {
    "previous": "02faf07f...",
    "timestamp": "2023-01-15T12:34:56",
    "witness": "witness-account",
    "transaction_merkle_root": "abc123...",
    "extensions": [],
    "witness_signature": "1f2e3d...",
    "transactions": [
      {
        "ref_block_num": 61567,
        "ref_block_prefix": 123456789,
        "expiration": "2023-01-15T12:35:56",
        "operations": [
          ["vote", {
            "voter": "alice",
            "author": "bob",
            "permlink": "test-post",
            "weight": 10000
          }]
        ],
        "extensions": [],
        "signatures": ["1f2e3d..."]
      }
    ],
    "block_id": "02faf080...",
    "signing_key": "STM7...",
    "transaction_ids": ["abc123..."]
  }
}
```

**Returns `null` if block does not exist.**

## Usage Examples

### Get Block Header

Retrieve just the header for quick metadata access:

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "block_api.get_block_header",
  "params": {
    "block_num": 50000000
  },
  "id": 1
}' | jq
```

**Response**:
```json
{
  "jsonrpc": "2.0",
  "result": {
    "header": {
      "previous": "02faf07f1234567890abcdef...",
      "timestamp": "2023-01-15T12:34:56",
      "witness": "steemit",
      "transaction_merkle_root": "0000000000000000000000000000000000000000",
      "extensions": []
    }
  },
  "id": 1
}
```

### Get Full Block

Retrieve complete block with all transactions:

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "block_api.get_block",
  "params": {
    "block_num": 50000000
  },
  "id": 1
}' | jq
```

### Check if Block Exists

```bash
block_exists() {
    local block_num=$1

    result=$(curl -s http://localhost:8090 -d "{
        \"jsonrpc\": \"2.0\",
        \"method\": \"block_api.get_block_header\",
        \"params\": {\"block_num\": $block_num},
        \"id\": 1
    }" | jq -r '.result.header')

    if [ "$result" = "null" ]; then
        echo "Block $block_num does not exist"
        return 1
    else
        echo "Block $block_num exists"
        return 0
    fi
}
```

### Extract Block Timestamp

```python
import requests
from datetime import datetime

def get_block_timestamp(block_num):
    """Get the timestamp when a block was produced."""
    payload = {
        "jsonrpc": "2.0",
        "method": "block_api.get_block_header",
        "params": {"block_num": block_num},
        "id": 1
    }

    response = requests.post("http://localhost:8090", json=payload)
    result = response.json()

    if result["result"]["header"]:
        timestamp_str = result["result"]["header"]["timestamp"]
        return datetime.fromisoformat(timestamp_str.replace('Z', '+00:00'))
    return None

# Example usage
timestamp = get_block_timestamp(50000000)
print(f"Block produced at: {timestamp}")
```

### Count Transactions in Block

```python
def count_transactions(block_num):
    """Count the number of transactions in a block."""
    payload = {
        "jsonrpc": "2.0",
        "method": "block_api.get_block",
        "params": {"block_num": block_num},
        "id": 1
    }

    response = requests.post("http://localhost:8090", json=payload)
    result = response.json()

    if result["result"]["block"]:
        return len(result["result"]["block"]["transactions"])
    return 0

# Example usage
tx_count = count_transactions(50000000)
print(f"Block contains {tx_count} transactions")
```

### Verify Block Signature

```python
def get_block_witness(block_num):
    """Get the witness who produced a block."""
    payload = {
        "jsonrpc": "2.0",
        "method": "block_api.get_block_header",
        "params": {"block_num": block_num},
        "id": 1
    }

    response = requests.post("http://localhost:8090", json=payload)
    result = response.json()

    if result["result"]["header"]:
        return result["result"]["header"]["witness"]
    return None
```

### Sync Block Range

```javascript
async function syncBlockRange(startBlock, endBlock) {
    const blocks = [];

    for (let num = startBlock; num <= endBlock; num++) {
        const response = await fetch('http://localhost:8090', {
            method: 'POST',
            body: JSON.stringify({
                jsonrpc: '2.0',
                method: 'block_api.get_block',
                params: { block_num: num },
                id: 1
            })
        });

        const result = await response.json();
        if (result.result.block) {
            blocks.push(result.result.block);
        }

        // Small delay to avoid overwhelming node
        await new Promise(resolve => setTimeout(resolve, 10));
    }

    return blocks;
}

// Example usage
const blocks = await syncBlockRange(50000000, 50000100);
console.log(`Downloaded ${blocks.length} blocks`);
```

### Extract All Operations

```python
def extract_all_operations(block_num):
    """Extract all operations from a block."""
    payload = {
        "jsonrpc": "2.0",
        "method": "block_api.get_block",
        "params": {"block_num": block_num},
        "id": 1
    }

    response = requests.post("http://localhost:8090", json=payload)
    result = response.json()

    operations = []
    if result["result"]["block"]:
        for tx in result["result"]["block"]["transactions"]:
            for op in tx["operations"]:
                operations.append({
                    "type": op[0],
                    "data": op[1]
                })

    return operations

# Example usage
ops = extract_all_operations(50000000)
for op in ops:
    print(f"{op['type']}: {op['data']}")
```

## Block Structure

### Block Header

**Fields**:
- `previous`: Hash of previous block (links the chain)
- `timestamp`: When block was produced (3-second intervals)
- `witness`: Account name of witness who produced the block
- `transaction_merkle_root`: Merkle root of all transactions
- `extensions`: Future protocol extensions

### Full Block

**Additional Fields**:
- `witness_signature`: Cryptographic signature of witness
- `transactions`: Array of all transactions in the block
- `block_id`: Unique identifier for this block
- `signing_key`: Public key used to sign the block
- `transaction_ids`: Array of transaction IDs

### Transaction Structure

**Fields**:
- `ref_block_num`: Reference block number (for TaPoS)
- `ref_block_prefix`: Reference block prefix (for TaPoS)
- `expiration`: When transaction expires
- `operations`: Array of operations in transaction
- `extensions`: Future extensions
- `signatures`: Cryptographic signatures

## Performance Considerations

### Query Performance

- **get_block_header**: ~1-5ms (lightweight, only header data)
- **get_block**: ~5-50ms (full block with all transactions)
- **Large blocks**: Blocks with many transactions take longer

### Caching Strategy

Blocks are immutable once produced, making them ideal for caching:

```python
from functools import lru_cache

@lru_cache(maxsize=1000)
def get_cached_block(block_num):
    """Cache block data since blocks never change."""
    payload = {
        "jsonrpc": "2.0",
        "method": "block_api.get_block",
        "params": {"block_num": block_num},
        "id": 1
    }

    response = requests.post("http://localhost:8090", json=payload)
    return response.json()["result"]["block"]
```

### Batch Processing

When processing many blocks, use batch requests:

```python
import asyncio
import aiohttp

async def get_blocks_batch(block_nums):
    """Fetch multiple blocks concurrently."""
    async with aiohttp.ClientSession() as session:
        tasks = []
        for block_num in block_nums:
            payload = {
                "jsonrpc": "2.0",
                "method": "block_api.get_block",
                "params": {"block_num": block_num},
                "id": block_num
            }
            tasks.append(session.post("http://localhost:8090", json=payload))

        responses = await asyncio.gather(*tasks)
        blocks = []
        for response in responses:
            result = await response.json()
            if result["result"]["block"]:
                blocks.append(result["result"]["block"])

        return blocks
```

## Common Use Cases

### Block Explorer

```javascript
// Display block information
async function displayBlockInfo(blockNum) {
    const response = await fetch('http://localhost:8090', {
        method: 'POST',
        body: JSON.stringify({
            jsonrpc: '2.0',
            method: 'block_api.get_block',
            params: { block_num: blockNum },
            id: 1
        })
    });

    const result = await response.json();
    const block = result.result.block;

    if (block) {
        console.log(`Block ${blockNum}`);
        console.log(`Witness: ${block.witness}`);
        console.log(`Timestamp: ${block.timestamp}`);
        console.log(`Transactions: ${block.transactions.length}`);
        console.log(`Block ID: ${block.block_id}`);
    }
}
```

### Witness Schedule Verification

```python
def verify_witness_schedule(start_block, count=21):
    """Verify witness schedule by checking consecutive blocks."""
    witnesses = []

    for block_num in range(start_block, start_block + count):
        payload = {
            "jsonrpc": "2.0",
            "method": "block_api.get_block_header",
            "params": {"block_num": block_num},
            "id": 1
        }

        response = requests.post("http://localhost:8090", json=payload)
        result = response.json()

        if result["result"]["header"]:
            witnesses.append(result["result"]["header"]["witness"])

    print(f"Witnesses for blocks {start_block} to {start_block + count - 1}:")
    for i, witness in enumerate(witnesses):
        print(f"  Block {start_block + i}: {witness}")

    return witnesses
```

### Transaction Search

```bash
# Find all transactions in a block
find_transactions() {
    local block_num=$1

    curl -s http://localhost:8090 -d "{
        \"jsonrpc\": \"2.0\",
        \"method\": \"block_api.get_block\",
        \"params\": {\"block_num\": $block_num},
        \"id\": 1
    }" | jq -r '.result.block.transactions[] | @json'
}
```

### Block Time Analysis

```python
from datetime import datetime

def analyze_block_times(start_block, end_block):
    """Analyze block production times."""
    timestamps = []

    for block_num in range(start_block, end_block + 1):
        payload = {
            "jsonrpc": "2.0",
            "method": "block_api.get_block_header",
            "params": {"block_num": block_num},
            "id": 1
        }

        response = requests.post("http://localhost:8090", json=payload)
        result = response.json()

        if result["result"]["header"]:
            ts_str = result["result"]["header"]["timestamp"]
            ts = datetime.fromisoformat(ts_str.replace('Z', '+00:00'))
            timestamps.append(ts)

    # Calculate intervals
    intervals = []
    for i in range(1, len(timestamps)):
        delta = (timestamps[i] - timestamps[i-1]).total_seconds()
        intervals.append(delta)

    avg_interval = sum(intervals) / len(intervals)
    print(f"Average block interval: {avg_interval:.2f} seconds")
    print(f"Expected: 3.0 seconds")

    return intervals
```

## Troubleshooting

### Block Not Found

**Problem**: Query returns `null` for a block number

**Possible Causes**:
1. Block number is in the future (hasn't been produced yet)
2. Node hasn't synced to that block yet
3. Block number is invalid (negative or zero)

**Check**:
```bash
# Get current head block
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "database_api.get_dynamic_global_properties",
  "params": {},
  "id": 1
}' | jq -r '.result.head_block_number'
```

### Slow Block Queries

**Problem**: Block queries taking too long

**Solutions**:
- Use `get_block_header` instead of `get_block` when possible
- Implement caching for frequently accessed blocks
- Add indexes if using custom storage
- Check node resources (CPU, disk I/O)

### Incomplete Block Data

**Problem**: Transactions missing from blocks

**Cause**: Most likely node is still syncing

**Solution**: Wait for full sync and verify:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "database_api.get_dynamic_global_properties",
  "params": {},
  "id": 1
}' | jq
```

## Security Considerations

### Block Validation

Always verify block signatures in security-critical applications:
- Check witness signature matches signing key
- Verify transaction signatures
- Validate merkle root matches transactions

### Rate Limiting

For public nodes, implement rate limiting:
```nginx
limit_req_zone $binary_remote_addr zone=block_limit:10m rate=20r/s;

location / {
    limit_req zone=block_limit burst=50;
    proxy_pass http://localhost:8090;
}
```

### Resource Protection

Large block queries can be resource-intensive:
- Implement timeouts
- Monitor query patterns
- Cache frequently accessed blocks

## Integration Examples

### TypeScript

```typescript
interface BlockHeader {
    previous: string;
    timestamp: string;
    witness: string;
    transaction_merkle_root: string;
    extensions: any[];
}

interface Block extends BlockHeader {
    witness_signature: string;
    transactions: Transaction[];
    block_id: string;
    signing_key: string;
    transaction_ids: string[];
}

async function getBlock(blockNum: number): Promise<Block | null> {
    const response = await fetch('http://localhost:8090', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
            jsonrpc: '2.0',
            method: 'block_api.get_block',
            params: { block_num: blockNum },
            id: 1
        })
    });

    const result = await response.json();
    return result.result.block;
}
```

## Related Documentation

- [chain Plugin](chain.md) - Blockchain database
- [database_api](database_api.md) - Core blockchain data queries
- [account_history_api](account_history_api.md) - Operation history queries
- [System Architecture](../technical-reference/system-architecture.md) - Overall architecture

## Source Code

- **API Definition**: [src/plugins/apis/block_api/include/steem/plugins/block_api/block_api.hpp](../../src/plugins/apis/block_api/include/steem/plugins/block_api/block_api.hpp)
- **Implementation**: [src/plugins/apis/block_api/block_api_plugin.cpp](../../src/plugins/apis/block_api/block_api_plugin.cpp)

## License

See [LICENSE.md](../../LICENSE.md)
