# database_api Plugin

Core API for querying blockchain state, accounts, witnesses, and all fundamental blockchain data.

## Overview

The `database_api` plugin provides comprehensive read-only access to all blockchain state data. This is the primary API for accessing account information, witness data, blockchain configuration, and performing transaction validation. It's essential for wallets, explorers, and any application that needs to read blockchain state.

**Plugin Type**: API (Read-only)
**Dependencies**: [chain](chain.md), [json_rpc](json_rpc.md)
**State Plugin**: None (reads directly from chain database)
**Memory**: Minimal (~20MB)
**Default**: Not enabled

## Purpose

- **Account Queries**: Retrieve account data, balances, and permissions
- **Witness Information**: Query witness schedules and voting
- **Blockchain State**: Get global properties and configuration
- **Content Access**: Read posts, comments, and votes
- **Market Data**: Query limit orders and market state
- **Transaction Validation**: Verify signatures and authorities
- **Configuration**: Access blockchain parameters and constants

## Configuration

### Enable Plugin

```ini
# In config.ini
plugin = database_api
```

### Command Line

```bash
steemd --plugin=database_api
```

### No Additional Configuration

The `database_api` plugin requires no special configuration. It provides read-only access to the chain database.

## API Method Categories

### Global State

#### get_config

Retrieve compile-time blockchain constants.

**Parameters**: None

**Returns**: Configuration object with constants like `STEEM_SYMBOL`, `STEEM_BLOCK_INTERVAL`, etc.

#### get_dynamic_global_properties

Get current blockchain state.

**Parameters**: None

**Returns**:
```json
{
  "head_block_number": 50000000,
  "head_block_id": "02faf080...",
  "time": "2023-01-15T12:34:56",
  "current_witness": "witness-account",
  "total_vesting_fund_steem": "192345678.901 STEEM",
  "total_vesting_shares": "391234567890.123456 VESTS",
  "current_supply": "271234567.890 STEEM",
  "current_sbd_supply": "15123456.789 SBD",
  "maximum_block_size": 65536,
  "last_irreversible_block_num": 49999970
}
```

#### get_witness_schedule

Get the current witness schedule.

**Parameters**: None

**Returns**: Witness schedule object with current shuffled witnesses.

#### get_hardfork_properties

Get hardfork version information.

**Parameters**: None

**Returns**: Current and next hardfork versions and timing.

#### get_reward_funds

Get reward fund information.

**Parameters**: None

**Returns**: Array of reward fund objects.

#### get_current_price_feed

Get current STEEM/SBD price feed.

**Parameters**: None

**Returns**: Current price from witness feeds.

#### get_feed_history

Get price feed history.

**Parameters**: None

**Returns**: Historical price feed data.

### Witnesses

#### list_witnesses

List witnesses with pagination.

**Parameters**:
```json
{
  "start": null,
  "limit": 100,
  "order": "by_name"
}
```

**Returns**: Array of witness objects.

#### find_witnesses

Find specific witnesses by account name.

**Parameters**:
```json
{
  "owners": ["witness1", "witness2"]
}
```

**Returns**: Array of witness objects.

#### list_witness_votes

List witness votes with pagination.

**Parameters**:
```json
{
  "start": null,
  "limit": 100,
  "order": "by_account_witness"
}
```

**Returns**: Array of witness vote objects.

#### get_active_witnesses

Get list of currently active witnesses.

**Parameters**: None

**Returns**: Array of account names of active witnesses.

### Accounts

#### list_accounts

List accounts with pagination and sorting.

**Parameters**:
```json
{
  "start": null,
  "limit": 100,
  "order": "by_name"
}
```

**Orders**: `by_name`, `by_proxy`, `by_next_vesting_withdrawal`

**Returns**: Array of account objects.

#### find_accounts

Find specific accounts by name.

**Parameters**:
```json
{
  "accounts": ["alice", "bob", "charlie"]
}
```

