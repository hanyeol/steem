# condenser_api Plugin

Legacy API providing backward compatibility with Steem's original API format.

## Overview

The `condenser_api` plugin provides a unified API interface that aggregates multiple specialized APIs into a single endpoint with the original Steem API format. This ensures backward compatibility with existing applications, wallets, and services built for the original Steem implementation.

**Plugin Type**: API (Read-only + Broadcasting)
**Dependencies**: [database_api](database_api.md), [block_api](block_api.md), [json_rpc](json_rpc.md)
**Optional Dependencies**: account_history_api, account_by_key_api, network_broadcast_api, tags_api, follow_api, reputation_api, market_history_api, witness_api
**Memory**: Minimal (~20MB)
**Default**: Not enabled

## Purpose

- **Backward Compatibility**: Support legacy applications without code changes
- **Unified Interface**: Single API for all common operations
- **Simplified Integration**: Easier for new developers to get started
- **Historical Support**: Maintain compatibility with existing tools
- **Comprehensive Access**: Access all major blockchain features

## Configuration

### Enable Plugin

```ini
# In config.ini - requires database_api at minimum
plugin = database_api condenser_api

# For full functionality, enable all optional plugins
plugin = database_api block_api account_history_api account_by_key_api \
         network_broadcast_api tags_api follow_api reputation_api \
         market_history_api witness_api condenser_api
```

### Command Line

```bash
# Minimal
steemd --plugin="database_api condenser_api"

# Full-featured
steemd --plugin="database_api block_api account_history_api \
  account_by_key_api network_broadcast_api tags_api follow_api \
  reputation_api market_history_api witness_api condenser_api"
```

## API Categories

The condenser_api aggregates methods from multiple plugins:

### Core Blockchain Data
- `get_dynamic_global_properties`
- `get_config`
- `get_chain_properties`
- `get_hardfork_version`
- `get_witness_schedule`

### Account Operations
- `get_accounts`
- `lookup_account_names`
- `lookup_accounts`
- `get_account_count`
- `get_account_history`

### Content & Discussions
- `get_content`
- `get_content_replies`
- `get_discussions_by_trending`
- `get_discussions_by_created`
- `get_discussions_by_hot`

### Witness & Voting
- `get_witnesses`
- `get_witness_by_account`
- `get_active_witnesses`
- `get_witness_count`

### Market Data
- `get_order_book`
- `get_ticker`
- `get_volume`
- `get_trade_history`

### Social Features
- `get_followers`
- `get_following`
- `get_follow_count`
- `get_blog`
- `get_feed`

### Transaction Broadcasting
- `broadcast_transaction`
- `broadcast_transaction_synchronous`
- `broadcast_block`

## Usage Examples

### Get Dynamic Global Properties

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "condenser_api.get_dynamic_global_properties",
  "params": [],
  "id": 1
}' | jq
```

**Response**:
```json
{
  "jsonrpc": "2.0",
  "result": {
    "head_block_number": 50000000,
    "head_block_id": "02faf080...",
    "time": "2023-01-15T12:34:56",
    "current_witness": "witness-account",
    "total_pow": 514415,
    "num_pow_witnesses": 172,
    "virtual_supply": "283123456.789 STEEM",
    "current_supply": "271234567.890 STEEM",
    "current_sbd_supply": "15123456.789 SBD",
    "total_vesting_fund_steem": "192345678.901 STEEM",
    "total_vesting_shares": "391234567890.123456 VESTS"
  },
  "id": 1
}
```

### Get Account Information

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "condenser_api.get_accounts",
  "params": [["alice", "bob"]],
  "id": 1
}' | jq
```

### Get Blog Posts

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "condenser_api.get_discussions_by_blog",
  "params": [{"tag": "alice", "limit": 10}],
  "id": 1
}' | jq
```

### Get Account History

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "condenser_api.get_account_history",
  "params": ["alice", -1, 10],
  "id": 1
}' | jq
```

### Broadcast Transaction

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "condenser_api.broadcast_transaction",
  "params": [{
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
    "signatures": ["1f2e3d4c5b..."]
  }],
  "id": 1
}' | jq
```

### Get Market Order Book

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "condenser_api.get_order_book",
  "params": [10],
  "id": 1
}' | jq
```

