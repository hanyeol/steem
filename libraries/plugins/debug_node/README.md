# Debug Node Plugin

Development and testing utilities.

## Overview

The debug_node plugin provides:
- Manual block generation
- State manipulation
- Debug operations
- Testing utilities

**WARNING**: For development/testing only. Never use in production!

## Configuration

```ini
plugin = debug_node

# Requires testnet build
# cmake -DBUILD_STEEM_TESTNET=ON
```

## Features

- **Block Generation**: Create blocks on demand
- **State Control**: Manipulate blockchain state
- **Time Control**: Advance blockchain time
- **Debug Operations**: Special testing operations

## Use Cases

- Plugin development
- Operation testing
- Scenario simulation
- Integration testing

## Security

**CRITICAL**:
- Only for testnet/development
- Never enable on production nodes
- Provides complete blockchain control
- Can break consensus

## Dependencies

- `chain` - State access

## Used By

- `debug_node_api` - Debug commands
- Testing frameworks