**Returns**: Array of account objects with full details.

#### list_owner_histories

List owner authority change history.

**Parameters**:
```json
{
  "start": null,
  "limit": 100
}
```

**Returns**: Array of owner authority history objects.

#### find_owner_histories

Find owner history for specific account.

**Parameters**:
```json
{
  "owner": "alice"
}
```

**Returns**: Owner authority change history.

#### list_account_recovery_requests

List pending account recovery requests.

**Parameters**:
```json
{
  "start": null,
  "limit": 100,
  "order": "by_account"
}
```

**Returns**: Array of recovery request objects.

#### list_escrows

List escrow transactions.

**Parameters**:
```json
{
  "start": null,
  "limit": 100,
  "order": "by_from_id"
}
```

**Returns**: Array of escrow objects.

#### list_withdraw_vesting_routes

List vesting withdrawal routes.

**Parameters**:
```json
{
  "start": null,
  "limit": 100,
  "order": "by_withdraw_route"
}
```

**Returns**: Array of withdrawal route objects.

#### list_savings_withdrawals

List savings withdrawal requests.

**Parameters**:
```json
{
  "start": null,
  "limit": 100,
  "order": "by_from_id"
}
```

**Returns**: Array of savings withdrawal objects.

#### list_vesting_delegations

List vesting delegations.

**Parameters**:
```json
{
  "start": null,
  "limit": 100,
  "order": "by_delegation"
}
```

**Returns**: Array of delegation objects.

#### list_sbd_conversion_requests

List SBD to STEEM conversion requests.

**Parameters**:
```json
{
  "start": null,
  "limit": 100,
  "order": "by_account"
}
```

**Returns**: Array of conversion request objects.

### Comments

#### list_comments

List comments/posts with pagination.

**Parameters**:
```json
{
  "start": null,
  "limit": 100,
  "order": "by_cashout_time"
}
```

**Orders**: `by_cashout_time`, `by_permlink`, `by_root`, `by_parent`, etc.

**Returns**: Array of comment objects.

#### find_comments

Find specific comments.

**Parameters**:
```json
{
  "comments": [
    ["author1", "permlink1"],
    ["author2", "permlink2"]
  ]
}
```

**Returns**: Array of comment objects.

#### list_votes

List votes on comments.

**Parameters**:
```json
{
  "start": null,
  "limit": 100,
  "order": "by_comment_voter"
}
```

**Returns**: Array of vote objects.

#### find_votes

Find votes for specific comment.

**Parameters**:
```json
{
  "author": "alice",
  "permlink": "my-post"
}
```

**Returns**: Array of vote objects for that comment.

### Market

#### list_limit_orders

List active limit orders.

**Parameters**:
```json
{
  "start": null,
  "limit": 100,
  "order": "by_price"
}
```

**Returns**: Array of limit order objects.

#### find_limit_orders

Find limit orders for specific account.

**Parameters**:
```json
{
  "account": "alice"
}
```

**Returns**: Array of limit orders.

#### get_order_book

Get market order book.

**Parameters**:
```json
{
  "limit": 100
}
```

**Returns**: Order book with bids and asks.

### Transaction Validation

#### get_transaction_hex

Get hexadecimal representation of transaction.

**Parameters**:
```json
{
  "trx": { /* transaction object */ }
}
```

**Returns**: Hex string of serialized transaction.

#### get_required_signatures

Get minimum set of required signatures.

**Parameters**:
```json
{
  "trx": { /* transaction */ },
  "available_keys": ["STM7...", "STM8..."]
}
```

**Returns**: Subset of keys needed to sign.

#### get_potential_signatures

Get all possible signing keys for transaction.

**Parameters**:
```json
{
  "trx": { /* transaction */ }
}
```

**Returns**: All public keys that could sign.

#### verify_authority

Verify transaction has sufficient signatures.

**Parameters**:
```json
{
  "trx": { /* signed transaction */ }
}
```

