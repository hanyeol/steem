# Account History Plugin

Tracks transaction history for accounts.

## Overview

The account_history plugin:
- Records all operations affecting accounts
- Maintains operation history indices
- Supports filtering by operation type
- Enables account activity queries

## Configuration

```ini
plugin = account_history

# Track specific account ranges
# account-history-track-account-range = ["a","z"]

# Whitelist specific operations
# account-history-whitelist-ops = transfer_operation vote_operation

# Blacklist specific operations
# account-history-blacklist-ops = custom_json_operation
```

## Features

- **Full History**: Complete operation history per account
- **Operation Filtering**: Whitelist/blacklist operations
- **Account Ranges**: Track specific account name ranges
- **Efficient Indexing**: Fast history lookups

## Memory Usage

Account history can consume significant memory. For lower memory usage:
- Use operation whitelists
- Track limited account ranges
- Consider `account_history_rocksdb` plugin instead

## Dependencies

- `chain` - Operation processing

## Used By

- `account_history_api` - Exposes history data
