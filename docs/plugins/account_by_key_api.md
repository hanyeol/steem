# account_by_key_api Plugin

Lookup Steem accounts by public key for wallet and identity management.

## Overview

The `account_by_key_api` plugin provides API endpoints to find which accounts are associated with a given public key. This is essential for wallets and applications that need to discover accounts controlled by specific keys.

**Plugin Type**: API (Read-only)
**Dependencies**: [account_by_key](account_by_key.md), [json_rpc](json_rpc.md)
**State Plugin**: account_by_key
**Memory**: Minimal (~10MB depending on index size)
**Default**: Not enabled

## Purpose

- **Key-to-Account Mapping**: Find all accounts that use a specific public key
- **Wallet Account Discovery**: Help wallets discover accounts controlled by imported keys
- **Authority Verification**: Identify accounts where a key has signing authority
- **Multi-Signature Support**: Discover all accounts in multi-sig configurations

## Configuration

### Enable Plugin

```ini
# In config.ini
plugin = account_by_key account_by_key_api
```

### Command Line

```bash
steemd --plugin="account_by_key account_by_key_api"
```

### Requirements

The `account_by_key_api` requires the `account_by_key` state plugin to be enabled. The state plugin maintains an index mapping public keys to account names.

## API Methods

### get_key_references

Retrieve all accounts that reference a set of public keys in any authority (owner, active, or posting).

**Parameters**:
```json
{
  "keys": ["STM7..."]
}
```

**Returns**:
```json
{
  "accounts": [
    ["account1", "account2"],
    ["account3"]
  ]
}
```

The return value is an array of arrays, where each inner array corresponds to the accounts referencing the public key at the same index in the request.

## Usage Examples

### Basic Key Lookup

Find accounts associated with a single public key:

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "account_by_key_api.get_key_references",
  "params": {
    "keys": ["STM7ZkhNhYjJZD8qKzpYQEqJkFyXqJ7wXzPCnEjLKYN9R3VqvBWHw"]
  },
  "id": 1
}' | jq
```

**Response**:
```json
{
  "jsonrpc": "2.0",
  "result": {
    "accounts": [
      ["alice", "bob"]
    ]
  },
  "id": 1
}
```

This shows that the public key is used by accounts "alice" and "bob".

### Multiple Key Lookup

Check multiple keys at once:

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "account_by_key_api.get_key_references",
  "params": {
    "keys": [
      "STM7ZkhNhYjJZD8qKzpYQEqJkFyXqJ7wXzPCnEjLKYN9R3VqvBWHw",
      "STM8ZkhNhYjJZD8qKzpYQEqJkFyXqJ7wXzPCnEjLKYN9R3VqvBWHx"
    ]
  },
  "id": 1
}' | jq
```

**Response**:
```json
{
  "jsonrpc": "2.0",
  "result": {
    "accounts": [
      ["alice", "bob"],
      ["charlie"]
    ]
  },
  "id": 1
}
```

The first key is used by "alice" and "bob", while the second key is used by "charlie".

### Wallet Account Discovery

When importing a private key, wallets can discover all controlled accounts:

```python
import requests
import json

def find_accounts_for_key(public_key):
    """Find all accounts controlled by a public key."""
    payload = {
        "jsonrpc": "2.0",
        "method": "account_by_key_api.get_key_references",
        "params": {"keys": [public_key]},
        "id": 1
    }

    response = requests.post("http://localhost:8090", json=payload)
    result = response.json()

    if "result" in result:
        accounts = result["result"]["accounts"][0]
        return accounts
    return []

# Example usage
pubkey = "STM7ZkhNhYjJZD8qKzpYQEqJkFyXqJ7wXzPCnEjLKYN9R3VqvBWHw"
accounts = find_accounts_for_key(pubkey)
print(f"Accounts using this key: {accounts}")
```

### Empty Result

