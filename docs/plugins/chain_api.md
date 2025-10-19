# chain_api Plugin

Push blocks and broadcast transactions directly to the Steem blockchain.

## Overview

The `chain_api` plugin provides API endpoints for submitting blocks and transactions to the blockchain. This is primarily used by witnesses to broadcast blocks they produce, and by nodes for transaction propagation and testing.

**Plugin Type**: API (Write Operations)
**Dependencies**: [chain](chain.md), [json_rpc](json_rpc.md)
**State Plugin**: None (writes directly to chain)
**Memory**: Minimal (~10MB)
**Default**: Not enabled

## Purpose

- **Block Submission**: Witnesses push newly produced blocks
- **Transaction Broadcasting**: Submit signed transactions to the network
- **Testing**: Test transaction validation and block processing
- **Replay**: Replay blocks during synchronization
- **P2P Integration**: Support peer-to-peer block propagation

## Configuration

### Enable Plugin

```ini
# In config.ini
plugin = chain_api
```

### Command Line

```bash
steemd --plugin=chain_api
```

### Security Warning

The `chain_api` allows direct block and transaction submission. For production nodes:
- **Witnesses**: Enable only on trusted private networks
- **Public APIs**: Usually disabled for security
- **Seed Nodes**: Should NOT enable this plugin

## API Methods

### push_block

Submit a signed block to the blockchain.

**Parameters**:
```json
{
  "block": {
    "previous": "02faf07f...",
    "timestamp": "2023-01-15T12:34:56",
    "witness": "witness-account",
    "transaction_merkle_root": "abc123...",
    "extensions": [],
    "witness_signature": "1f2e3d...",
    "transactions": []
  },
  "currently_syncing": false
}
```

**Returns**:
```json
{
  "success": true,
  "error": null
}
```

**On Error**:
```json
{
  "success": false,
  "error": "Invalid block signature"
}
```

**Parameters**:
- `block`: Full signed block object
- `currently_syncing`: If true, skip some validations during sync

### push_transaction

Submit a signed transaction to the blockchain.

**Parameters**:
```json
{
  "ref_block_num": 61567,
  "ref_block_prefix": 123456789,
  "expiration": "2023-01-15T12:35:56",
  "operations": [
    ["transfer", {
      "from": "alice",
      "to": "bob",
      "amount": "10.000 STEEM",
      "memo": "Payment"
    }]
  ],
  "extensions": [],
  "signatures": ["1f2e3d4c5b..."]
}
```

**Returns**:
```json
{
  "success": true,
  "error": null
}
```

**On Error**:
```json
{
  "success": false,
  "error": "insufficient_balance: alice's balance is insufficient"
}
```

## Usage Examples

### Push a Transaction

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "chain_api.push_transaction",
  "params": {
    "ref_block_num": 12345,
    "ref_block_prefix": 987654321,
    "expiration": "2023-12-31T23:59:59",
    "operations": [
      ["vote", {
        "voter": "alice",
        "author": "bob",
        "permlink": "test-post",
        "weight": 10000
      }]
    ],
    "extensions": [],
    "signatures": ["1f2e3d4c5b6a7989..."]
  },
  "id": 1
}' | jq
```

### Broadcast Block (Witness)

```bash
# Typically called by witness software after producing a block
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "chain_api.push_block",
  "params": {
    "block": {
      "previous": "02faf07f1234567890abcdef...",
      "timestamp": "2023-01-15T12:34:56",
      "witness": "my-witness",
      "transaction_merkle_root": "0000000000000000000000000000000000000000",
      "extensions": [],
      "witness_signature": "1f2e3d4c...",
      "transactions": []
    },
    "currently_syncing": false
  },
  "id": 1
}' | jq
```

### Test Transaction Validation

```python
import requests
import json

def test_transaction(transaction):
    """Test if a transaction would be accepted."""
    payload = {
        "jsonrpc": "2.0",
        "method": "chain_api.push_transaction",
        "params": transaction,
        "id": 1
    }

    response = requests.post("http://localhost:8090", json=payload)
    result = response.json()

    if "result" in result:
        if result["result"]["success"]:
            print("Transaction is valid and was accepted")
            return True
        else:
            print(f"Transaction rejected: {result['result']['error']}")
            return False
    else:
        print(f"Error: {result.get('error', 'Unknown error')}")
        return False

# Example transaction
tx = {
    "ref_block_num": 12345,
    "ref_block_prefix": 987654321,
    "expiration": "2023-12-31T23:59:59",
    "operations": [
        ["transfer", {
            "from": "alice",
            "to": "bob",
            "amount": "1.000 STEEM",
            "memo": "Test payment"
        }]
    ],
    "extensions": [],
    "signatures": ["1f2e3d4c5b..."]
}

