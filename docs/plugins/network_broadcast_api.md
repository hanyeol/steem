# network_broadcast_api Plugin

Broadcast signed transactions and blocks to the Steem network.

## Overview

The `network_broadcast_api` plugin provides the interface for submitting signed transactions and blocks to the peer-to-peer network. This is the primary mechanism for applications to interact with the blockchain by broadcasting operations.

**Plugin Type**: API Plugin
**Dependencies**: [chain](chain.md), [p2p](p2p.md), [json_rpc](json_rpc.md)
**Memory**: Minimal
**Default**: Disabled

## Purpose

- **Transaction Broadcasting**: Submit signed transactions to the network
- **Block Broadcasting**: Propagate newly produced blocks (witness use)
- **Operation Submission**: Enable all blockchain interactions (transfers, posts, votes, etc.)
- **Network Propagation**: Distribute transactions/blocks to connected peers

## Configuration

### Enable Plugin

```ini
# In config.ini
plugin = network_broadcast_api
```

Or via command line:

```bash
steemd --plugin=network_broadcast_api
```

### Prerequisites

This plugin requires core networking plugins:

```ini
# Required dependencies
plugin = chain
plugin = p2p
plugin = network_broadcast_api
```

These are typically enabled by default in most node configurations.

### No Configuration Options

This plugin has no configurable parameters. All network settings are handled by the `p2p` plugin.

## Security Considerations

### Public Nodes

**Warning**: Enabling `network_broadcast_api` allows anyone with network access to broadcast transactions through your node.

**Risk Mitigation**:

1. **Firewall restrictions** - Limit API access
```bash
# Only allow specific IPs
iptables -A INPUT -p tcp --dport 8090 -s 203.0.113.0/24 -j ACCEPT
iptables -A INPUT -p tcp --dport 8090 -j DROP
```

2. **Reverse proxy** - Add authentication layer
```nginx
location / {
    auth_basic "Restricted";
    auth_basic_user_file /etc/nginx/.htpasswd;
    proxy_pass http://localhost:8090;
}
```

3. **Separate nodes** - Use dedicated nodes for broadcasting

### Rate Limiting

Nodes may implement rate limiting to prevent abuse:
- Transaction flood protection
- Bandwidth throttling
- Connection limits

Check your node's resource usage when exposing this API publicly.

## API Methods

All methods are called via JSON-RPC at the endpoint configured by the `webserver` plugin (default: `http://localhost:8090`).

### broadcast_transaction

Broadcast a signed transaction to the network.

**Request Parameters**:
- `trx` (signed_transaction) - Signed transaction object
- `max_block_age` (int32, optional) - Maximum age in blocks (-1 = no limit, default: -1)

**Returns**: void (success or throws exception)

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"network_broadcast_api.broadcast_transaction",
  "params":{
    "trx":{
      "ref_block_num":12345,
      "ref_block_prefix":678901234,
      "expiration":"2025-10-19T12:30:00",
      "operations":[
        [
          "transfer",
          {
            "from":"alice",
            "to":"bob",
            "amount":"10.000 STEEM",
            "memo":"Payment for services"
          }
        ]
      ],
      "extensions":[],
      "signatures":[
        "1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a"
      ]
    },
    "max_block_age":-1
  }
}'
```

**Success Response**:
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {}
}
```

**Error Response**:
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "error": {
    "code": -32000,
    "message": "Assert Exception: Insufficient balance",
    "data": {
      "code": 10,
      "name": "assert_exception",
      "message": "Assert Exception",
      "stack": [...]
    }
  }
}
```

### broadcast_block

Broadcast a signed block to the network (witness use only).

**Request Parameters**:
- `block` (signed_block) - Complete signed block

**Returns**: void (success or throws exception)

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"network_broadcast_api.broadcast_block",
  "params":{
    "block":{
      "previous":"00bc614eb...",
      "timestamp":"2025-10-19T12:00:00",
      "witness":"alice",
      "transaction_merkle_root":"0000000000...",
      "extensions":[],
      "witness_signature":"1f2a3b...",
      "transactions":[...]
    }
  }
}'
```

**Use Case**: Only for witness nodes broadcasting newly produced blocks. Most applications will never use this method.

## Transaction Structure

### Required Fields

Every transaction must include:

```javascript
{
  "ref_block_num": 12345,        // Recent block number (lower 16 bits)
  "ref_block_prefix": 678901234,  // Recent block ID prefix
  "expiration": "2025-10-19T12:30:00", // Transaction deadline
  "operations": [...],            // Array of operations
  "extensions": [],               // Usually empty
  "signatures": ["..."]           // Cryptographic signatures
}
```