If no accounts use a key:

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "account_by_key_api.get_key_references",
  "params": {
    "keys": ["STM5UnusedKeyThatNoAccountHas123456789"]
  },
  "id": 1
}' | jq
```

**Response**:
```json
{
  "jsonrpc": "2.0",
  "result": {
    "accounts": [[]]
  },
  "id": 1
}
```

## Key Authority Types

The API searches for keys in all authority types:

**Owner Authority**: Full control, can change all keys
**Active Authority**: Can perform transfers and most operations
**Posting Authority**: Can post, comment, and vote
**Memo Key**: Used for encrypted messages

If a key appears in any of these authorities, the account will be returned.

## Performance Considerations

### Index Size

The account_by_key index grows with:
- Number of accounts
- Number of unique keys
- Multi-signature configurations

**Typical Size**: 5-20 MB for mainnet

### Query Performance

- **Single key lookup**: ~1ms
- **Batch lookup**: ~1-5ms for 10-100 keys
- **Cold start**: First query may be slower after restart

### Best Practices

**Batch Requests**: Query multiple keys in a single request when possible
**Cache Results**: Cache key-to-account mappings in your application
**Rate Limiting**: Implement rate limiting for public-facing services

## Common Use Cases

### Wallet Integration

```javascript
// Discover all accounts when importing a key
async function importPrivateKey(privateKey) {
    const publicKey = derivePublicKey(privateKey);

    const response = await fetch('http://localhost:8090', {
        method: 'POST',
        body: JSON.stringify({
            jsonrpc: '2.0',
            method: 'account_by_key_api.get_key_references',
            params: { keys: [publicKey] },
            id: 1
        })
    });

    const result = await response.json();
    const accounts = result.result.accounts[0];

    console.log(`Found ${accounts.length} account(s): ${accounts.join(', ')}`);
    return accounts;
}
```

### Multi-Signature Detection

```python
def check_multisig_accounts(public_keys):
    """Find accounts that use multiple keys (potential multi-sig)."""
    payload = {
        "jsonrpc": "2.0",
        "method": "account_by_key_api.get_key_references",
        "params": {"keys": public_keys},
        "id": 1
    }

    response = requests.post("http://localhost:8090", json=payload)
    result = response.json()

    all_accounts = set()
    for account_list in result["result"]["accounts"]:
        all_accounts.update(account_list)

    return list(all_accounts)
```

### Authority Verification

```bash
# Check if a key has authority over an account
check_authority() {
    local pubkey=$1
    local account=$2

    accounts=$(curl -s http://localhost:8090 -d "{
        \"jsonrpc\": \"2.0\",
        \"method\": \"account_by_key_api.get_key_references\",
        \"params\": {\"keys\": [\"$pubkey\"]},
        \"id\": 1
    }" | jq -r ".result.accounts[0][]")

    if echo "$accounts" | grep -q "^$account$"; then
        echo "Key has authority over $account"
        return 0
    else
        echo "Key does NOT have authority over $account"
        return 1
    fi
}
```

## Troubleshooting

### Plugin Not Responding

**Problem**: API calls return "method not found"

**Check**:
```bash
# Verify plugin is loaded
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "jsonrpc.get_methods",
  "params": {},
  "id": 1
}' | jq -r '.result[]' | grep account_by_key
```

**Solution**: Ensure both plugins are enabled:
```ini
plugin = account_by_key account_by_key_api
```

### Empty Results for Known Keys

**Problem**: get_key_references returns empty array for a key you know is used

**Possible Causes**:
1. Index not fully built (node still syncing)
2. Key format incorrect (check STM prefix)
3. Account was created after node was started without replay

**Solution**:
- Wait for full sync
- Verify key format
- Replay with account_by_key plugin enabled

### Index Out of Sync

**Problem**: Results don't reflect recent key changes

**Solution**: The index updates in real-time, but if data seems stale:
```bash
# Restart node to rebuild index
steemd --replay-blockchain --plugin="account_by_key account_by_key_api"
```

## Security Considerations

### Public Information

All key-to-account mappings are public blockchain data. The API only exposes information already available on-chain.

### Rate Limiting

For public nodes, implement rate limiting:
```bash
# Using nginx
limit_req_zone $binary_remote_addr zone=api_limit:10m rate=10r/s;
location / {
    limit_req zone=api_limit burst=20;
    proxy_pass http://localhost:8090;
}
```

### Input Validation

Validate public key format before querying:
- Must start with "STM" prefix
- Must be valid base58 encoding
- Length should be appropriate for key type

## Integration Examples

### CLI Wallet

The `cli_wallet` uses this API to discover accounts:

```
import_key <private_key>
# Internally calls account_by_key_api.get_key_references
# Automatically adds all found accounts to wallet
```

### Web Applications

```typescript
interface KeyReference {
    accounts: string[][];
}

async function getAccountsByKey(publicKey: string): Promise<string[]> {
    const response = await fetch('https://api.steemit.com', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
            jsonrpc: '2.0',
            method: 'account_by_key_api.get_key_references',
            params: { keys: [publicKey] },
            id: 1
        })
    });

    const result: { result: KeyReference } = await response.json();
    return result.result.accounts[0] || [];
}
```

## Related Documentation

- [account_by_key Plugin](account_by_key.md) - State plugin that maintains the key index
- [database_api](database_api.md) - Core blockchain data queries
- [Account Authority](../technical-reference/account-authority.md) - Understanding authorities

## Source Code

- **API Definition**: [libraries/plugins/apis/account_by_key_api/include/steem/plugins/account_by_key_api/account_by_key_api.hpp](../../libraries/plugins/apis/account_by_key_api/include/steem/plugins/account_by_key_api/account_by_key_api.hpp)
- **Implementation**: [libraries/plugins/apis/account_by_key_api/account_by_key_api_plugin.cpp](../../libraries/plugins/apis/account_by_key_api/account_by_key_api_plugin.cpp)

## License

See [LICENSE.md](../../LICENSE.md)
