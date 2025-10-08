# Chain Plugin

Core plugin that initializes and manages the blockchain database.

## Overview

The chain plugin is **required** for all Steem nodes. It:
- Initializes the blockchain database (chainbase)
- Manages block processing and validation
- Provides access to blockchain state
- Emits signals for blockchain events
- Handles blockchain configuration

## Features

- Block application and validation
- Database initialization and management
- Fork detection and resolution
- State persistence
- Event notification system

## Configuration

### Required Settings

```ini
# Always enabled - cannot be disabled
plugin = chain

# Shared memory configuration
shared-file-dir = blockchain
shared-file-size = 54G

# Blockchain replay options (if needed)
# replay-blockchain = true
# resync-blockchain = true
```

### Optional Settings

```ini
# Flush shared memory to disk every N blocks (0 = disabled)
flush-state-interval = 0

# Block checkpoints for validation
# checkpoint = ["1000000", "BLOCK_ID"]
```

## Signals

The chain plugin emits signals that other plugins can connect to:

- `pre_apply_block` - Before block is applied
- `post_apply_block` - After block is applied
- `pre_apply_transaction` - Before transaction is applied
- `post_apply_transaction` - After transaction is applied
- `pre_apply_operation` - Before operation is applied
- `post_apply_operation` - After operation is applied

## Usage

Other plugins access the database through the chain plugin:

```cpp
auto& db = app().get_plugin<chain_plugin>().db();

db.with_read_lock([&]() {
   const auto& account = db.get<account_object>("alice");
   return account.balance;
});
```

## Dependencies

None - this is the foundational plugin.

## Required By

Almost all other plugins depend on the chain plugin.