### Operation Array

Operations are tuples of `[operation_name, operation_params]`:

```javascript
"operations": [
  ["transfer", {
    "from": "alice",
    "to": "bob",
    "amount": "10.000 STEEM",
    "memo": "Hello"
  }],
  ["vote", {
    "voter": "alice",
    "author": "bob",
    "permlink": "example-post",
    "weight": 10000
  }]
]
```

### Expiration

Transactions expire after a configured time (default: 1 hour max):

```javascript
// Set expiration 30 minutes in future
const expiration = new Date(Date.now() + 30 * 60 * 1000);
transaction.expiration = expiration.toISOString().split('.')[0];
```

### Reference Block

Must reference a recent block (within last 65,536 blocks):

```javascript
// Get reference block from database_api
const props = await database_api.get_dynamic_global_properties();
const refBlock = await database_api.get_block({
  block_num: props.head_block_number - 3  // 3 blocks back for safety
});

transaction.ref_block_num = refBlock.block_id.substring(0, 8);
transaction.ref_block_prefix = parseInt(refBlock.block_id.substring(8, 16), 16);
```

### Signatures

Transactions must be signed with the appropriate private key(s):

```javascript
// Using steem-js library
const steem = require('steem');

// Sign transaction
const transaction = {
  ref_block_num: 12345,
  ref_block_prefix: 678901234,
  expiration: "2025-10-19T12:30:00",
  operations: [...],
  extensions: []
};

// Get chain ID
const chainId = "0000000000000000000000000000000000000000000000000000000000000000";

// Sign with active key for transfer
const signatures = [
  steem.auth.signTransaction(transaction, ["5JPrivateKeyHere..."])
];

transaction.signatures = signatures;
```

## Common Operations

### Transfer

Send STEEM or SBD between accounts:

```json
["transfer", {
  "from": "alice",
  "to": "bob",
  "amount": "10.000 STEEM",
  "memo": "Optional encrypted message"
}]
```

### Vote

Vote on a post or comment:

```json
["vote", {
  "voter": "alice",
  "author": "bob",
  "permlink": "example-post",
  "weight": 10000
}]
```

Weight: -10000 (full downvote) to +10000 (full upvote)

### Comment

Create post or comment:

```json
["comment", {
  "parent_author": "",
  "parent_permlink": "tag",
  "author": "alice",
  "permlink": "my-post",
  "title": "My Post Title",
  "body": "Post content...",
  "json_metadata": "{\"tags\":[\"tag\"]}"
}]
```

### Custom JSON

Interact with plugins (follows, etc.):

```json
["custom_json", {
  "required_auths": [],
  "required_posting_auths": ["alice"],
  "id": "follow",
  "json": "{\"follower\":\"alice\",\"following\":\"bob\",\"what\":[\"blog\"]}"
}]
```

## Transaction Lifecycle

### 1. Construction

Application builds transaction with operations:

```javascript
const tx = {
  ref_block_num: ...,
  ref_block_prefix: ...,
  expiration: ...,
  operations: [["transfer", {...}]],
  extensions: []
};
```

### 2. Signing

Transaction signed with appropriate private key:

```javascript
tx.signatures = [signTransaction(tx, privateKey)];
```

### 3. Broadcasting

Submitted via `broadcast_transaction`:

```javascript
await network_broadcast_api.broadcast_transaction({trx: tx});
```

### 4. Network Propagation

Node broadcasts to connected peers (1-2 seconds):

```
Your Node -> Peer 1 -> Peer 2 -> ...
          -> Peer 3 -> Peer 4 -> ...
```

### 5. Inclusion in Block

Witness includes transaction in next block (~3 seconds):

```
Pending Pool -> Block Production -> Blockchain
```

### 6. Execution

Operations execute and modify blockchain state:

```
Block Applied -> Evaluators Run -> State Updated
```

### 7. Irreversibility

After 15 blocks (~45 seconds), transaction is irreversible:

```
Block N -> Block N+15 -> Irreversible
```

## Error Handling

### Validation Errors

Transaction rejected before broadcast:

```json
{
  "error": {
    "message": "Assert Exception: Account 'alice' does not have sufficient funds"
  }
}
```

**Common causes**:
- Insufficient balance
- Invalid signature
- Missing required authority
- Malformed operation

### Network Errors

Transaction fails to propagate:

```json
{
  "error": {
    "message": "Unable to broadcast transaction"
  }
}
```

**Common causes**:
- Node not connected to network
- Expired transaction
- Duplicate transaction

### Best Practices