### Get Followers

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "condenser_api.get_followers",
  "params": ["alice", "", "blog", 100],
  "id": 1
}' | jq
```

## Key Differences from Modern APIs

### Legacy Format

The condenser_api uses the original parameter format:

```javascript
// Condenser API (legacy format)
condenser_api.get_accounts([["alice", "bob"]])

// Modern database_api format
database_api.find_accounts({"accounts": ["alice", "bob"]})
```

### Asset Format

Uses legacy asset representation:

```json
// Condenser format
"balance": "123.456 STEEM"

// Modern format
"balance": {"amount": "123456", "precision": 3, "nai": "@@000000021"}
```

### Return Types

Returns data in the original format expected by legacy applications.

## Common Use Cases

### Wallet Integration

```python
import requests

class SteemWallet:
    def __init__(self, url="http://localhost:8090"):
        self.url = url

    def get_account(self, username):
        """Get account information using condenser_api."""
        payload = {
            "jsonrpc": "2.0",
            "method": "condenser_api.get_accounts",
            "params": [[username]],
            "id": 1
        }

        response = requests.post(self.url, json=payload)
        result = response.json()

        if "result" in result and result["result"]:
            return result["result"][0]
        return None

    def get_balance(self, username):
        """Get account balance."""
        account = self.get_account(username)
        if account:
            return {
                "balance": account["balance"],
                "sbd_balance": account["sbd_balance"],
                "vesting_shares": account["vesting_shares"]
            }
        return None

# Usage
wallet = SteemWallet()
balance = wallet.get_balance("alice")
print(f"STEEM: {balance['balance']}")
print(f"SBD: {balance['sbd_balance']}")
```

### Blog Reader

```javascript
async function getBlogPosts(author, limit = 10) {
    const response = await fetch('http://localhost:8090', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
            jsonrpc: '2.0',
            method: 'condenser_api.get_discussions_by_blog',
            params: [{ tag: author, limit: limit }],
            id: 1
        })
    });

    const result = await response.json();
    return result.result || [];
}

// Usage
const posts = await getBlogPosts('alice', 5);
posts.forEach(post => {
    console.log(`${post.title} by ${post.author}`);
    console.log(`Votes: ${post.net_votes}`);
});
```

### Market Monitor

```python
def monitor_market():
    """Monitor STEEM/SBD market."""
    payload = {
        "jsonrpc": "2.0",
        "method": "condenser_api.get_ticker",
        "params": [],
        "id": 1
    }

    response = requests.post("http://localhost:8090", json=payload)
    result = response.json()

    if "result" in result:
        ticker = result["result"]
        print(f"Latest price: {ticker['latest']}")
        print(f"24h volume (STEEM): {ticker['steem_volume']}")
        print(f"24h volume (SBD): {ticker['sbd_volume']}")
        print(f"Change: {ticker['percent_change']}%")

    return result.get("result")
```

### Follow Tracker

```bash
# Get all followers for an account
get_all_followers() {
    local account=$1
    local start=""

    while true; do
        followers=$(curl -s http://localhost:8090 -d "{
            \"jsonrpc\": \"2.0\",
            \"method\": \"condenser_api.get_followers\",
            \"params\": [\"$account\", \"$start\", \"blog\", 1000],
            \"id\": 1
        }" | jq -r '.result[]?.follower')

        if [ -z "$followers" ]; then
            break
        fi

        echo "$followers"

        # Get last follower for pagination
        start=$(echo "$followers" | tail -1)
    done
}
```

## Performance Considerations

### Method Availability

Some methods require optional plugins to be enabled:

| Method | Required Plugin |
|--------|----------------|
| get_account_history | account_history_api |
| get_key_references | account_by_key_api |
| broadcast_transaction | network_broadcast_api |
| get_discussions_by_* | tags_api |
| get_followers | follow_api |
| get_account_reputations | reputation_api |
| get_ticker | market_history_api |

If a required plugin is not enabled, the method will return an error.

### Query Limits

- Most list methods have a maximum limit (typically 1000)
- Discussion queries limited to prevent abuse
- Account history limited to 1000 operations per call

### Caching Recommendations

```python
from functools import lru_cache
from datetime import datetime, timedelta