test_transaction(tx)
```

### Handle Push Errors

```python
def push_transaction_safe(transaction):
    """Push transaction with error handling."""
    payload = {
        "jsonrpc": "2.0",
        "method": "chain_api.push_transaction",
        "params": transaction,
        "id": 1
    }

    try:
        response = requests.post("http://localhost:8090", json=payload, timeout=5)
        result = response.json()

        if "result" in result:
            if result["result"]["success"]:
                return {"status": "success", "error": None}
            else:
                return {"status": "failed", "error": result["result"]["error"]}
        else:
            return {"status": "error", "error": result.get("error", "Unknown")}

    except requests.exceptions.Timeout:
        return {"status": "error", "error": "Request timeout"}
    except Exception as e:
        return {"status": "error", "error": str(e)}
```

## Transaction Structure

### Required Fields

**ref_block_num**: Reference block number (TaPoS - for replay protection)
**ref_block_prefix**: First 4 bytes of reference block ID
**expiration**: When transaction expires (typically 1 hour from creation)
**operations**: Array of operations to execute
**signatures**: Array of cryptographic signatures

### Transaction as Proof of Stake (TaPoS)

TaPoS prevents replay attacks on forks:
- Transaction references a recent block
- Transaction only valid on chains containing that block
- Prevents executing transaction on alternative forks

```python
def create_tapos_params(block):
    """Create TaPoS parameters from a block."""
    import struct

    ref_block_num = block["block_num"] & 0xFFFF
    ref_block_prefix = struct.unpack_from("<I", bytes.fromhex(block["block_id"]), 4)[0]

    return {
        "ref_block_num": ref_block_num,
        "ref_block_prefix": ref_block_prefix
    }
```

## Validation Process

### Transaction Validation

When you push a transaction, the node validates:

1. **Signature Verification**: All required signatures present and valid
2. **Authority Check**: Signers have required authority
3. **Balance Check**: Sufficient balance for transfers
4. **Expiration**: Transaction not expired
5. **TaPoS**: Reference block exists
6. **Operation Validation**: Each operation passes validation
7. **Resource Check**: Bandwidth and RC available

### Block Validation

When pushing a block:

1. **Previous Block**: Exists and matches previous hash
2. **Witness Signature**: Valid signature from scheduled witness
3. **Timestamp**: Within acceptable time range
4. **Transaction Merkle Root**: Matches included transactions
5. **Transaction Validation**: All transactions valid
6. **Witness Schedule**: Witness was scheduled for this slot

## Performance Considerations

### Transaction Propagation

- **Local Validation**: ~1-10ms
- **Network Propagation**: ~500ms-2s to reach network
- **Block Inclusion**: Next block (within 3 seconds if in pool)

### Block Processing

- **Empty Block**: ~5-20ms
- **Full Block**: ~50-200ms depending on transaction count
- **Sync Mode**: Faster with `currently_syncing=true`

## Common Use Cases

### Witness Block Production

```javascript
// Simplified witness block production
async function produceBlock(witness, timestamp, transactions) {
    const block = {
        previous: await getPreviousBlockId(),
        timestamp: timestamp,
        witness: witness,
        transaction_merkle_root: calculateMerkleRoot(transactions),
        extensions: [],
        transactions: transactions
    };

    // Sign block with witness key
    const signature = signBlock(block, witnessPrivateKey);
    block.witness_signature = signature;

    // Push to network
    const response = await fetch('http://localhost:8090', {
        method: 'POST',
        body: JSON.stringify({
            jsonrpc: '2.0',
            method: 'chain_api.push_block',
            params: {
                block: block,
                currently_syncing: false
            },
            id: 1
        })
    });

    return await response.json();
}
```

### Transaction Broadcasting

```python
from datetime import datetime, timedelta

def broadcast_transfer(from_account, to_account, amount, memo, private_key):
    """Create and broadcast a transfer transaction."""
    # Get TaPoS parameters
    block = get_head_block()
    tapos = create_tapos_params(block)

    # Create transaction
    tx = {
        "ref_block_num": tapos["ref_block_num"],
        "ref_block_prefix": tapos["ref_block_prefix"],
        "expiration": (datetime.utcnow() + timedelta(hours=1)).isoformat(),
        "operations": [
            ["transfer", {
                "from": from_account,
                "to": to_account,
                "amount": amount,
                "memo": memo
            }]
        ],
        "extensions": []
    }

    # Sign transaction
    signature = sign_transaction(tx, private_key)
    tx["signatures"] = [signature]

    # Broadcast
    result = push_transaction_safe(tx)
    return result
