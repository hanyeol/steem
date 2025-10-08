# Reputation API Plugin

Account reputation queries.

## Overview

Exposes reputation calculation data.

## Configuration

```ini
plugin = reputation_api

# Requires state plugin
plugin = reputation
```

## Methods

- `get_account_reputations` - Account reputation scores

## Dependencies

- `reputation`
- `json_rpc`
- `webserver`
