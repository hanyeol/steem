# Account History API Plugin

Account transaction history queries.

## Overview

Exposes account history data from `account_history` or `account_history_rocksdb` plugins.

## Configuration

```ini
plugin = account_history_api

# Requires state plugin
plugin = account_history
# OR
plugin = account_history_rocksdb
```

## Methods

- `get_account_history` - Account operation history
- `get_ops_in_block` - Operations in specific block
- `enum_virtual_ops` - Virtual operation enumeration

## Usage

```bash
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"account_history_api.get_account_history",
  "params":{"account":"alice", "start":-1, "limit":100},
  "id":1
}' http://localhost:8090
```

## Dependencies

- `account_history` OR `account_history_rocksdb`
- `json_rpc`
- `webserver`