```

### Batch Transaction Submission

```python
def submit_batch_transactions(transactions):
    """Submit multiple transactions efficiently."""
    results = []

    for tx in transactions:
        result = push_transaction_safe(tx)
        results.append({
            "transaction": tx,
            "result": result
        })

        # Small delay to avoid overwhelming node
        time.sleep(0.1)

    success_count = sum(1 for r in results if r["result"]["status"] == "success")
    print(f"Submitted {len(transactions)} transactions, {success_count} successful")

    return results
```

## Troubleshooting

### Transaction Rejected

**Problem**: Transaction push returns `success: false`

**Common Errors**:

1. **"insufficient_balance"**: Account doesn't have enough funds
   - Check account balance before transfer
   - Account for transaction fees

2. **"missing required active authority"**: Missing or invalid signature
   - Verify signature is correct
   - Check using correct private key

3. **"transaction tapos exception"**: Invalid TaPoS parameters
   - Ensure reference block still exists
   - Use recent block for TaPoS (within last ~64k blocks)

4. **"transaction expiration exception"**: Transaction expired
   - Set expiration time in future
   - Typical: current time + 1 hour

5. **"duplicate transaction"**: Transaction already submitted
   - Transaction is already in pending pool or blockchain
   - Create new transaction

### Block Push Failed

**Problem**: Block push rejected

**Common Causes**:
- Invalid witness signature
- Witness not scheduled for this timeslot
- Block timestamp incorrect
- Previous block hash doesn't match

**Debug**:
```bash
# Check witness schedule
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "database_api.get_witness_schedule",
  "params": {},
  "id": 1
}' | jq '.result.current_shuffled_witnesses'
```

### Plugin Not Responding

**Problem**: API returns "method not found"

**Solution**:
```ini
# Ensure plugin is enabled
plugin = chain_api
```

## Security Considerations

### Access Control

**Never expose chain_api on public nodes:**

```nginx
# Block chain_api methods on public endpoint
location / {
    if ($request_body ~* "chain_api") {
        return 403;
    }
    proxy_pass http://localhost:8090;
}
```

### Witness Security

For witness nodes:
- Only enable on private network
- Use firewall to restrict access
- Connect only to trusted peers

```ini
# Witness configuration
plugin = chain_api witness

# Disable public access
webserver-http-endpoint = 127.0.0.1:8090
webserver-ws-endpoint = 127.0.0.1:8091
```

### Transaction Signing

Always sign transactions securely:
- Never expose private keys
- Sign transactions client-side
- Use hardware wallets for high-value accounts

### Rate Limiting

If you must expose chain_api:
```nginx
limit_req_zone $binary_remote_addr zone=chain_limit:10m rate=1r/s;

location / {
    limit_req zone=chain_limit burst=2;
    proxy_pass http://localhost:8090;
}
```

## Integration Examples

### Python

```python
import requests
from datetime import datetime, timedelta

class SteemChainAPI:
    def __init__(self, url="http://localhost:8090"):
        self.url = url

    def push_transaction(self, tx):
        payload = {
            "jsonrpc": "2.0",
            "method": "chain_api.push_transaction",
            "params": tx,
            "id": 1
        }

        response = requests.post(self.url, json=payload)
        result = response.json()

        if "result" in result:
            return result["result"]
        else:
            raise Exception(result.get("error", "Unknown error"))

    def push_block(self, block, syncing=False):
        payload = {
            "jsonrpc": "2.0",
            "method": "chain_api.push_block",
            "params": {
                "block": block,
                "currently_syncing": syncing
            },
            "id": 1
        }

        response = requests.post(self.url, json=payload)
        result = response.json()

        if "result" in result:
            return result["result"]
        else:
            raise Exception(result.get("error", "Unknown error"))
```

## Related Documentation

- [chain Plugin](chain.md) - Blockchain database
- [network_broadcast_api](network_broadcast_api.md) - Transaction broadcasting (recommended)
- [witness Plugin](witness.md) - Witness block production
- [p2p Plugin](p2p.md) - Network propagation

## Source Code

- **API Definition**: [libraries/plugins/apis/chain_api/include/steem/plugins/chain_api/chain_api.hpp](../../libraries/plugins/apis/chain_api/include/steem/plugins/chain_api/chain_api.hpp)
- **Implementation**: [libraries/plugins/apis/chain_api/chain_api_plugin.cpp](../../libraries/plugins/apis/chain_api/chain_api_plugin.cpp)

## License

See [LICENSE.md](../../LICENSE.md)
