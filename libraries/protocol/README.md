# Protocol Library

Defines the Steem blockchain protocol including operations, types, and serialization.

## Overview

The `protocol` library is a foundational layer that defines:
- All blockchain operations
- Data types and structures
- Serialization rules
- Transaction format
- Authority and signature validation
- Protocol constants and configuration

This library is **pure protocol** - it contains no business logic or state management.

## Key Components

### Operations (`steem_operations.cpp`, `steem_operations.hpp`)
Defines all Steem operations:
- Account operations (create, update, delete)
- Content operations (comment, vote, delete_comment)
- Financial operations (transfer, escrow, convert)
- Witness operations (witness_update, feed_publish)
- Power operations (delegate, withdraw_vesting)

### SMT Operations (`smt_operations.cpp`)
Smart Media Token operations for creating and managing custom tokens.

### Types (`types.cpp`)
Core protocol types:
- `asset` - Token amounts with symbol
- `price` - Exchange rates
- `authority` - Multi-sig permissions
- `public_key_type` - Cryptographic keys

### Transactions (`transaction.cpp`)
Transaction structure and validation:
- `signed_transaction` - User-signed transactions
- `transaction` - Base transaction type
- Signature verification
- Expiration handling

### Authority (`authority.cpp`)
Permission system for accounts:
- Owner authority (account recovery)
- Active authority (transfers, posts)
- Posting authority (social actions)
- Multi-signature support

### Sign State (`sign_state.cpp`)
Signature validation and authority checking.

### Configuration (`config.hpp`)
Protocol constants and parameters:
- Block timing
- Inflation parameters
- Network parameters
- Asset symbols (STEEM, SBD, VESTS)
- Hardfork versions

### Hardfork Management (`hardfork.d/`)
Hardfork definitions are generated from parts in `hardfork.d/`:
```bash
python3 -m steem_build_helpers.cat_parts hardfork.d/ hardfork.hpp
```

## Operation Structure

All operations follow this pattern:

```cpp
struct transfer_operation
{
   account_name_type from;
   account_name_type to;
   asset             amount;
   string            memo;

   void validate() const;  // Syntax validation
};
```

Operations are evaluated by corresponding evaluators in the `chain` library.

## Serialization

Uses FC reflection for binary serialization:

```cpp
FC_REFLECT( transfer_operation, (from)(to)(amount)(memo) )
```

This enables:
- Binary packing for network transmission
- JSON conversion for APIs
- Type introspection

## Asset Symbols

Three native assets:
- **STEEM** - Liquid token
- **SBD** - Steem Backed Dollars (stablecoin)
- **VESTS** - Steem Power (vesting shares)

## Configuration Modes

### Testnet
```cpp
#define IS_TEST_NET
```
- Faster block times
- Different chain ID
- Reduced cashout windows

### Mainnet
Default production configuration.

## Build

```bash
cd build
make steem_protocol
```

## Dependencies

- `fc` - Foundational C++ utilities
- `chainbase` (header-only)

## Usage

Used by:
- `steem_chain` - Implements protocol logic
- `steem_wallet` - Transaction creation
- All plugins - Operation handling
- Client libraries - Transaction signing

## Development Notes

- Operations should be stateless (no database access)
- `validate()` only checks syntax, not state
- Use `FC_REFLECT` for all serializable types
- Add new operations at end to maintain compatibility
- Hardfork changes require version bumps
