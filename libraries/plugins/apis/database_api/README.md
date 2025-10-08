# Database API Plugin

Core blockchain data access API.

## Overview

The database_api provides read access to:
- Blockchain state
- Accounts and balances
- Content (posts/comments)
- Witnesses and votes
- Global properties
- Block and transaction data

## Configuration

```ini
plugin = database_api

# Make public (optional)
public-api = database_api login_api
```

## Key Methods

### Blockchain Info
- `get_dynamic_global_properties` - Current blockchain state
- `get_config` - Blockchain configuration
- `get_hardfork_version` - Current hardfork

### Accounts
- `get_accounts` - Account data
- `lookup_account_names` - Find accounts
- `get_account_count` - Total accounts

### Content
- `get_content` - Post/comment data
- `get_content_replies` - Comment replies
- `get_discussions_by_*` - Content queries

### Witnesses
- `get_witnesses` - Witness data
- `get_witness_by_account` - Witness lookup
- `get_active_witnesses` - Current schedule

### Blocks
- `get_block` - Block data
- `get_block_header` - Block header
- `get_ops_in_block` - Operations in block

## Usage Example

```bash
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"database_api.get_dynamic_global_properties",
  "params":{},
  "id":1
}' http://localhost:8090
```

## Dependencies

- `chain` - Blockchain data
- `json_rpc` - Request handling
- `webserver` - HTTP/WebSocket

## Notes

Most commonly used API. Essential for all Steem applications.
