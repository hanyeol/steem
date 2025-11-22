# SBD (Steem Backed Dollars) Technical Reference

## Table of Contents

1. [Overview](#overview)
2. [Purpose and Design](#purpose-and-design)
3. [SBD Issuance Mechanism](#sbd-issuance-mechanism)
4. [Debt Ratio and Print Rate](#debt-ratio-and-print-rate)
5. [SBD Conversions](#sbd-conversions)
6. [SBD Interest on Savings](#sbd-interest-on-savings)
7. [Price Feed Mechanism](#price-feed-mechanism)
8. [Virtual Supply Calculation](#virtual-supply-calculation)
9. [Implementation Details](#implementation-details)
10. [Economic Implications](#economic-implications)
11. [Best Practices](#best-practices)

---

## Overview

**SBD (Steem Backed Dollars)** is a debt instrument on the Steem blockchain designed to maintain a soft peg to $1 USD. SBD represents a claim on approximately $1 worth of STEEM and can be converted to STEEM at the current market price via a conversion mechanism.

### Key Characteristics

- **Symbol**: `SBD`
- **Precision**: 3 decimal places (0.001 SBD minimum)
- **Target Value**: ~$1 USD
- **Backing**: Convertible to STEEM at median price feed
- **Debt Instrument**: Represents blockchain debt backed by STEEM supply
- **Interest-Bearing**: Can earn interest when held in savings (if enabled by witnesses)

### Design Goals

1. **Price Stability**: Provide a stable medium of exchange pegged to USD
2. **Low Volatility**: Reduce exposure to STEEM price fluctuations
3. **Convertibility**: Maintain conversion mechanism to STEEM
4. **Sustainability**: Limit SBD supply to prevent excessive blockchain debt
5. **Liquidity**: Allow trading on internal market and external exchanges

---

## Purpose and Design

### Why SBD Exists

SBD was created to solve several problems in a cryptocurrency ecosystem:

1. **Price Volatility**: STEEM price fluctuates, making it difficult to price content and services
2. **Merchant Adoption**: Merchants prefer stable currencies for pricing
3. **Predictable Rewards**: Content creators want to know the USD value of rewards
4. **Store of Value**: Users need a stable asset to hold purchasing power

### How SBD Maintains the Peg

SBD uses multiple mechanisms to maintain its $1 peg:

#### 1. Conversion Mechanism

Users can convert SBD to STEEM at the **median price feed** published by witnesses:

```
Conversion: 1 SBD → $1 worth of STEEM (at median price)
```

**Example**:
- Median price feed: 1 STEEM = $0.50
- User converts: 1 SBD → 2 STEEM (worth $1)

This creates a **price floor** at $1:
- If SBD trades below $1, arbitrageurs buy SBD and convert to STEEM for profit
- This buying pressure pushes SBD price back toward $1

#### 2. Reward Reduction at High Debt

When SBD supply becomes too large relative to STEEM market cap, the blockchain reduces or stops printing new SBD (see [Debt Ratio](#debt-ratio-and-print-rate)).

#### 3. Internal Market

The blockchain provides a decentralized exchange for STEEM/SBD trading, allowing price discovery and arbitrage.

### SBD vs Stablecoins

SBD differs from modern stablecoins:

| Feature | SBD | USDT/USDC | DAI |
|---------|-----|-----------|-----|
| **Backing** | STEEM (volatile crypto) | USD reserves | Crypto collateral |
| **Peg Mechanism** | Conversion + market | Redemption | Liquidation |
| **Supply Limit** | Yes (debt ratio) | No | No |
| **Interest** | Optional (witness-set) | No | DSR (savings rate) |
| **Decentralization** | Full | Centralized | Decentralized |

**Important**: SBD is a **soft peg**, not a hard peg. It can trade above or below $1.

---

## SBD Issuance Mechanism

### When is SBD Created?

SBD is **only created** during content reward distribution. It is NOT created through:
- Inflation
- Witness rewards
- Vesting fund interest
- User operations

### Reward Distribution Split

When a post/comment receives rewards after the 7-day payout period, the total reward is distributed as:

**For Authors** (75% of total reward):
- **50% as STEEM Power** (vesting shares)
- **50% as liquid reward**:
  - If `sbd_print_rate = 100%`: Paid entirely in SBD
  - If `sbd_print_rate < 100%`: Split between SBD and STEEM

**For Curators** (25% of total reward):
- **100% as STEEM Power** (no SBD to curators)

### SBD Print Rate Calculation

The amount of SBD printed depends on `sbd_print_rate`:

**Code Implementation** ([database.cpp:1075](libraries/chain/database.cpp#L1075)):

```cpp
const auto& median_price = get_feed_history().current_median_history;
const auto& gpo = get_dynamic_global_properties();

if (!median_price.is_null())
{
   auto to_sbd = (gpo.sbd_print_rate * steem.amount) / STEEM_100_PERCENT;
   auto to_steem = steem.amount - to_sbd;

   auto sbd = asset(to_sbd, STEEM_SYMBOL) * median_price;

   // Pay to_sbd amount as SBD
   // Pay to_steem amount as STEEM
}
```

**Formula**:
```
SBD Amount = (Total Author Liquid Reward × sbd_print_rate / 100%) × Median Price
STEEM Amount = Total Author Liquid Reward - (Total Author Liquid Reward × sbd_print_rate / 100%)
```

**Example 1: Full SBD Printing** (`sbd_print_rate = 100%`)
```
Total author reward: 100 STEEM
- 50 STEEM → STEEM Power
- 50 STEEM → Liquid:
  - to_sbd = 50 × 100% = 50 STEEM worth
  - to_steem = 50 - 50 = 0 STEEM
  - Median price: 1 STEEM = $0.50
  - SBD paid: 50 STEEM × $0.50 = $25 = 25 SBD
```

**Example 2: 50% SBD Printing** (`sbd_print_rate = 50%`)
```
Total author reward: 100 STEEM
- 50 STEEM → STEEM Power
- 50 STEEM → Liquid:
  - to_sbd = 50 × 50% = 25 STEEM worth
  - to_steem = 50 - 25 = 25 STEEM
  - Median price: 1 STEEM = $0.50
  - SBD paid: 25 STEEM × $0.50 = $12.50 = 12.5 SBD
  - STEEM paid: 25 STEEM
```

**Example 3: Zero SBD Printing** (`sbd_print_rate = 0%`)
```
Total author reward: 100 STEEM
- 50 STEEM → STEEM Power
- 50 STEEM → Liquid:
  - All paid as STEEM (no SBD created)
```

---

## Debt Ratio and Print Rate

### The Debt Ratio Problem

SBD is a **debt instrument** - each SBD represents a claim on ~$1 worth of STEEM. If too much SBD exists relative to STEEM market cap, the blockchain cannot guarantee conversions.

### Debt Ratio Calculation

```
Debt Ratio = (Current SBD Supply × Median STEEM Price) / (Virtual STEEM Supply × Median STEEM Price)
           = Current SBD Supply / Virtual STEEM Supply × 100%
```

**Simplified**:
```
Debt Ratio = SBD Market Cap / STEEM Market Cap
```

**Example**:
```
Current SBD supply: 15,000,000 SBD
Virtual STEEM supply: 400,000,000 STEEM
Median price: 1 STEEM = $0.50

SBD market cap: 15,000,000 SBD × $1 = $15,000,000
STEEM market cap: 400,000,000 STEEM × $0.50 = $200,000,000

Debt Ratio = $15,000,000 / $200,000,000 = 7.5%
```

### Print Rate Adjustment

The blockchain automatically adjusts `sbd_print_rate` based on debt ratio:

**Constants** ([config.hpp:189-190](libraries/protocol/include/steem/protocol/config.hpp#L189-L190)):
```cpp
#define STEEM_SBD_STOP_PERCENT    (5 * STEEM_1_PERCENT)  // 5%
#define STEEM_SBD_START_PERCENT   (2 * STEEM_1_PERCENT) // 2%
```

**Print Rate Rules**:

| Debt Ratio | sbd_print_rate | Effect |
|------------|----------------|--------|
| **< 2%** | **100%** | Full SBD printing (50% of author liquid rewards) |
| **2% - 5%** | **Linear reduction** | Gradually reduce SBD printing |
| **≥ 5%** | **0%** | Stop SBD printing (all STEEM rewards) |

**Linear Reduction Formula** (2% ≤ debt ratio < 5%):
```
sbd_print_rate = ((5% - debt_ratio) / (5% - 2%)) × 100%
               = ((5% - debt_ratio) / 3%) × 100%
```

**Examples**:

```
Debt Ratio 1%:  sbd_print_rate = 100% (full SBD)
Debt Ratio 2%:  sbd_print_rate = 100% (threshold start)
Debt Ratio 3%:  sbd_print_rate = 66.7% ((5-3)/3 = 67%)
Debt Ratio 4%:  sbd_print_rate = 33.3% ((5-4)/3 = 33%)
Debt Ratio 5%:  sbd_print_rate = 0%   (no SBD)
Debt Ratio 10%: sbd_print_rate = 0%   (no SBD)
```

### Why These Thresholds?

**Conservative Debt Management**:
- **2% threshold**: Early warning to reduce SBD creation
- **5% limit**: Prevents excessive debt accumulation
- **Linear reduction**: Smooth transition avoids sudden changes

**Historical Context**:
- At high debt ratios, conversion guarantees become unreliable
- If debt ratio exceeds 100%, blockchain cannot fulfill all conversions
- Conservative limits ensure long-term sustainability

### Print Rate Adjustment in Code

The print rate is updated during block processing based on current debt ratio. The calculation occurs in the blockchain's maintenance operations.

**Result**:
- `dynamic_global_property_object.sbd_print_rate` is updated each block
- This value is used in reward distribution ([database.cpp:1075](libraries/chain/database.cpp#L1075))

---

## SBD Conversions

### Two Types of Conversion

#### 1. SBD to STEEM Conversion (convert_operation)

Users can convert SBD to STEEM at the **median price feed**.

**Operation** ([steem_operations.hpp:590-595](libraries/protocol/include/steem/protocol/steem_operations.hpp#L590-L595)):
```cpp
struct convert_operation : public base_operation {
   account_name_type owner;
   uint32_t          requestid = 0;
   asset             amount;     // SBD to convert
};
```

**Process**:
1. User submits `convert_operation` with SBD amount
2. SBD is immediately removed from user balance
3. Conversion completes after **3.5 days** (STEEM_CONVERSION_DELAY)
4. User receives STEEM based on **median price at conversion time**

**Example**:
```
User converts: 100 SBD
Median price at conversion: 1 STEEM = $0.40

STEEM received = 100 SBD / $0.40 per STEEM = 250 STEEM
```

**Important Notes**:
- **3.5 day delay**: Prevents gaming price feeds
- **Median price**: Uses 3.5 day median of witness feeds
- **One-way**: Cannot convert STEEM to SBD this way
- **Price risk**: STEEM price may change during conversion period

#### 2. STEEM to SBD Conversion (Indirect - Internal Market)

There is **no direct operation** to convert STEEM to SBD. Users must:
- Use the internal market to trade STEEM for SBD
- Or use external exchanges

### Conversion Delay

**Constant** ([config.hpp:146](libraries/protocol/include/steem/protocol/config.hpp#L146)):
```cpp
#define STEEM_CONVERSION_DELAY  (fc::days(3.5))
```

**Why 3.5 Days?**
- Prevents rapid arbitrage against price feed manipulation
- Allows median price to stabilize
- Protects blockchain from price oracle attacks

### Conversion Economics

**Price Floor**:
The conversion mechanism creates a **price floor** at $1 for SBD:

```
If SBD < $1 on market:
→ Buy SBD for < $1
→ Convert to STEEM worth $1
→ Sell STEEM for profit
→ This buying pressure pushes SBD back toward $1
```

**No Price Ceiling**:
SBD can trade **above** $1 because:
- No mechanism to create SBD from STEEM
- Limited supply (debt ratio limits)
- Market demand can exceed supply

**Historical Examples**:
- 2017-2018: SBD traded as high as $13 during crypto bull market
- Conversion mechanism only provides downside protection, not upside

---

## SBD Interest on Savings

### Savings Account Feature

Users can deposit SBD (or STEEM) into a **savings account** which:
- Requires 3-day withdrawal period
- Can earn interest (if enabled by witnesses)
- Provides additional security against account hacks

### Interest Rate

**Dynamic Property**:
```cpp
uint16_t sbd_interest_rate = 0;  // Basis points (1/100 of 1%)
```

**Set by Witnesses**:
- Each witness proposes an interest rate
- **Median** of all 21 active witness proposals becomes active rate
- Expressed in **basis points**: 100 bp = 1% APR

**Examples**:
```
sbd_interest_rate = 0     → 0% APR (no interest)
sbd_interest_rate = 100   → 1% APR
sbd_interest_rate = 1000  → 10% APR
sbd_interest_rate = 1500  → 15% APR
```

### Interest Calculation

Interest is calculated continuously and compounds:

**Formula**:
```
Interest per second = (Balance × Interest Rate) / (100 × 100 × Seconds per Year)
                    = (Balance × Interest Rate) / (10000 × 31536000)
```

**Example** (10% APR = 1000 bp):
```
SBD in savings: 1000 SBD
Interest rate: 1000 bp (10% APR)
Time: 1 year

Interest earned = 1000 SBD × 10% = 100 SBD
```

### Current Status

**Important**: As of recent history, SBD interest is typically **set to 0%** by witness consensus due to:
- Debt ratio concerns
- Focus on limiting SBD supply
- Market dynamics favoring no interest

---

## Price Feed Mechanism

### Purpose

Witnesses publish **price feeds** to establish the STEEM/SBD exchange rate used for:
- SBD conversions
- Reward calculations
- Debt ratio monitoring
- Internal market reference

### Price Feed Structure

**Operation** ([steem_operations.hpp:668-672](libraries/protocol/include/steem/protocol/steem_operations.hpp#L668-L672)):
```cpp
struct feed_publish_operation : public base_operation {
   account_name_type publisher;    // Witness account
   price exchange_rate;            // STEEM/SBD price
};
```

**Price Object**:
```cpp
struct price {
   asset base;   // e.g., "1.000 SBD"
   asset quote;  // e.g., "2.500 STEEM"
};
// Interpretation: 1 SBD = 2.5 STEEM
// Or: 1 STEEM = $0.40
```

### Median Price Feed

The blockchain calculates a **median** of all witness price feeds:

**Process**:
1. Each witness publishes their price feed (ideally every hour)
2. Blockchain maintains history of recent feeds
3. **Median** price is calculated from all active witness feeds
4. Median is used for conversions and debt ratio

**Why Median?**
- Prevents single witness from manipulating price
- Resistant to outliers
- Requires 11+ witnesses to shift median (majority consensus)
- More stable than average

### Feed History

**Object**: `feed_history_object`
- Tracks recent price feeds
- Maintains moving median
- Updates each time a witness publishes

**Access**:
```cpp
const auto& feed_history = db.get_feed_history();
price median_price = feed_history.current_median_history;
```

### Price Feed Best Practices

**For Witnesses**:
1. **Update Frequency**: Publish feeds at least every hour
2. **Price Sources**: Use multiple reliable exchanges
3. **Averaging**: Average prices over short time window (5-15 min)
4. **Outlier Detection**: Discard obvious errors
5. **Bias**: 1% bias toward $1 helps maintain peg

**Price Feed Formula**:
```
median_price = median(
   Binance_STEEM_USD,
   Upbit_STEEM_USD,
   Bittrex_STEEM_USD,
   ...
)
```

---

## Virtual Supply Calculation

### What is Virtual Supply?

**Virtual Supply** is a calculated value representing the **total value** of all STEEM-based assets on the blockchain, including:
- Liquid STEEM
- Vested STEEM (STEEM Power)
- **SBD debt** (converted to STEEM equivalent)

### Calculation

**Formula**:
```cpp
virtual_supply = current_supply + (current_sbd_supply × median_price)
```

**Components**:
- `current_supply`: Actual circulating STEEM tokens
- `current_sbd_supply`: Total SBD in existence
- `median_price`: STEEM per SBD (inverse of USD price)

**Example**:
```
current_supply: 350,000,000 STEEM
current_sbd_supply: 15,000,000 SBD
median_price: 1 SBD = 2 STEEM (i.e., 1 STEEM = $0.50)

virtual_supply = 350,000,000 + (15,000,000 × 2)
               = 350,000,000 + 30,000,000
               = 380,000,000 STEEM
```

### Why Virtual Supply Matters

**1. Debt Ratio Calculation**:
```
debt_ratio = current_sbd_supply / virtual_supply
```

**2. Inflation Calculation**:
Inflation rewards are based on virtual supply ([database.cpp:1829](libraries/chain/database.cpp#L1829)):
```cpp
auto new_steem = (props.virtual_supply.amount × current_inflation_rate)
                 / (STEEM_100_PERCENT × STEEM_BLOCKS_PER_YEAR);
```

**3. Economic Metrics**:
- Represents "total value" in STEEM terms
- Used for APR calculations
- Market cap equivalent

### Virtual Supply Updates

Virtual supply is updated when:
- New STEEM is created (inflation)
- SBD supply changes (creation/destruction)
- Conversions occur (SBD ↔ STEEM)

**Code** ([database.cpp:2002-2003](libraries/chain/database.cpp#L2002-L2003)):
```cpp
modify(props, [&](dynamic_global_property_object& p) {
   p.current_supply += net_steem;
   p.current_sbd_supply -= net_sbd;
   p.virtual_supply += net_steem;
   p.virtual_supply -= net_sbd × get_feed_history().current_median_history;
});
```

---

## Implementation Details

### Database Objects

#### Dynamic Global Properties

SBD-related properties in `dynamic_global_property_object`:

```cpp
asset current_sbd_supply = asset(0, SBD_SYMBOL);         // Total SBD
asset confidential_sbd_supply = asset(0, SBD_SYMBOL);    // Blinded SBD
uint16_t sbd_print_rate = STEEM_100_PERCENT;             // Print rate %
uint16_t sbd_interest_rate = 0;                          // Interest rate (bp)
asset virtual_supply = asset(0, STEEM_SYMBOL);           // Virtual supply
```

#### Account Balance Fields

Each account has SBD balance fields:

```cpp
struct account_object {
   asset sbd_balance = asset(0, SBD_SYMBOL);              // Liquid SBD
   asset savings_sbd_balance = asset(0, SBD_SYMBOL);      // SBD in savings
   asset reward_sbd_balance = asset(0, SBD_SYMBOL);       // Pending SBD rewards
   // ...
};
```

### Operations

#### convert_operation

**File**: [libraries/protocol/include/steem/protocol/steem_operations.hpp:590](libraries/protocol/include/steem/protocol/steem_operations.hpp#L590)

```cpp
struct convert_operation : public base_operation {
   account_name_type owner;
   uint32_t          requestid = 0;
   asset             amount;

   void validate() const;
};
```

**Validation**:
- `amount` must be SBD
- `amount` must be > 0
- `requestid` must be unique per account

**Evaluator**: [libraries/chain/steem_evaluator.cpp](libraries/chain/steem_evaluator.cpp)

#### feed_publish_operation

**File**: [libraries/protocol/include/steem/protocol/steem_operations.hpp:668](libraries/protocol/include/steem/protocol/steem_operations.hpp#L668)

```cpp
struct feed_publish_operation : public base_operation {
   account_name_type publisher;
   price exchange_rate;

   void validate() const;
};
```

**Validation**:
- `publisher` must be an active witness
- `exchange_rate` must be SBD/STEEM format
- Price must be positive

### Key Functions

#### Reward Distribution with SBD

**File**: [libraries/chain/database.cpp:1070-1090](libraries/chain/database.cpp#L1070-L1090)

```cpp
void database::cashout_comment_helper(const comment_object& comment) {
   const auto& median_price = get_feed_history().current_median_history;
   const auto& gpo = get_dynamic_global_properties();

   if (!median_price.is_null()) {
      auto to_sbd = (gpo.sbd_print_rate × steem.amount) / STEEM_100_PERCENT;
      auto to_steem = steem.amount - to_sbd;

      auto sbd = asset(to_sbd, STEEM_SYMBOL) × median_price;

      if (to_reward_balance) {
         adjust_reward_balance(to_account, sbd);
         adjust_reward_balance(to_account, asset(to_steem, STEEM_SYMBOL));
      } else {
         adjust_balance(to_account, sbd);
         adjust_balance(to_account, asset(to_steem, STEEM_SYMBOL));
      }
   }
}
```

#### Conversion Processing

**File**: [libraries/chain/database.cpp:1984-2005](libraries/chain/database.cpp#L1984-L2005)

```cpp
void database::process_conversions() {
   const auto& conversion_idx = get_index<convert_request_index>()
      .indices().get<by_conversion_date>();

   while (!conversion_idx.empty() &&
          conversion_idx.begin()->conversion_date <= head_block_time()) {
      const auto& conversion = *conversion_idx.begin();
      const auto& owner = get_account(conversion.owner);

      // Calculate STEEM to give based on median price
      asset steem_to_give = conversion.amount × get_feed_history().current_median_history;

      // Update balances and supplies
      adjust_balance(owner, steem_to_give);

      const auto& props = get_dynamic_global_properties();
      modify(props, [&](dynamic_global_property_object& p) {
         p.current_supply += steem_to_give;
         p.current_sbd_supply -= conversion.amount;
         p.virtual_supply += steem_to_give;
         p.virtual_supply -= conversion.amount × get_feed_history().current_median_history;
      });

      remove(conversion);
   }
}
```

### Constants Reference

**File**: [libraries/protocol/include/steem/protocol/config.hpp](libraries/protocol/include/steem/protocol/config.hpp)

```cpp
// SBD Constants
#define STEEM_SBD_STOP_PERCENT           (5 * STEEM_1_PERCENT)   // 500 bp = 5%
#define STEEM_SBD_START_PERCENT          (2 * STEEM_1_PERCENT)   // 200 bp = 2%
#define STEEM_MIN_PAYOUT_SBD             (asset(20, SBD_SYMBOL)) // 0.020 SBD

// Conversion Constants
#define STEEM_CONVERSION_DELAY           (fc::days(3.5))
#define STEEM_CONVERSION_DELAY_PRE_HF_16 (fc::days(7))

// Default Interest
#define STEEM_DEFAULT_SBD_INTEREST_RATE  0
```

---

## Economic Implications

### SBD as Debt

**Key Insight**: SBD is blockchain **debt**, not an asset.

**Implications**:
1. **Liability**: Each SBD is a claim on STEEM supply
2. **Unsustainable Growth**: Unlimited SBD creation would destabilize system
3. **Conservative Limits**: Debt ratio caps protect blockchain solvency
4. **Conversion Risk**: High debt ratios make conversions unreliable

### Price Dynamics

#### When SBD > $1 (Premium)

**Causes**:
- High demand for stable asset
- Limited supply (debt ratio restrictions)
- Speculation / FOMO
- No mechanism to create SBD from STEEM

**Effects**:
- Rewards worth more in USD terms
- Authors benefit from premium
- Internal market arbitrage opportunities
- External market provides liquidity

**Historical**: SBD has traded as high as $13 during bull markets

#### When SBD < $1 (Discount)

**Causes**:
- Low demand / selling pressure
- Concern about STEEM backing
- General crypto bear market
- Conversion mechanism should provide floor

**Effects**:
- Arbitrage opportunity: Buy SBD, convert to STEEM
- Buying pressure should restore peg
- If persistent, may indicate weak STEEM fundamentals

**Rare**: Conversion mechanism usually prevents sustained discounts

#### Why SBD Can't Always Hold $1

**Upside Risk**:
- No way to create SBD on demand (only from rewards)
- Debt ratio limits total supply
- Market demand can exceed available supply

**Downside Protection**:
- Conversion provides guaranteed $1 worth of STEEM
- Arbitrage restores peg when SBD < $1

**Result**: SBD is a **soft peg** with:
- Strong floor at $1 (conversion)
- Weak ceiling (no creation mechanism)

### Impact on Rewards

**High SBD Print Rate** (100%):
- Authors get 25% liquid SBD, 25% liquid STEEM (when price adjusted)
- More stable rewards in USD terms
- Increases SBD supply

**Low SBD Print Rate** (0%):
- Authors get 50% liquid STEEM, no SBD
- More volatile rewards (STEEM price fluctuation)
- Reduces SBD supply

**Trade-off**:
- Stability vs Supply Control
- Author preference vs Blockchain health

### Debt Sustainability

**Healthy Debt Ratio** (< 2%):
- Full SBD printing
- Conversions fully backed
- System sustainable

**Warning Zone** (2% - 5%):
- Reduced SBD printing
- Still sustainable
- Signals caution

**Critical Zone** (> 5%):
- No new SBD creation
- Existing SBD still backed
- Focus on reducing debt

**Disaster Scenario** (> 100%):
- Cannot fulfill all conversions
- System trust broken
- Has never occurred in Steem history

---

## Best Practices

### For Users

**Holding SBD**:
1. ✅ Good for stable value storage (when at peg)
2. ✅ Can trade on internal market
3. ⚠️ Can trade above $1 (premium risk)
4. ⚠️ May not always be available in rewards (debt ratio)

**Converting SBD**:
1. ✅ Use when SBD < $1 for arbitrage profit
2. ⚠️ 3.5 day delay - price risk
3. ⚠️ Don't convert when SBD > $1 (lose premium)
4. ✅ Conversion rate locked at submission

**Savings Account**:
1. ✅ Extra security (3 day withdrawal)
2. ✅ May earn interest (check witness settings)
3. ⚠️ Interest currently disabled on most networks
4. ⚠️ 3 day lockup period

### For Witnesses

**Price Feeds**:
1. ✅ Update at least hourly
2. ✅ Use multiple exchange sources
3. ✅ Average prices (reduce volatility)
4. ✅ Slight bias toward $1 (1% suggestion)
5. ❌ Don't use single source
6. ❌ Don't publish stale data

**Interest Rate**:
1. ✅ Consider debt ratio before setting interest
2. ✅ Coordinate with other witnesses
3. ⚠️ High interest increases SBD supply
4. ⚠️ Zero interest currently recommended

**Parameter Monitoring**:
1. Track debt ratio trends
2. Monitor SBD market price
3. Coordinate print rate expectations
4. Respond to community feedback

### For Developers

**Using SBD in Applications**:
1. ✅ Check `sbd_print_rate` before assuming SBD availability
2. ✅ Use `get_feed_history()` for median price
3. ✅ Handle both SBD and STEEM in reward displays
4. ⚠️ Don't assume SBD = $1 (check market price)
5. ⚠️ Account for conversion delay in UX

**API Queries**:
```bash
# Get current SBD metrics
get_dynamic_global_properties

# Get median price feed
get_feed_history

# Get witness price feeds
get_witnesses

# Get account SBD balance
get_accounts ["username"]
```

---

## Related Documentation

### Core Documentation
- [Vesting and Rewards System](vesting-and-rewards-detailed.md) - How rewards are distributed
- [Internal Market](internal-market.md) - STEEM/SBD trading
- [Chain Parameters](chain-parameters.md) - Witness governance
- [Dynamic Global Properties](dynamic-global-properties.md) - Real-time blockchain state
- [Process Funds Function](process-funds-function.md) - Inflation and reward distribution

### Operations Guides
- [Creating Operations](../development/create-operation.md) - How operations work
- [CLI Wallet Guide](../getting-started/cli-wallet-guide.md) - Using SBD in wallet

### External Resources
- [Steem Whitepaper](https://steem.com/steem-whitepaper.pdf) - Original SBD design
- [Steem Blue Paper](https://steem.com/SteemWhitePaper.pdf) - Economic model

---

## Summary

**Key Takeaways**:

1. **Purpose**: SBD is a debt-backed stablecoin designed to maintain a $1 peg
2. **Creation**: Only created from content rewards, amount determined by `sbd_print_rate`
3. **Debt Ratio**: Automatically limits SBD supply to 2-5% of STEEM market cap
4. **Conversion**: One-way SBD→STEEM conversion at median price (3.5 day delay)
5. **Price Floor**: Conversion mechanism provides strong $1 floor
6. **No Price Ceiling**: SBD can trade above $1 (has reached $13 historically)
7. **Interest**: Optional interest on savings (witness-set, currently ~0%)
8. **Price Feeds**: Witnesses publish prices, median used for conversions
9. **Virtual Supply**: Includes SBD debt, used for inflation and debt ratio calculations
10. **Sustainability**: Conservative debt limits ensure long-term viability

**SBD Print Rate Rules**:
```
Debt < 2%:   100% SBD printing
Debt 2-5%:   Linear reduction
Debt ≥ 5%:   0% SBD printing (STEEM only)
```

**SBD is a sophisticated stablecoin mechanism that balances**:
- ✅ Price stability (conversion floor)
- ✅ Supply sustainability (debt ratio limits)
- ✅ Reward predictability (USD-denominated)
- ⚠️ Soft peg (can trade above $1)
- ⚠️ Conversion delay (3.5 days)

---

**Document Version**: 1.0
**Last Updated**: 2025-11-22
**Code Version**: STEEM 0.0.0
