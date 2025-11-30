# Dynamic Global Properties

This document describes the `dynamic_global_property_object`, a critical database object that maintains real-time global state information for the Steem blockchain.

## Table of Contents

- [Overview](#overview)
- [Core Properties](#core-properties)
  - [Block Information](#block-information)
  - [Supply and Token Economics](#supply-and-token-economics)
  - [Vesting and Rewards](#vesting-and-rewards)
  - [Witness Scheduling](#witness-scheduling)
  - [Blockchain Parameters](#blockchain-parameters)
- [Key Methods](#key-methods)
- [Usage in Code](#usage-in-code)
- [API Access](#api-access)
- [Implementation Details](#implementation-details)

## Overview

The `dynamic_global_property_object` is a singleton database object that stores the current state of the blockchain. Unlike static configuration parameters, these properties are continuously updated during block processing and reflect real-time blockchain conditions.

**Location**: `src/core/chain/include/steem/chain/global_property_object.hpp`

This object is fundamental to:
- Block validation and production
- Token supply tracking
- Reward distribution
- Witness scheduling
- Vote power calculations
- Economic parameter adjustments

## Core Properties

### Block Information

Properties related to the current state of the blockchain:

| Property | Type | Description |
|----------|------|-------------|
| `head_block_number` | `uint32_t` | Current block height (starts at 0 for genesis) |
| `head_block_id` | `block_id_type` | Hash/ID of the most recent block |
| `time` | `time_point_sec` | Timestamp of the current head block |
| `current_witness` | `account_name_type` | Account name of the witness who produced the current block |
| `last_irreversible_block_num` | `uint32_t` | Block number that is irreversible (cannot be forked) |

**Example Access**:
```cpp
const auto& dgpo = db.get_dynamic_global_properties();
uint32_t current_block = dgpo.head_block_number;
time_point_sec block_time = dgpo.time;
```

### Supply and Token Economics

Properties tracking the total supply of STEEM and SBD tokens:

| Property | Type | Description |
|----------|------|-------------|
| `virtual_supply` | `asset` | Virtual STEEM supply used for rate limiting and price feeds |
| `current_supply` | `asset` | Total circulating STEEM tokens |
| `confidential_supply` | `asset` | STEEM held in confidential/blinded balances |
| `current_sbd_supply` | `asset` | Total circulating SBD tokens |
| `confidential_sbd_supply` | `asset` | SBD held in confidential/blinded balances |
| `sbd_interest_rate` | `uint16_t` | Annual interest rate for SBD deposits (basis points) |
| `sbd_print_rate` | `uint16_t` | Rate at which new SBD can be printed (default: `STEEM_100_PERCENT`) |

**Notes**:
- Virtual supply includes liquid STEEM, vested STEEM, and SBD debt
- Supply values are updated on every block based on rewards, conversions, and other operations
- Interest rates are set by witnesses and adjusted through consensus

### Vesting and Rewards

Properties related to STEEM Power (vested STEEM) and the reward pool:

| Property | Type | Description |
|----------|------|-------------|
| `total_vesting_fund_steem` | `asset` | Total STEEM currently vested (STEEM Power backing) |
| `total_vesting_shares` | `asset` | Total VESTS (Vesting Shares) in existence |
| `total_reward_fund_steem` | `asset` | Total STEEM available in the reward pool |
| `total_reward_shares2` | `fc::uint128` | Sum of curve-adjusted reward shares (running total of `evaluate_reward_curve(rshares)`) for proportional reward distribution. Note: "2" is historical - originally for quadratic curve |
| `pending_rewarded_vesting_shares` | `asset` | VESTS that will be created when pending rewards are claimed |
| `pending_rewarded_vesting_steem` | `asset` | STEEM backing for pending VESTS rewards |

**Key Calculations**:

The object provides two important price calculations:

```cpp
// Current VESTS to STEEM conversion rate
price get_vesting_share_price() const
{
    if (total_vesting_fund_steem.amount == 0 || total_vesting_shares.amount == 0)
        return price(asset(1000, STEEM_SYMBOL), asset(1000000, VESTS_SYMBOL));

    return price(total_vesting_shares, total_vesting_fund_steem);
}

// VESTS to STEEM rate including pending rewards
price get_reward_vesting_share_price() const
{
    return price(total_vesting_shares + pending_rewarded_vesting_shares,
                 total_vesting_fund_steem + pending_rewarded_vesting_steem);
}
```

**Usage Example**:
```cpp
const auto& dgpo = db.get_dynamic_global_properties();

// Convert VESTS to STEEM
asset vests = asset(1000000, VESTS_SYMBOL);
asset steem = vests * dgpo.get_vesting_share_price();

// Calculate total vesting fund including pending rewards
asset total_fund = dgpo.total_vesting_fund_steem + dgpo.pending_rewarded_vesting_steem;
```

### Witness Scheduling

Properties used for witness scheduling and participation tracking:

| Property | Type | Description |
|----------|------|-------------|
| `current_aslot` | `uint64_t` | Absolute slot number (total slots since genesis) |
| `recent_slots_filled` | `fc::uint128_t` | Bitmap of recent slot fills (1 = filled, 0 = missed) |
| `participation_count` | `uint8_t` | Number of filled slots in recent history (divide by 128 for percentage) |

**Witness Participation Calculation**:
```cpp
// Calculate witness participation rate
const auto& dgpo = db.get_dynamic_global_properties();
double participation_rate = dgpo.participation_count / 128.0;

// Check if a specific slot was filled
bool slot_filled = (dgpo.recent_slots_filled & (fc::uint128_t(1) << slot_offset)) != 0;
```

**Notes**:
- Absolute slot (`current_aslot`) = total slots since genesis = missed slots + `head_block_number`
- `recent_slots_filled` tracks the last 128 slots as a bitmap
- Participation is critical for network health monitoring

### Blockchain Parameters

Dynamic parameters that govern blockchain behavior:

| Property | Type | Description |
|----------|------|-------------|
| `maximum_block_size` | `uint32_t` | Maximum allowed block size (median of witness proposals) |
| `vote_power_reserve_rate` | `uint32_t` | Vote regeneration rate per day |
| `delegation_return_period` | `uint32_t` | Time before delegated VESTS are returned after undelegation |
| `smt_creation_fee` | `asset` | Fee to create a Smart Media Token (SMT) (if enabled) |

**Notes**:
- `maximum_block_size` is set by witness consensus (median of all active witness proposals)
- Protocol enforces a minimum block size to prevent network stalling
- `vote_power_reserve_rate` determines vote regeneration speed (default: `STEEM_INITIAL_VOTE_POWER_RATE`)
- `delegation_return_period` prevents rapid delegation cycling abuse

## Key Methods

### get_vesting_share_price()

Calculates the current exchange rate between VESTS (Vesting Shares) and STEEM.

**Signature**:
```cpp
price get_vesting_share_price() const
```

**Returns**: `price` object representing VESTS to STEEM conversion rate

**Usage**:
```cpp
const auto& dgpo = db.get_dynamic_global_properties();
price vests_to_steem = dgpo.get_vesting_share_price();

// Convert 1,000,000 VESTS to STEEM
asset vests(1000000, VESTS_SYMBOL);
asset steem = vests * vests_to_steem;
```

**Default Rate**: If vesting fund is empty, returns 1000 STEEM = 1,000,000 VESTS (1:1000 ratio)

### get_reward_vesting_share_price()

Calculates the VESTS to STEEM rate including pending rewards that haven't been claimed yet.

**Signature**:
```cpp
price get_reward_vesting_share_price() const
```

**Returns**: `price` object including pending reward VESTS and STEEM

**Usage**:
```cpp
const auto& dgpo = db.get_dynamic_global_properties();
price reward_rate = dgpo.get_reward_vesting_share_price();

// Use for reward calculations
asset pending_vests = calculate_pending_vests_reward();
asset pending_steem_value = pending_vests * reward_rate;
```

## Usage in Code

### Accessing Dynamic Global Properties

The database provides a convenience method to access the singleton object:

```cpp
const dynamic_global_property_object& db.get_dynamic_global_properties() const
```

**Common Usage Pattern**:
```cpp
void some_evaluator::do_apply(const some_operation& op)
{
    const auto& dgpo = _db.get_dynamic_global_properties();

    // Access current block time
    time_point_sec now = dgpo.time;

    // Check block number
    uint32_t current_block = dgpo.head_block_number;

    // Get current witness
    account_name_type current_witness = dgpo.current_witness;

    // Calculate vesting conversion
    asset vests = op.vesting_shares;
    asset steem = vests * dgpo.get_vesting_share_price();
}
```

### Modifying Properties

Dynamic global properties are modified by the database during block processing. Direct modifications should only be done within the chain library:

```cpp
db.modify(dgpo, [&](dynamic_global_property_object& gpo) {
    gpo.total_vesting_fund_steem += new_vesting;
    gpo.total_vesting_shares += new_shares;
});
```

**Important**: Only the chain library should modify these properties. Plugins and evaluators should treat them as read-only.

## API Access

### database_api

The dynamic global properties are accessible via the `database_api` plugin:

**Method**: `get_dynamic_global_properties`

**Example JSON-RPC Request**:
```json
{
  "jsonrpc": "2.0",
  "method": "database_api.get_dynamic_global_properties",
  "params": {},
  "id": 1
}
```

**Example Response**:
```json
{
  "jsonrpc": "2.0",
  "result": {
    "id": 0,
    "head_block_number": 50000000,
    "head_block_id": "02faf080...",
    "time": "2023-01-15T12:00:00",
    "current_witness": "witness.account",
    "virtual_supply": "400000000.000 STEEM",
    "current_supply": "350000000.000 STEEM",
    "confidential_supply": "0.000 STEEM",
    "current_sbd_supply": "15000000.000 SBD",
    "confidential_sbd_supply": "0.000 SBD",
    "total_vesting_fund_steem": "200000000.000 STEEM",
    "total_vesting_shares": "400000000000.000000 VESTS",
    "total_reward_fund_steem": "5000000.000 STEEM",
    "total_reward_shares2": "10000000000000000000",
    "pending_rewarded_vesting_shares": "100000.000000 VESTS",
    "pending_rewarded_vesting_steem": "50.000 STEEM",
    "sbd_interest_rate": 0,
    "sbd_print_rate": 10000,
    "maximum_block_size": 65536,
    "current_aslot": 52000000,
    "recent_slots_filled": "340282366920938463463374607431768211455",
    "participation_count": 128,
    "last_irreversible_block_num": 49999900,
    "vote_power_reserve_rate": 10,
    "delegation_return_period": 432000
  },
  "id": 1
}
```

## Implementation Details

### Database Object Type

```cpp
class dynamic_global_property_object :
    public object<dynamic_global_property_object_type, dynamic_global_property_object>
{
    // ... properties ...
};
```

**Object ID**: Always `id = 0` (singleton object)

**Storage**: Stored in chainbase memory-mapped database

### Multi-Index Container

```cpp
typedef multi_index_container<
    dynamic_global_property_object,
    indexed_by<
        ordered_unique<tag<by_id>,
            member<dynamic_global_property_object,
                   dynamic_global_property_object::id_type,
                   &dynamic_global_property_object::id>>
    >,
    allocator<dynamic_global_property_object>
> dynamic_global_property_index;
```

**Index**: Single index by ID (since there's only one object)

### Serialization

The object uses `FC_REFLECT` for automatic serialization to binary and JSON formats:

```cpp
FC_REFLECT(steem::chain::dynamic_global_property_object,
    (id)
    (head_block_number)
    (head_block_id)
    // ... all fields ...
)
```

This enables:
- Binary packing for efficient storage
- JSON conversion for API responses
- Type introspection for debugging

### Update Frequency

Properties are updated at different frequencies:

| Property | Update Frequency |
|----------|------------------|
| Block info (`head_block_number`, `time`, etc.) | Every block (3 seconds) |
| Supply values | On operations affecting supply (transfers, rewards, etc.) |
| Vesting values | On power up/down operations, reward claims |
| Witness scheduling | Every block (slot tracking) |
| Parameters (`sbd_interest_rate`, etc.) | When witnesses vote to change them |

### Thread Safety

- **Read Access**: Thread-safe via database read locks
- **Write Access**: Only during block application (single-threaded)
- **Important**: Read locks auto-expire after 1 second - design API calls to complete quickly

### Hardfork Considerations

Some properties were added or modified in specific hardforks:

- **Hardfork 0**: Initial `delegation_return_period` value
- **Later Hardforks**: Additional SMT properties (if enabled)

Always check hardfork version when accessing newly added properties:

```cpp
if (db.has_hardfork(STEEM_HARDFORK_0_XX)) {
    // Safe to access properties added in HF 0_XX
}
```

## Common Patterns

### Calculate Current STEEM Power Value

```cpp
const auto& dgpo = db.get_dynamic_global_properties();
const auto& account = db.get_account(account_name);

asset vesting_shares = account.vesting_shares;
asset steem_power = vesting_shares * dgpo.get_vesting_share_price();
```

### Check Network Participation

```cpp
const auto& dgpo = db.get_dynamic_global_properties();
double participation = dgpo.participation_count / 128.0;

if (participation < 0.95) {
    wlog("Low network participation: ${p}%", ("p", participation * 100));
}
```

### Validate Block Timing

```cpp
const auto& dgpo = db.get_dynamic_global_properties();
time_point_sec now = dgpo.time;

FC_ASSERT(op.timestamp >= now, "Operation timestamp is in the past");
FC_ASSERT(op.timestamp <= now + fc::seconds(3600), "Operation timestamp is too far in future");
```

### Calculate Inflation Rate

```cpp
const auto& dgpo = db.get_dynamic_global_properties();

// Virtual supply includes all forms of STEEM value
asset virtual_supply = dgpo.virtual_supply;

// Current supply is liquid STEEM
asset current_supply = dgpo.current_supply;

// SBD debt affects virtual supply
asset sbd_debt = dgpo.current_sbd_supply;
```

## Best Practices

1. **Read-Only Access**: Always treat `dynamic_global_property_object` as read-only in evaluators and plugins
2. **Cache Locally**: If accessing multiple properties, store reference to avoid repeated lookups:
   ```cpp
   const auto& dgpo = db.get_dynamic_global_properties();
   // Use dgpo multiple times
   ```
3. **Don't Hold Long**: Don't store references across block boundaries - properties change frequently
4. **Use Helper Methods**: Prefer `get_vesting_share_price()` over manual calculation
5. **Check Hardforks**: Verify hardfork version before accessing properties added in later versions
6. **API Performance**: When exposing via API, consider caching since properties change only once per block

## Related Documentation

- [Chain Parameters](chain-parameters.md) - Static blockchain configuration
- [System Architecture](system-architecture.md) - Overall system design
- [API Notes](api-notes.md) - API usage guidelines
- [Asset Format](asset-format.md) - Asset representation and conversion

## References

- **Source Code**: `src/core/chain/include/steem/chain/global_property_object.hpp`
- **Database Implementation**: `src/core/chain/database.cpp`
- **API Implementation**: `src/plugins/apis/database_api/database_api.cpp`
- **Evaluator Usage**: `src/core/chain/steem_evaluator.cpp`