**Returns**: `true` if valid, throws exception otherwise.

#### verify_account_authority

Verify keys can authorize an account.

**Parameters**:
```json
{
  "account": "alice",
  "signers": ["STM7..."]
}
```

**Returns**: `true` if signers have authority.

#### verify_signatures

Verify signatures against arbitrary hash.

**Parameters**:
```json
{
  "hash": "abc123...",
  "signatures": ["1f2e3d..."],
  "required_owner": [],
  "required_active": ["alice"],
  "required_posting": [],
  "required_other": []
}
```

**Returns**: `true` if signatures valid.

## Usage Examples

### Get Account Info

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "database_api.find_accounts",
  "params": {
    "accounts": ["alice", "bob"]
  },
  "id": 1
}' | jq
```

### Get Current Head Block

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "database_api.get_dynamic_global_properties",
  "params": {},
  "id": 1
}' | jq -r '.result.head_block_number'
```

### List Top Witnesses

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "database_api.list_witnesses",
  "params": {
    "start": null,
    "limit": 21,
    "order": "by_vote_name"
  },
  "id": 1
}' | jq -r '.result.witnesses[].owner'
```

### Find Comment

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "database_api.find_comments",
  "params": {
    "comments": [["alice", "my-first-post"]]
  },
  "id": 1
}' | jq
```

### Verify Transaction Signatures

```python
import requests

def verify_transaction(signed_tx):
    """Verify a signed transaction has valid signatures."""
    payload = {
        "jsonrpc": "2.0",
        "method": "database_api.verify_authority",
        "params": {"trx": signed_tx},
        "id": 1
    }

    response = requests.post("http://localhost:8090", json=payload)
    result = response.json()

    if "result" in result:
        return result["result"]
    else:
        # Exception thrown if invalid
        return False
```

### Get Active Witnesses

```python
def get_active_witnesses():
    """Get currently active witness accounts."""
    payload = {
        "jsonrpc": "2.0",
        "method": "database_api.get_active_witnesses",
        "params": {},
        "id": 1
    }

    response = requests.post("http://localhost:8090", json=payload)
    result = response.json()

    return result["result"]["witnesses"]

witnesses = get_active_witnesses()
print(f"Active witnesses: {', '.join(witnesses)}")
```

### Calculate Account Value

```python
def calculate_account_value(account_name):
    """Calculate total account value in STEEM."""
    # Get account
    payload = {
        "jsonrpc": "2.0",
        "method": "database_api.find_accounts",
        "params": {"accounts": [account_name]},
        "id": 1
    }

    response = requests.post("http://localhost:8090", json=payload)
    account = response.json()["result"]["accounts"][0]

    # Get global properties for conversion
    payload = {
        "jsonrpc": "2.0",
        "method": "database_api.get_dynamic_global_properties",
        "params": {},
        "id": 1
    }

    response = requests.post("http://localhost:8090", json=payload)
    props = response.json()["result"]

    # Parse balances
    balance = float(account["balance"]["amount"]) / 1000  # STEEM
    sbd_balance = float(account["sbd_balance"]["amount"]) / 1000  # SBD
    vests = float(account["vesting_shares"]["amount"]) / 1000000

    # Convert VESTS to STEEM
    total_vests = float(props["total_vesting_shares"]["amount"]) / 1000000
    total_steem = float(props["total_vesting_fund_steem"]["amount"]) / 1000
    steem_power = vests * (total_steem / total_vests)

    total = balance + sbd_balance + steem_power

    return {
        "balance": balance,
        "sbd_balance": sbd_balance,
        "steem_power": steem_power,
        "total": total
    }
```

### Paginate Through Accounts

