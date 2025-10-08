# Account By Key Plugin

Maps public keys to account names for account discovery.

## Overview

The account_by_key plugin:
- Indexes accounts by their public keys
- Enables key-to-account lookups
- Supports account recovery workflows
- Tracks authority key changes

## Configuration

```ini
plugin = account_by_key
```

No additional configuration required.

## Features

- **Key Mapping**: Public key → account name lookups
- **Authority Tracking**: Monitors owner/active/posting keys
- **Recovery Support**: Used in account recovery processes
- **Multi-sig Support**: Tracks all keys in authorities

## Use Cases

- Account discovery by public key
- Account recovery operations
- Multi-signature account management
- Key rotation tracking

## Dependencies

- `chain` - Account data access

## Used By

- `account_by_key_api` - Key lookup API
