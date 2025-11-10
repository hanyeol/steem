# Asset Format and Representation

This document describes how assets are represented in the Steem blockchain, including the NAI (Numeric Asset Identifier) system and various asset formats used in APIs and operations.

## Table of Contents

- [Overview](#overview)
- [Legacy String Format](#legacy-string-format)
- [NAI Format](#nai-format)
- [Core Asset Types](#core-asset-types)
- [Asset Conversion Examples](#asset-conversion-examples)
- [Working with Assets in Code](#working-with-assets-in-code)
- [API Response Formats](#api-response-formats)
- [Best Practices](#best-practices)

## Overview

Steem uses two primary formats for representing assets:

1. **Legacy String Format**: Human-readable format like `"1.000 STEEM"`
2. **NAI Format**: Structured JSON format with numeric asset identifiers

The NAI (Numeric Asset Identifier) system was introduced to support Smart Media Tokens (SMT) and provides a more extensible way to represent different asset types on the blockchain.

## Legacy String Format

The traditional format used in early Steem implementations and still supported for backward compatibility.

### Format

```
<amount> <symbol>
```

### Examples

```
"1.000 STEEM"
"10.000 TBD"
"1000.000000 VESTS"
```

### Rules

- Amount must include decimal point
- Precision must match the asset type:
  - STEEM/TESTS: 3 decimal places
  - SBD/TBD: 3 decimal places
  - VESTS: 6 decimal places
- Space between amount and symbol is required
- Symbol must be uppercase

### Usage in CLI Wallet

```bash
# Transfer using legacy format
transfer alice bob "10.000 TESTS" "memo" true

# Convert STEEM to SBD
convert alice "100.000 TESTS"
```

## NAI Format

The modern structured format used in API responses and internal representations.

### Structure

```json
{
  "amount": "1000",
  "precision": 3,
  "nai": "@@000000021"
}
```

### Fields

| Field | Type | Description |
|-------|------|-------------|
| `amount` | string | Amount in smallest unit (satoshis), without decimal point |
| `precision` | integer | Number of decimal places |
| `nai` | string | Numeric Asset Identifier prefixed with `@@` |

### Example Representations

**1.000 STEEM:**
```json
{
  "amount": "1000",
  "precision": 3,
  "nai": "@@000000021"
}
```

**10.000 TBD:**
```json
{
  "amount": "10000",
  "precision": 3,
  "nai": "@@000000013"
}
```

**1000.000000 VESTS:**
```json
{
  "amount": "1000000000",
  "precision": 6,
  "nai": "@@000000037"
}
```

## Core Asset Types

### STEEM (TESTS in Testnet)

The primary liquid token of the Steem blockchain.

**NAI:** `@@000000021`
**Precision:** 3
**Symbol:** STEEM (mainnet) / TESTS (testnet)

**Example:**
```json
{
  "amount": "250000000000",
  "precision": 3,
  "nai": "@@000000021"
}
```
Represents: `250000000.000 STEEM`

### SBD (TBD in Testnet)

Steem Backed Dollars - a debt instrument pegged to USD.

**NAI:** `@@000000013`
**Precision:** 3
**Symbol:** SBD (mainnet) / TBD (testnet)

**Example:**
```json
{
  "amount": "1000",
  "precision": 3,
  "nai": "@@000000013"
}
```
Represents: `1.000 SBD`

### VESTS

Vesting shares representing Steem Power (locked/staked STEEM).

**NAI:** `@@000000037`
**Precision:** 6
**Symbol:** VESTS

**Example:**
```json
{
  "amount": "1000000000",
  "precision": 6,
  "nai": "@@000000037"
}
```
Represents: `1000.000000 VESTS`

## Asset Conversion Examples

### Converting String to NAI Format

**Input:** `"123.456 STEEM"`

**Process:**
1. Parse amount: `123.456`
2. Determine precision: `3`
3. Convert to satoshis: `123.456 * 1000 = 123456`
4. Look up NAI for STEEM: `@@000000021`

**Output:**
```json
{
  "amount": "123456",
  "precision": 3,
  "nai": "@@000000021"
}
```

### Converting NAI to String Format

**Input:**
```json
{
  "amount": "5000000",
  "precision": 6,
  "nai": "@@000000037"
}
```

**Process:**
1. Parse amount: `5000000`
2. Apply precision: `5000000 / 10^6 = 5.000000`
3. Look up symbol for NAI `@@000000037`: `VESTS`

**Output:** `"5.000000 VESTS"`

### Precision Calculation

The `amount` field stores the value in the smallest unit (satoshis):

```
display_amount = amount / (10 ^ precision)
```

**Examples:**

| Amount (satoshis) | Precision | Display Value |
|-------------------|-----------|---------------|
| 1000 | 3 | 1.000 |
| 123456 | 3 | 123.456 |
| 1000000 | 6 | 1.000000 |
| 5000000000 | 6 | 5000.000000 |

## Working with Assets in Code

### C++ Asset Class

**File:** `libraries/protocol/include/steem/protocol/asset.hpp`

```cpp
// Create an asset
asset steem_amount = asset(1000, STEEM_SYMBOL);  // 1.000 STEEM

// Access components
int64_t amount = steem_amount.amount;  // 1000
asset_symbol_type symbol = steem_amount.symbol;

// String representation
std::string str = steem_amount.to_string();  // "1.000 STEEM"
```

### Asset Symbol Type

```cpp
struct asset_symbol_type
{
    uint8_t  decimals;  // Precision
    uint32_t nai;       // Numeric asset identifier

    // Predefined symbols
    static const asset_symbol_type STEEM_SYMBOL;  // 3 decimals, NAI 21
    static const asset_symbol_type SBD_SYMBOL;    // 3 decimals, NAI 13
    static const asset_symbol_type VESTS_SYMBOL;  // 6 decimals, NAI 37
};
```

### Creating Assets in Tests

```cpp
// Using ASSET macro
asset amount = ASSET("10.000 TESTS");
asset sbd = ASSET("5.000 TBD");
asset vests = ASSET("1000.000000 VESTS");

// Manual construction
asset steem = asset(10000, STEEM_SYMBOL);  // 10.000 STEEM
```

### JSON Serialization

Assets are serialized differently depending on the context:

**Legacy API (condenser_api):**
```json
"balance": "1.000 STEEM"
```

**Modern API (database_api):**
```json
"balance": {
  "amount": "1000",
  "precision": 3,
  "nai": "@@000000021"
}
```

## API Response Formats

### database_api.find_accounts

Returns assets in NAI format:

```json
{
  "accounts": [{
    "name": "alice",
    "balance": {
      "amount": "1000",
      "precision": 3,
      "nai": "@@000000021"
    },
    "sbd_balance": {
      "amount": "500",
      "precision": 3,
      "nai": "@@000000013"
    },
    "vesting_shares": {
      "amount": "1000000000",
      "precision": 6,
      "nai": "@@000000037"
    }
  }]
}
```

### Operation Serialization

Operations use the legacy string format for human readability:

```json
{
  "type": "transfer_operation",
  "value": {
    "from": "alice",
    "to": "bob",
    "amount": "10.000 STEEM",
    "memo": "payment"
  }
}
```

### Block API

Block content may include both formats depending on the specific API call.

## Best Practices

### When to Use Each Format

**Use Legacy String Format:**
- CLI wallet commands
- User-facing displays
- Operation construction
- Human-readable logs

**Use NAI Format:**
- API responses (modern APIs)
- Internal calculations
- Database storage
- Cross-asset comparisons

### Validation

Always validate asset formats before processing:

```cpp
// Validate precision matches asset type
if (amount.symbol.decimals() != 3) {
    throw invalid_asset_exception();
}

// Validate amount is non-negative (unless explicitly allowed)
if (amount.amount < 0) {
    throw negative_amount_exception();
}

// Validate symbol matches expected type
if (amount.symbol != STEEM_SYMBOL) {
    throw wrong_asset_type_exception();
}
```

### Precision Handling

**Always maintain precision when converting:**

```cpp
// WRONG - loses precision
int64_t steem = asset.amount / 1000;

// CORRECT - preserves precision
asset steem = asset(amount_satoshis, STEEM_SYMBOL);
```

**Avoid floating point arithmetic:**

```cpp
// WRONG - floating point errors
double value = 10.000;
asset a = asset(value * 1000, STEEM_SYMBOL);

// CORRECT - integer arithmetic
asset a = asset(10000, STEEM_SYMBOL);
```

### String Parsing

When parsing asset strings:

1. Split on space to separate amount and symbol
2. Validate symbol is recognized
3. Parse amount with exact precision
4. Convert to smallest unit (satoshis)
5. Validate result is within valid range

```cpp
// Example parsing
std::string input = "10.000 STEEM";
auto space_pos = input.find(' ');
std::string amount_str = input.substr(0, space_pos);
std::string symbol_str = input.substr(space_pos + 1);

// Validate and convert
asset result = asset::from_string(input);
```

## SMT and Custom Assets

When SMT (Smart Media Tokens) are enabled, additional asset types can be created with custom NAIs:

### Custom NAI Range

- Core assets: `@@000000001` - `@@000000099`
- SMT assets: `@@000000100` - `@@999999999`

### SMT Asset Example

```json
{
  "amount": "1000000",
  "precision": 4,
  "nai": "@@000000123"
}
```

### Creating SMT Assets

SMT assets are created through special operations and registered on-chain. Each SMT:
- Has a unique NAI
- Defines its own precision (0-12 decimals)
- Has configurable supply and emission rules

## Related Files

- **Asset Definition:** `libraries/protocol/include/steem/protocol/asset.hpp`
- **Asset Implementation:** `libraries/protocol/asset.cpp`
- **Asset Symbol:** `libraries/protocol/include/steem/protocol/asset_symbol.hpp`
- **Serialization:** `libraries/protocol/include/steem/protocol/types.hpp`

## Additional Resources

- [Chain Parameters](chain-parameters.md) - Blockchain parameter reference
- [API Notes](api-notes.md) - API usage guidelines
- [System Architecture](system-architecture.md) - Overall system design

## Summary

- **NAI Format**: Structured JSON with amount (satoshis), precision, and numeric identifier
- **Legacy Format**: Human-readable string like "1.000 STEEM"
- **Core Assets**: STEEM (NAI 21), SBD (NAI 13), VESTS (NAI 37)
- **Precision**: Always use integer arithmetic in smallest unit
- **API Compatibility**: Modern APIs use NAI format, legacy APIs use string format
- **Best Practice**: Validate precision and symbol before processing assets
