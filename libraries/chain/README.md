# Chain Library

Core blockchain implementation containing consensus rules, database logic, and operation evaluators.

## Overview

The `chain` library is the heart of the Steem blockchain. It implements:
- Block validation and consensus rules
- Database schema and object persistence
- Operation evaluators (transaction processing logic)
- Fork handling and reorganization
- Witness scheduling
- Reward distribution

## Key Components

### Database (`database.cpp`, `database.hpp`)
Central database class managing the blockchain state using memory-mapped storage via chainbase.

**Key Responsibilities:**
- Block application and validation
- Transaction processing
- State management and persistence
- Index maintenance
- Signal emission for plugins

**Note:** `database.cpp` has the longest compile time - it's built first to optimize parallel builds.

### Evaluators
Operation evaluators implement the business logic for each operation type:
- `steem_evaluator.cpp` - Core Steem operations
- `smt_evaluator.cpp` - Smart Media Token operations

Each evaluator validates and applies state changes for its operation type.

### Objects
Database objects representing blockchain state:
- `account_object.hpp` - User accounts and balances
- `comment_object.hpp` - Posts and comments
- `witness_object.hpp` - Witness data
- `global_property_object.hpp` - Global blockchain properties
- And many more in `include/steem/chain/`

### Block Log (`block_log.cpp`)
Persistent storage for the immutable block history. Allows replay and recovery.

### Fork Database (`fork_database.cpp`)
Manages competing blockchain forks and determines the canonical chain.

### Witness Schedule (`witness_schedule.cpp`)
Determines block production order and witness rotation.

### Utilities
- `util/reward.cpp` - Reward calculation algorithms
- `util/impacted.cpp` - Track accounts impacted by operations
- `util/advanced_benchmark_dumper.cpp` - Performance profiling

## Database Schema

Objects are stored in chainbase using multi-index containers. Key indexes include:
- Accounts by name, ID, authority
- Comments by author/permlink
- Votes by voter/comment
- Witnesses by name, vote count

## Build

```bash
cd build
make steem_chain
```

## Dependencies

- `steem_protocol` - Operation and type definitions
- `steem_jsonball` - JSON processing
- `chainbase` - Memory-mapped database
- `fc` - Foundational utilities
- `appbase` - Plugin framework

## Usage

The chain library is primarily used by:
- `steemd` - Main node daemon
- `chain_test` - Unit tests
- Plugins that need direct database access

## Development Notes

- All state changes must go through evaluators
- Use chainbase transactions for atomicity
- Objects are copy-on-write for undo/redo support
- Indexes are automatically maintained by chainbase
- Hardfork checks control consensus rule changes