**Retry logic**:
```javascript
async function broadcastWithRetry(tx, maxRetries = 3) {
  for (let i = 0; i < maxRetries; i++) {
    try {
      await network_broadcast_api.broadcast_transaction({trx: tx});
      return true;
    } catch (error) {
      if (error.message.includes("duplicate")) {
        return true; // Already broadcasted
      }
      if (i === maxRetries - 1) throw error;
      await sleep(1000 * (i + 1)); // Exponential backoff
    }
  }
}
```

**Transaction confirmation**:
```javascript
async function waitForConfirmation(txId, timeout = 60000) {
  const start = Date.now();
  while (Date.now() - start < timeout) {
    try {
      const tx = await database_api.get_transaction({id: txId});
      if (tx) return tx;
    } catch (e) {
      // Transaction not yet in blockchain
    }
    await sleep(3000); // Check every 3 seconds
  }
  throw new Error("Transaction confirmation timeout");
}
```

## Use Cases

### Wallet Application

Enable user transactions:

```ini
plugin = database_api
plugin = network_broadcast_api
plugin = account_by_key_api
```

**Features**:
- Send/receive STEEM/SBD
- Transfer history
- Balance queries

### Social Media Application

Post and vote operations:

```ini
plugin = database_api
plugin = network_broadcast_api
plugin = tags_api
plugin = follow_api
```

**Features**:
- Create posts/comments
- Vote on content
- Follow/unfollow users
- Content feeds

### Exchange Integration

Deposit/withdrawal automation:

```ini
plugin = database_api
plugin = network_broadcast_api
plugin = account_history_api
```

**Features**:
- Monitor deposits
- Process withdrawals
- Account management

### Witness Node

Block production:

```ini
plugin = witness
plugin = network_broadcast_api
```

**Use**: `broadcast_block` to propagate produced blocks

## Performance Notes

### Transaction Pool

Nodes maintain a pending transaction pool:
- Transactions validated before broadcasting
- Duplicate transactions rejected
- Pool cleared on block production

### Bandwidth Limits

Blockchain enforces bandwidth limits:
- Based on account STEEM Power
- Rate limiting per account
- Regenerates over time

Check bandwidth before broadcasting:
```javascript
const bandwidth = await witness_api.get_account_bandwidth({
  account: "alice",
  type: "forum"
});
```

### Mempool Size

Large transaction volumes may fill mempool:
- Transactions with higher fees prioritized (if enabled)
- Old transactions may be evicted
- Witness discretion on inclusion

## Troubleshooting

### "Duplicate transaction" Error

**Problem**: Transaction already in blockchain or mempool

**Causes**:
- Transaction broadcasted multiple times
- Another node broadcast same transaction
- Replay attack (old transaction resubmitted)

**Solution**: Check if transaction is already confirmed. If not, increase `ref_block_num` and re-sign.

### "Transaction expired" Error

**Problem**: Transaction expiration time passed

**Cause**: Transaction not included before expiration

**Solution**: Regenerate transaction with new expiration and reference block.

### "Missing required authority" Error

**Problem**: Signature doesn't match required authority

**Causes**:
- Wrong private key used
- Incorrect authority level (owner vs active vs posting)
- Account doesn't have permission

**Solution**: Verify key corresponds to required authority for operation.

### "Insufficient bandwidth" Error

**Problem**: Account exceeded bandwidth allocation

**Cause**: Too many operations in short period

**Solution**: Wait for bandwidth to regenerate or power up more STEEM.

### Broadcast Succeeds but Transaction Not Confirmed

**Problem**: `broadcast_transaction` returns success but transaction not in block

**Causes**:
- Node not fully synced
- Network partition
- Witness rejected transaction

**Solution**: Query multiple nodes to verify transaction status. May need to re-broadcast with updated reference block.

## Related Documentation

- [p2p Plugin](p2p.md) - Peer-to-peer networking
- [chain Plugin](chain.md) - Blockchain state management
- [database_api](database_api.md) - Query blockchain data
- [witness_api](witness_api.md) - Bandwidth queries
- [Creating Operations](../development/create-operation.md) - Add custom operations

## Source Code

- **API Implementation**: [src/plugins/apis/network_broadcast_api/network_broadcast_api.cpp](../../src/plugins/apis/network_broadcast_api/network_broadcast_api.cpp)
- **API Header**: [src/plugins/apis/network_broadcast_api/include/steem/plugins/network_broadcast_api/network_broadcast_api.hpp](../../src/plugins/apis/network_broadcast_api/include/steem/plugins/network_broadcast_api/network_broadcast_api.hpp)

## License

See [LICENSE.md](../../LICENSE.md)
