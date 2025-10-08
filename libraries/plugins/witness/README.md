# Witness Plugin

Block production plugin for witness nodes.

## Overview

The witness plugin enables block production for elected witnesses. It:
- Signs and produces blocks on schedule
- Publishes price feeds
- Manages witness properties
- Handles witness key management

## Configuration

```ini
plugin = witness

# Witness account name
witness = "your-witness-name"

# Private signing key
private-key = WIF_PRIVATE_KEY

# Enable block production even on stale chain (dev only)
enable-stale-production = false

# Required witness participation (0-99)
required-participation = 33
```

## Features

- **Block Production**: Create blocks when scheduled
- **Price Feeds**: Publish STEEM:SBD price feeds
- **Witness Updates**: Modify witness properties
- **Key Management**: Secure signing key handling

## Witness Operations

### Publishing Price Feeds

```bash
# Witnesses should publish regular price feeds
# Set via witness_api or cli_wallet
```

### Updating Witness Properties

```bash
# Update witness URL, block signing key, etc.
# Done via witness_api.update_witness
```

## Security

**Critical**:
- Keep private keys secure
- Use separate signing keys (not active/owner)
- Monitor block production
- Run on reliable infrastructure

## Dependencies

- `chain` - Block creation
- `p2p` - Block propagation

## Required By

- `witness_api` - Witness management API