```javascript
async function getAllAccounts() {
    const accounts = [];
    let start = null;
    const limit = 1000;

    while (true) {
        const response = await fetch('http://localhost:8090', {
            method: 'POST',
            body: JSON.stringify({
                jsonrpc: '2.0',
                method: 'database_api.list_accounts',
                params: { start, limit, order: 'by_name' },
                id: 1
            })
        });

        const result = await response.json();
        const batch = result.result.accounts;

        if (batch.length === 0) break;

        accounts.push(...batch);

        // Use last account name as start for next page
        start = batch[batch.length - 1].name;

        if (batch.length < limit) break;
    }

    return accounts;
}
```

## Performance Considerations

### Query Limits

All list methods have a maximum limit (default 1000):
- Prevents excessive memory usage
- Ensures queries complete within read lock timeout
- Use pagination for large datasets

### Efficient Queries

```python
# Good: Find specific accounts
find_accounts({"accounts": ["alice", "bob"]})

# Bad: List all then filter
list_accounts({"start": null, "limit": 1000000})
```

### Caching Strategy

Blockchain state changes frequently, but some data is more stable:

```python
# Cache config (never changes)
@lru_cache(maxsize=1)
def get_config():
    return call_api("database_api.get_config", {})

# Cache DGP briefly (changes every 3 seconds)
@lru_cache(maxsize=1)
@timed_cache(seconds=3)
def get_dgp():
    return call_api("database_api.get_dynamic_global_properties", {})

# Don't cache account data (changes frequently)
def get_account(name):
    return call_api("database_api.find_accounts", {"accounts": [name]})
```

## Common Use Cases

### Blockchain Explorer

```typescript
interface BlockchainStats {
    headBlock: number;
    totalAccounts: number;
    activeWitnesses: string[];
    currentSupply: string;
}

async function getBlockchainStats(): Promise<BlockchainStats> {
    const dgp = await callAPI('database_api.get_dynamic_global_properties', {});
    const witnesses = await callAPI('database_api.get_active_witnesses', {});

    return {
        headBlock: dgp.head_block_number,
        totalAccounts: await getAccountCount(),
        activeWitnesses: witnesses.witnesses,
        currentSupply: dgp.current_supply.amount
    };
}
```

### Wallet Balance Display

```python
def format_wallet_balance(account_name):
    """Format account balance for wallet display."""
    accounts = call_api("database_api.find_accounts", {
        "accounts": [account_name]
    })

    if not accounts["accounts"]:
        return None

    account = accounts["accounts"][0]

    return {
        "STEEM": parse_asset(account["balance"]),
        "SBD": parse_asset(account["sbd_balance"]),
        "Savings STEEM": parse_asset(account["savings_balance"]),
        "Savings SBD": parse_asset(account["savings_sbd_balance"]),
        "STEEM Power": parse_asset(account["vesting_shares"])
    }
```

## Troubleshooting

### Empty Results

**Problem**: Query returns empty array

**Causes**:
1. No data matching query
2. Wrong parameter format
3. Invalid start parameter for pagination

**Debug**:
```bash
# Check parameter format
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "database_api.find_accounts",
  "params": {
    "accounts": ["nonexistent"]
  },
  "id": 1
}' | jq
```

### Pagination Issues

**Problem**: Unable to paginate through results

**Solution**: Use correct start parameter:
```python
# Correct pagination
start = None
while True:
    result = list_accounts(start=start, limit=1000, order="by_name")
    if not result["accounts"]:
        break
    start = result["accounts"][-1]["name"]  # Last account name
```

## Related Documentation

- [chain Plugin](chain.md) - Blockchain database
- [block_api](block_api.md) - Block data queries

## Source Code

- **API Definition**: [src/plugins/apis/database_api/include/steem/plugins/database_api/database_api.hpp](../../src/plugins/apis/database_api/include/steem/plugins/database_api/database_api.hpp)
- **Implementation**: [src/plugins/apis/database_api/database_api_plugin.cpp](../../src/plugins/apis/database_api/database_api_plugin.cpp)

## License

See [LICENSE.md](../../LICENSE.md)
