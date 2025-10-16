# Chain Parameters and Witness Governance

This document explains the various parameters and values maintained by the Steem blockchain, how they are governed by witnesses, and how witnesses can modify them.

## Overview

Steem blockchain maintains **two categories of parameters**:

1. **Dynamic Global Properties** - Automatically calculated values reflecting current chain state
2. **Witness-Governed Parameters** - Values set by witness consensus (median voting)

Witnesses propose values for governance parameters, and the **median value** from all active witnesses becomes the active chain parameter.

## Dynamic Global Properties

### Overview

The `dynamic_global_property_object` stores real-time blockchain state ([global_property_object.hpp:22](../libraries/chain/include/steem/chain/global_property_object.hpp#L22)).

These values are **automatically calculated** during block processing and cannot be directly modified by witnesses.

### Block State Properties

**Current Block Information:**
```cpp
uint32_t          head_block_number = 0;    // Current block height
block_id_type     head_block_id;            // Current block hash
time_point_sec    time;                     // Current block timestamp
account_name_type current_witness;          // Witness who produced current block
```

**Block Scheduling:**
```cpp
uint64_t current_aslot = 0;                 // Absolute slot number since genesis
                                            // = total slots (including missed)
```

**Participation Tracking:**
```cpp
fc::uint128_t recent_slots_filled;          // Bitmap of recent block production
                                            // 1 = produced, 0 = missed
uint8_t participation_count = 0;            // Count of filled slots / 128
                                            // Used to calculate participation %
```

**Example - Participation Calculation:**
```cpp
// 128-bit bitmap, each bit = 1 block slot
// participation_rate = (participation_count / 128) * 100%
// If 100 of last 128 slots filled: 78% participation
```

### Supply and Economics

**STEEM Supply:**
```cpp
asset virtual_supply = asset(0, STEEM_SYMBOL);    // Total theoretical supply
asset current_supply = asset(0, STEEM_SYMBOL);    // Actual circulating supply
asset confidential_supply = asset(0, STEEM_SYMBOL); // Blinded balance totals
```

**SBD (Steem Backed Dollars) Supply:**
```cpp
asset current_sbd_supply = asset(0, SBD_SYMBOL);         // Total SBD in existence
asset confidential_sbd_supply = asset(0, SBD_SYMBOL);    // Blinded SBD balance
```

**Vesting (Steem Power):**
```cpp
asset total_vesting_fund_steem = asset(0, STEEM_SYMBOL);  // Total STEEM powered up
asset total_vesting_shares = asset(0, VESTS_SYMBOL);      // Total vesting shares (SP)
```

**Vesting Share Price Calculation:**
```cpp
price get_vesting_share_price() const {
    if (total_vesting_fund_steem == 0 || total_vesting_shares == 0)
        return price(asset(1000, STEEM_SYMBOL), asset(1000000, VESTS_SYMBOL));

    return price(total_vesting_shares, total_vesting_fund_steem);
}
// Example: If 100M STEEM → 100M VESTS, then 1 STEEM = 1 VESTS
```

**Reward Pool:**
```cpp
asset total_reward_fund_steem = asset(0, STEEM_SYMBOL);   // Rewards available for distribution
fc::uint128 total_reward_shares2;                         // Sum of reward^2 for distribution curve
asset pending_rewarded_vesting_shares = asset(0, VESTS_SYMBOL); // Pending curation rewards
asset pending_rewarded_vesting_steem = asset(0, STEEM_SYMBOL);  // STEEM backing pending rewards
```

### Consensus Parameters (From Witness Median)

These values in `dynamic_global_property_object` are **derived from witness consensus**:

```cpp
uint16_t sbd_interest_rate = 0;              // SBD savings interest rate (basis points)
                                             // Set from witness median

uint32_t maximum_block_size = 0;             // Max block size in bytes
                                             // Set from witness median

uint16_t sbd_print_rate = STEEM_100_PERCENT; // SBD print rate (% of rewards paid as SBD)
                                             // Adjusted based on SBD supply
```

### Irreversibility

```cpp
uint32_t last_irreversible_block_num = 0;    // Last block that cannot be undone
                                             // Updated when 75% witness confirmation reached
```

**Usage:**
- Determines which blocks are permanent
- Used for transaction finality guarantees
- Critical for exchanges and cross-chain bridges

### Voting Parameters

```cpp
uint32_t vote_power_reserve_rate = STEEM_INITIAL_VOTE_POWER_RATE;  // Vote regeneration rate
                                                                     // Default: 10 (10 full votes/day)

uint32_t delegation_return_period = STEEM_DELEGATION_RETURN_PERIOD_HF0; // Vesting delegation cooldown
                                                                          // Default: 30 days
```

### Proof of Work (Deprecated)

```cpp
uint64_t total_pow = -1;                     // Total PoW accumulated (legacy)
uint32_t num_pow_witnesses = 0;              // Current PoW witnesses (deprecated)
```

## Witness-Governed Parameters

### Chain Properties Structure

Witnesses propose blockchain parameters via `chain_properties` ([witness_objects.hpp:25](../libraries/chain/include/steem/chain/witness_objects.hpp#L25)):

```cpp
struct chain_properties {
    asset    account_creation_fee;     // Fee to create account (in STEEM)
    uint32_t maximum_block_size;       // Max block size (bytes)
    uint16_t sbd_interest_rate;        // SBD savings interest (basis points)
    uint32_t account_subsidy_limit;    // Account creation rate limit
};
```

### Parameter Details

#### 1. Account Creation Fee

```cpp
asset account_creation_fee = asset(STEEM_MIN_ACCOUNT_CREATION_FEE, STEEM_SYMBOL);
```

**Purpose:**
- Cost to create new account (paid in STEEM)
- Converted to vesting shares (SP) for new account
- Anti-spam measure

**Constraints:**
- Minimum: `STEEM_MIN_ACCOUNT_CREATION_FEE` (protocol-defined)
- Must be paid in STEEM symbol

**Witness Voting:**
- Each witness proposes their preferred fee
- Median of all 21 active witnesses used
- Updates every witness schedule round (~63 seconds)

#### 2. Maximum Block Size

```cpp
uint32_t maximum_block_size = STEEM_MIN_BLOCK_SIZE_LIMIT * 2;  // Default: 128 KB
```

**Purpose:**
- Limits block size to control network bandwidth
- Affects transaction throughput
- Balance between capacity and decentralization

**Constraints:**
- Minimum: `STEEM_MIN_BLOCK_SIZE_LIMIT` (64 KB)
- Maximum: `STEEM_SOFT_MAX_BLOCK_SIZE` (2 MB)

**Validation ([steem_evaluator.cpp:100](../libraries/chain/steem_evaluator.cpp#L100)):**
```cpp
FC_ASSERT(o.props.maximum_block_size <= STEEM_SOFT_MAX_BLOCK_SIZE,
          "Max block size cannot be more than 2MiB");
```

#### 3. SBD Interest Rate

```cpp
uint16_t sbd_interest_rate = STEEM_DEFAULT_SBD_INTEREST_RATE;  // Default: 0 basis points
```

**Purpose:**
- Interest paid on SBD in savings
- Expressed in basis points (1/100 of 1%)
- Incentivizes holding SBD

**Calculation:**
- 100 basis points = 1% annual interest
- 1000 basis points = 10% annual interest

**Example:**
```
SBD in savings: 1000 SBD
Interest rate: 500 basis points (5% APR)
Annual interest: 1000 * 0.05 = 50 SBD
```

#### 4. Account Subsidy Limit

```cpp
uint32_t account_subsidy_limit = 0;
```

**Purpose:**
- Rate limiting for subsidized account creation
- Controls account creation pool regeneration
- Prevents account creation spam

**Mechanism:**
- Higher value = more accounts per time period
- Uses resource credit system
- Balances free accounts with anti-abuse

### Median Calculation Process

The blockchain uses the **median** of all active witness proposals ([witness_schedule.cpp:30](../libraries/chain/witness_schedule.cpp#L30)):

```cpp
void update_median_witness_props(database& db) {
    const witness_schedule_object& wso = db.get_witness_schedule_object();

    // Fetch all active witnesses
    vector<const witness_object*> active;
    for (int i = 0; i < wso.num_scheduled_witnesses; i++) {
        active.push_back(&db.get_witness(wso.current_shuffled_witnesses[i]));
    }

    // Sort by account_creation_fee and get median
    std::sort(active.begin(), active.end(),
        [](auto* a, auto* b) { return a->props.account_creation_fee < b->props.account_creation_fee; });
    asset median_account_creation_fee = active[active.size()/2]->props.account_creation_fee;

    // Repeat for other parameters...
    // Sort by maximum_block_size
    // Sort by sbd_interest_rate
    // Sort by account_subsidy_limit

    // Update witness schedule object with medians
    db.modify(wso, [&](witness_schedule_object& _wso) {
        _wso.median_props.account_creation_fee = median_account_creation_fee;
        _wso.median_props.maximum_block_size = median_maximum_block_size;
        _wso.median_props.sbd_interest_rate = median_sbd_interest_rate;
        _wso.median_props.account_subsidy_limit = median_account_subsidy_limit;
    });

    // Apply to dynamic global properties
    db.modify(db.get_dynamic_global_properties(), [&](auto& dgpo) {
        dgpo.maximum_block_size = median_maximum_block_size;
        dgpo.sbd_interest_rate = median_sbd_interest_rate;
    });
}
```

**Why Median?**
- Prevents single witness from setting extreme values
- Requires majority consensus (11+ of 21 witnesses) to change
- Smooth transitions when parameters change
- Resistant to outliers

## Understanding Median Voting: The 11-Witness Rule

### Mathematical Foundation

For 21 active witnesses, the median calculation works as follows:

```cpp
// Code from witness_schedule.cpp
asset median_account_creation_fee = active[active.size()/2]->props.account_creation_fee;

// Breakdown:
// active.size() = 21
// active.size() / 2 = 10 (integer division)
// Median = active[10] (the 11th element, 0-based index)
```

**Array Position:**
```
Index:  [0][1][2][3][4][5][6][7][8][9][10][11][12][13][14][15][16][17][18][19][20]
                                      ↑
                                 Median position
                                (11th element)
```

### Why 11 Witnesses Are Required

To change the median value, **at least 11 witnesses must agree** on a new value. Here's why:

**Scenario 1: 10 witnesses change (Insufficient)**
```
Initial state: All 21 witnesses propose 3 STEEM
10 witnesses change to 5 STEEM

Before sort: [5,5,5,5,5,5,5,5,5,5, 3,3,3,3,3,3,3,3,3,3,3]
After sort:  [3,3,3,3,3,3,3,3,3,3,3, 5,5,5,5,5,5,5,5,5,5]
Index:        0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
                                    ↑
                              Still 3 STEEM
Result: No change (10 witnesses insufficient)
```

**Scenario 2: 11 witnesses change (Sufficient!)**
```
Initial state: All 21 witnesses propose 3 STEEM
11 witnesses change to 5 STEEM

Before sort: [5,5,5,5,5,5,5,5,5,5,5, 3,3,3,3,3,3,3,3,3,3]
After sort:  [3,3,3,3,3,3,3,3,3,3, 5,5,5,5,5,5,5,5,5,5,5]
Index:        0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
                                    ↑
                              Now 5 STEEM
Result: Changed! (11 witnesses is the tipping point)
```

**Scenario 3: Extreme outlier ignored**
```
20 witnesses propose 3 STEEM
1 witness proposes 1000 STEEM (extreme value)

After sort:  [3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1000]
Index:        0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
                                    ↑
                              Still 3 STEEM
Result: Outlier completely ignored
```

### Gradual Parameter Changes

This mechanism enables **smooth, incremental transitions**:

```
Step 0: Initial state (all at 3 STEEM)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Sorted: [3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3]
                            ↑ Median = 3

Step 1: 5 witnesses change to 4 STEEM
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Sorted: [3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4]
                            ↑ Median = 3 (no change yet)

Step 2: 10 witnesses now at 4 STEEM
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Sorted: [3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4]
                            ↑ Median = 3 (still no change)

Step 3: 11 witnesses at 4 STEEM ⚡ TIPPING POINT
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Sorted: [3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4]
                            ↑ Median = 4 ✓ CHANGED!

Step 4: 15 witnesses at 4 STEEM
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Sorted: [3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4]
                            ↑ Median = 4 (stable)

Step 5: All 21 witnesses at 4 STEEM
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Sorted: [4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4]
                            ↑ Median = 4 (consensus reached)
```

### Multiple Value Scenarios

**Diverse proposals:**
```
Witnesses propose: [1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 7, 8, 9, 10, 15, 100]
                    0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18  19  20
                                                   ↑
                                              Median = 5

The median naturally finds the "middle ground" value
```

### Security and Governance Benefits

**1. Byzantine Fault Tolerance**
```
Total witnesses: 21
Median requires: 11 witnesses (52.4%)
Tolerate up to: 10 malicious witnesses (47.6%)
```

**2. Attack Resistance**
```
Attack: Single witness proposes 0 or MAX_VALUE
Result: ❌ No effect (needs 11 witnesses)

Attack: 10 witnesses collude
Result: ❌ No effect (needs 11 witnesses)

Attack: 11+ witnesses collude
Result: ⚠️  Successful (but requires majority)
       → Stakeholders vote out malicious witnesses
```

**3. Decentralized Governance**
```
✓ No single point of control
✓ Minority cannot force changes
✓ Majority consensus required
✓ Extreme values automatically rejected
✓ Gradual transitions allow community reaction
```

### Real-World Implications

**Parameter Change Timeline:**
1. Witness proposes new value → No immediate effect
2. More witnesses join → Still no effect (if < 11)
3. 11th witness changes → **Parameter changes immediately**
4. Community observes change → Can vote out witnesses if problematic
5. More witnesses adopt → Median stabilizes

**Example: Increasing block size from 64KB to 128KB**
```
Week 1: 3 witnesses propose 128KB  → No change (median stays 64KB)
Week 2: 8 witnesses propose 128KB  → No change (median stays 64KB)
Week 3: 11 witnesses propose 128KB → ✓ Change! (median becomes 128KB)
Week 4: 18 witnesses propose 128KB → Stable at 128KB
```

This gradual process allows:
- Community discussion and feedback
- Testing on smaller changes first
- Reversal if issues discovered (if 11+ witnesses revert)
- Democratic consensus building

## How Witnesses Change Parameters

### Method 1: witness_update_operation (Legacy)

**Operation Structure:**
```cpp
struct witness_update_operation {
    account_name_type owner;              // Witness account
    string            url;                // Witness info URL
    public_key_type   block_signing_key;  // Key to sign blocks
    chain_properties  props;              // Proposed chain parameters
    asset             fee;                // Operation fee
};
```

**Implementation ([steem_evaluator.cpp:83](../libraries/chain/steem_evaluator.cpp#L83)):**
```cpp
void witness_update_evaluator::do_apply(const witness_update_operation& o) {
    _db.get_account(o.owner);  // Verify owner exists

    // Validate fee symbol
    FC_ASSERT(o.props.account_creation_fee.symbol.is_canon());

    // Validate block size
    FC_ASSERT(o.props.maximum_block_size <= STEEM_SOFT_MAX_BLOCK_SIZE,
              "Max block size cannot be more than 2MiB");

    const auto& by_witness_name_idx = _db.get_index<witness_index>().indices().get<by_name>();
    auto wit_itr = by_witness_name_idx.find(o.owner);

    if (wit_itr != by_witness_name_idx.end()) {
        // Update existing witness
        _db.modify(*wit_itr, [&](witness_object& w) {
            from_string(w.url, o.url);
            w.signing_key = o.block_signing_key;
            w.props = o.props;  // Update chain parameters proposal
        });
    } else {
        // Create new witness
        _db.create<witness_object>([&](witness_object& w) {
            w.owner = o.owner;
            from_string(w.url, o.url);
            w.signing_key = o.block_signing_key;
            w.created = _db.head_block_time();
            w.props = o.props;
        });
    }
}
```

### Method 2: witness_set_properties_operation (HF20+)

**Modern flexible operation** ([steem_evaluator.cpp:131](../libraries/chain/steem_evaluator.cpp#L131)):

```cpp
struct witness_set_properties_operation {
    account_name_type owner;
    flat_map<string, vector<char>> props;  // Key-value properties
    extensions_type extensions;
};
```

**Supported Properties:**
- `"key"` - Current signing key (required for authentication)
- `"new_signing_key"` - New signing key to switch to
- `"account_creation_fee"` - Proposed account creation fee
- `"maximum_block_size"` - Proposed max block size
- `"sbd_interest_rate"` - Proposed SBD interest rate
- `"account_subsidy_limit"` - Proposed account subsidy limit
- `"sbd_exchange_rate"` - Price feed (see next section)
- `"url"` - Witness info URL

**Example Usage:**
```json
{
  "owner": "witness-account",
  "props": {
    "key": "STM_CURRENT_SIGNING_KEY",
    "account_creation_fee": "3.000 STEEM",
    "maximum_block_size": 65536,
    "sbd_interest_rate": 1000,
    "url": "https://witness.example.com"
  }
}
```

**Implementation:**
```cpp
void witness_set_properties_evaluator::do_apply(const witness_set_properties_operation& o) {
    // Verify current signing key
    auto itr = o.props.find("key");
    fc::raw::unpack_from_vector(itr->second, signing_key);
    FC_ASSERT(signing_key == witness.signing_key, "'key' does not match witness signing key");

    // Update individual properties
    if (auto itr = o.props.find("account_creation_fee"); itr != o.props.end()) {
        fc::raw::unpack_from_vector(itr->second, props.account_creation_fee);
        account_creation_changed = true;
    }

    if (auto itr = o.props.find("maximum_block_size"); itr != o.props.end()) {
        fc::raw::unpack_from_vector(itr->second, props.maximum_block_size);
        max_block_changed = true;
    }

    // ... similar for other properties

    // Apply changes
    _db.modify(witness, [&](witness_object& w) {
        if (account_creation_changed) w.props.account_creation_fee = props.account_creation_fee;
        if (max_block_changed) w.props.maximum_block_size = props.maximum_block_size;
        if (sbd_interest_changed) w.props.sbd_interest_rate = props.sbd_interest_rate;
        // ...
    });
}
```

## Price Feeds

### Purpose

Witnesses publish **STEEM/SBD price feeds** to establish exchange rates for:
- SBD to STEEM conversions
- Rewards calculation
- Debt ratio monitoring

### Feed Publishing

**Operation: feed_publish_operation** ([steem_evaluator.cpp:1931](../libraries/chain/steem_evaluator.cpp#L1931))

```cpp
struct feed_publish_operation {
    account_name_type publisher;    // Witness account
    price exchange_rate;            // SBD/STEEM price
};
```

**Price Structure:**
```cpp
struct price {
    asset base;   // e.g., "1.000 SBD"
    asset quote;  // e.g., "3.500 STEEM"
};
// Represents: 1 SBD = 3.5 STEEM
```

**Implementation:**
```cpp
void feed_publish_evaluator::do_apply(const feed_publish_operation& o) {
    // Validate price format
    FC_ASSERT(is_asset_type(o.exchange_rate.base, SBD_SYMBOL) &&
              is_asset_type(o.exchange_rate.quote, STEEM_SYMBOL),
              "Price feed must be a SBD/STEEM price");

    const auto& witness = _db.get_witness(o.publisher);
    _db.modify(witness, [&](witness_object& w) {
        w.sbd_exchange_rate = o.exchange_rate;
        w.last_sbd_exchange_update = _db.head_block_time();
    });
}
```

### Median Price Feed

The blockchain calculates a **median price** from all witness feeds:

**Feed History Object:**
- Tracks recent price feeds
- Calculates median over time window
- Used for conversions and debt ratio

**Median Calculation:**
1. Collect all witness price feeds
2. Sort by price ratio
3. Select middle value (median)
4. Update `feed_history_object.current_median_history`

**Usage:**
```cpp
const auto& fhistory = _db.get_feed_history();
FC_ASSERT(!fhistory.current_median_history.is_null(),
          "Cannot convert SBD because there is no price feed");

// Use median price for conversion
price median_price = fhistory.current_median_history;
```

## Witness Schedule Object

### Structure

The `witness_schedule_object` maintains witness scheduling state ([witness_objects.hpp:163](../libraries/chain/include/steem/chain/witness_objects.hpp#L163)):

```cpp
struct witness_schedule_object {
    id_type id;

    fc::uint128 current_virtual_time;                          // Current virtual time for scheduling
    uint32_t next_shuffle_block_num = 1;                       // Block number for next shuffle
    fc::array<account_name_type, STEEM_MAX_WITNESSES>         // Active witness schedule
        current_shuffled_witnesses;
    uint8_t num_scheduled_witnesses = 1;                       // Number of active witnesses

    // Witness category weights (for selection algorithm)
    uint8_t top19_weight = 1;
    uint8_t timeshare_weight = 5;
    uint8_t miner_weight = 1;

    uint32_t witness_pay_normalization_factor = 25;

    chain_properties median_props;                             // Median of witness proposals
    version majority_version;                                  // Consensus version

    uint8_t max_voted_witnesses = STEEM_MAX_VOTED_WITNESSES_HF0;
    uint8_t max_miner_witnesses = STEEM_MAX_MINER_WITNESSES_HF0;
    uint8_t max_runner_witnesses = STEEM_MAX_RUNNER_WITNESSES_HF0;
    uint8_t hardfork_required_witnesses = STEEM_HARDFORK_REQUIRED_WITNESSES;
};
```

### Update Frequency

**Median properties are updated:**
- Every witness schedule round
- After witness set changes
- ~63 seconds (21 blocks @ 3 sec/block)

## Parameter Change Process

### Step-by-Step: How Parameters Change

**1. Witness Proposes New Value**
```bash
# Via CLI wallet
update_witness "witness-account" "https://witness.com" \
    "STM_SIGNING_KEY" \
    '{"account_creation_fee":"5.000 STEEM", "maximum_block_size":131072, "sbd_interest_rate":500}' \
    true
```

**2. Transaction Broadcast and Confirmed**
- Operation included in block
- Witness object updated with new proposal
- No immediate effect on chain parameters

**3. Median Recalculation (Next Round)**
- New witness schedule computed
- Median of all 21 active witness proposals calculated
- Witness schedule object updated with new median

**4. Parameter Activation**
- New median copied to `dynamic_global_property_object`
- Takes effect for subsequent blocks
- Gradual adjustment as more witnesses update

### Example: Changing Account Creation Fee

**Current State:**
```
21 witnesses propose:
Witness 1-10:  3.000 STEEM
Witness 11-20: 3.000 STEEM
Witness 21:    3.000 STEEM
Median: 3.000 STEEM ← Active parameter
```

**Witness 1-11 change proposal to 5.000 STEEM:**
```
Witness 1-11:  5.000 STEEM
Witness 12-20: 3.000 STEEM
Witness 21:    3.000 STEEM
Sorted: [3,3,3,3,3,3,3,3,3,3,3,5,5,5,5,5,5,5,5,5,5]
Median (position 11): 5.000 STEEM ← New active parameter
```

**Result:**
- Requires 11+ witnesses (majority) to change parameter
- Smooth transition (median shifts gradually)
- No single witness can force extreme values

## Monitoring Parameters

### Via API

**Get Dynamic Global Properties:**
```bash
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"database_api.get_dynamic_global_properties",
  "params":{},
  "id":1
}' http://localhost:8080
```

**Get Witness Schedule:**
```bash
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"database_api.get_witness_schedule",
  "params":{},
  "id":1
}' http://localhost:8080
```

**Get Specific Witness:**
```bash
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"database_api.get_witness",
  "params":{"owner":"witness-account"},
  "id":1
}' http://localhost:8080
```

### Via CLI Wallet

```bash
# Get witness info
>>> get_witness "witness-account"

# Get global properties
>>> info

# Get witness schedule
>>> get_witness_schedule
```

## Security Considerations

### Parameter Manipulation

**Protection Mechanisms:**
1. **Median Voting**: Requires 11+ witnesses to change
2. **Hard Limits**: Protocol enforces min/max bounds
3. **Gradual Change**: Median shifts incrementally
4. **Stake Voting**: Witnesses elected by stakeholders

**Attack Scenarios:**
- **Cartel Attack**: 11+ witnesses collude
  - Mitigation: Stakeholder voting removes malicious witnesses
- **Extreme Values**: Single witness proposes 0 or max
  - Mitigation: Median ignores outliers
- **Rapid Changes**: Frequent parameter updates
  - Mitigation: Takes time for median to shift

### Governance Risks

**Centralization:**
- If few entities control many top witnesses
- Can coordinate parameter changes
- Stakeholder voting is key defense

**Coordination:**
- Witnesses may follow "leader" witness
- Reduces diversity of proposals
- Community discussion encourages independence

## Best Practices for Witnesses

### Parameter Proposals

1. **Research Community Needs**
   - Monitor blockchain metrics
   - Engage with community
   - Consider technical implications

2. **Gradual Adjustments**
   - Make small incremental changes
   - Observe effects before further changes
   - Build consensus with other witnesses

3. **Document Reasoning**
   - Publish rationale on witness page
   - Explain technical justification
   - Respond to community feedback

4. **Price Feeds**
   - Update regularly (at least daily)
   - Use reliable price sources
   - Average multiple exchanges

5. **Version Voting**
   - Test new versions on testnet
   - Coordinate hardfork activation
   - Monitor network compatibility

### Monitoring

**Track These Metrics:**
- Block size utilization
- Account creation rate
- SBD peg stability
- Network participation rate
- Transaction fees and volumes

**Adjust Parameters When:**
- Blocks frequently near size limit
- Account creation backlog
- SBD significantly off-peg
- Network congestion or spam

## Related Documentation

- [Block Confirmation Process](block-confirmation.md) - How witnesses produce blocks
- [Witness Plugin](../development/plugin.md) - Running a witness node
- [Genesis Launch](../operations/genesis-launch.md) - Initial parameter setup
- [Node Types Guide](../operations/node-types-guide.md) - Witness node configuration

## Summary

**Key Takeaways:**
- ✅ **Two parameter types**: Dynamic (auto-calculated) and Governed (witness consensus)
- ✅ **Median voting**: 11+ of 21 witnesses required to change parameters
- ✅ **Four main parameters**: Account creation fee, block size, SBD interest, account subsidy
- ✅ **Price feeds**: Witnesses publish STEEM/SBD prices for conversions
- ✅ **Gradual changes**: Median shifts incrementally as witnesses update
- ✅ **Stakeholder oversight**: Voters can remove witnesses who abuse parameters
- ✅ **Two update methods**: Legacy `witness_update` and modern `witness_set_properties` (HF20+)

**Parameter Change Flow:**
```
Witness Proposes → Tx Confirmed → Median Recalculated → Parameter Activated
                                   (Every 63 seconds)
```
