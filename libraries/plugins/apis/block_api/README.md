# Block API Plugin

Block data access.

## Overview

Provides block-specific queries.

## Configuration

```ini
plugin = block_api
```

## Methods

- `get_block` - Full block data
- `get_block_header` - Block header only
- `get_block_range` - Multiple blocks

## Usage

```bash
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"block_api.get_block",
  "params":{"block_num":1000000},
  "id":1
}' http://localhost:8090
```

## Dependencies

- `chain` - Block data
- `json_rpc`
- `webserver`
