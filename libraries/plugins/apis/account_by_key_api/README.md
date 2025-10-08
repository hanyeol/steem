# Account By Key API Plugin

Public key to account name lookups.

## Overview

Provides key-based account discovery.

## Configuration

```ini
plugin = account_by_key_api

# Requires state plugin
plugin = account_by_key
```

## Methods

- `get_key_references` - Find accounts by public key

## Usage

```bash
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"account_by_key_api.get_key_references",
  "params":{"keys":["STM..."]},
  "id":1
}' http://localhost:8090
```

## Use Cases

- Account recovery
- Key-based account lookup
- Multi-sig account discovery

## Dependencies

- `account_by_key`
- `json_rpc`
- `webserver`
