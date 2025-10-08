# Debug Node API Plugin

Development and testing API.

## Overview

Provides debug operations for testing.

**WARNING**: Development only. Never use in production!

## Configuration

```ini
plugin = debug_node_api

# Requires debug_node plugin and testnet build
plugin = debug_node
```

## Methods

- `debug_generate_blocks` - Generate blocks manually
- `debug_push_blocks` - Push specific blocks
- `debug_*` - Various debug operations

## Security

**CRITICAL**:
- Only for testnet/development
- Provides complete blockchain control
- Can break consensus
- Never enable on production

## Dependencies

- `debug_node`
- `json_rpc`
- `webserver`
- Requires BUILD_STEEM_TESTNET=ON