class CachedCondenser:
    def __init__(self):
        self.cache = {}
        self.cache_duration = timedelta(seconds=30)

    def get_dynamic_global_properties(self):
        """Get global properties with short cache."""
        now = datetime.now()

        if 'dgp' in self.cache:
            cached_time, cached_data = self.cache['dgp']
            if now - cached_time < self.cache_duration:
                return cached_data

        # Fetch fresh data
        payload = {
            "jsonrpc": "2.0",
            "method": "condenser_api.get_dynamic_global_properties",
            "params": [],
            "id": 1
        }

        response = requests.post("http://localhost:8090", json=payload)
        result = response.json()["result"]

        self.cache['dgp'] = (now, result)
        return result
```

## Migration from condenser_api

For new applications, consider migrating to specialized APIs:

```python
# Old (condenser_api)
result = call_api("condenser_api.get_accounts", [["alice"]])

# New (database_api)
result = call_api("database_api.find_accounts", {
    "accounts": ["alice"]
})
```

Benefits of specialized APIs:
- More consistent parameter format
- Better typing and validation
- More efficient queries
- Modern asset representation

## Troubleshooting

### Method Not Available

**Problem**: Method returns error even though condenser_api is enabled

**Cause**: Required underlying plugin not enabled

**Solution**: Enable the required plugin:
```ini
# For get_account_history
plugin = account_history account_history_api condenser_api

# For get_followers
plugin = follow follow_api condenser_api

# For get_ticker
plugin = market_history market_history_api condenser_api
```

### Asset Format Issues

**Problem**: Asset amounts appear in unexpected format

**Cause**: Some methods return legacy format, others modern format

**Solution**: Use condenser_api consistently or convert formats:
```python
def parse_legacy_asset(asset_str):
    """Parse '123.456 STEEM' to amount and symbol."""
    parts = asset_str.split()
    amount = float(parts[0])
    symbol = parts[1]
    return {"amount": amount, "symbol": symbol}
```

### Empty Results

**Problem**: Queries return empty arrays

**Possible Causes**:
1. Account/content doesn't exist
2. Wrong parameter order
3. Required plugin not tracking that data

**Debug**:
```bash
# Check if account exists
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "condenser_api.lookup_accounts",
  "params": ["alice", 1],
  "id": 1
}' | jq
```

## Security Considerations

### Input Validation

Always validate user input before querying:

```python
def safe_get_account(username):
    """Safely get account with validation."""
    # Validate username format
    import re
    if not re.match(r'^[a-z][a-z0-9\-\.]{2,15}$', username):
        raise ValueError("Invalid account name")

    payload = {
        "jsonrpc": "2.0",
        "method": "condenser_api.get_accounts",
        "params": [[username]],
        "id": 1
    }

    response = requests.post("http://localhost:8090", json=payload)
    return response.json()
```

### Rate Limiting

Implement rate limiting for public APIs:

```nginx
limit_req_zone $binary_remote_addr zone=condenser_limit:10m rate=10r/s;

location / {
    limit_req zone=condenser_limit burst=20;
    proxy_pass http://localhost:8090;
}
```

## Integration Examples

### TypeScript

```typescript
interface DynamicGlobalProperties {
    head_block_number: number;
    head_block_id: string;
    time: string;
    current_witness: string;
    total_vesting_fund_steem: string;
    total_vesting_shares: string;
}

async function getDynamicGlobalProperties(): Promise<DynamicGlobalProperties> {
    const response = await fetch('http://localhost:8090', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
            jsonrpc: '2.0',
            method: 'condenser_api.get_dynamic_global_properties',
            params: [],
            id: 1
        })
    });

    const result = await response.json();
    return result.result;
}
```

## Related Documentation

- [database_api](database_api.md) - Modern core blockchain API
- [account_history_api](account_history_api.md) - Account history queries
- [tags_api](tags_api.md) - Content and discussion queries
- [follow_api](follow_api.md) - Social features
- [market_history_api](market_history_api.md) - Market data

## Source Code

- **API Definition**: [libraries/plugins/apis/condenser_api/include/steem/plugins/condenser_api/condenser_api.hpp](../../libraries/plugins/apis/condenser_api/include/steem/plugins/condenser_api/condenser_api.hpp)
- **Implementation**: [libraries/plugins/apis/condenser_api/condenser_api_plugin.cpp](../../libraries/plugins/apis/condenser_api/condenser_api_plugin.cpp)

## License

See [LICENSE.md](../../LICENSE.md)
