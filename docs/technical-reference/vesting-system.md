# Vesting System (Steem Power)

## Overview

The Vesting system in the Steem blockchain is the core mechanism behind **Steem Power (SP)**. When users "Power Up" their STEEM, it is converted into vesting shares (VESTS), encouraging long-term holding and earning automatic interest through block inflation.

This document explains how the vesting pool operates, VESTS pricing calculations, and the Power Up/Power Down mechanisms.

## Table of Contents

1. [Vesting Pool Structure](#1-vesting-pool-structure)
2. [VESTS Price Determination](#2-vests-price-determination)
3. [Power Up](#3-power-up)
4. [Power Down](#4-power-down)
5. [Automatic Interest Through Inflation](#5-automatic-interest-through-inflation)
6. [Real-World Scenario Examples](#6-real-world-scenario-examples)

---

## 1. Vesting Pool Structure

### 1.1 Core Components

**File:** [src/core/chain/include/steem/chain/global_property_object.hpp:46-47](../src/core/chain/include/steem/chain/global_property_object.hpp#L46-L47)

```cpp
struct dynamic_global_property_object
{
   asset total_vesting_fund_steem = asset( 0, STEEM_SYMBOL );  // STEEM in vesting pool
   asset total_vesting_shares     = asset( 0, VESTS_SYMBOL );  // Total VESTS issued
   // ...
};
```

**Two key vesting pool variables:**

| Variable | Description | Initial Value |
|----------|-------------|---------------|
| `total_vesting_fund_steem` | Total STEEM deposited in vesting pool | 0 STEEM |
| `total_vesting_shares` | Total vesting shares (VESTS) issued | 0 VESTS |

### 1.2 Three Primary Roles of the Vesting Pool

1. **Deposit Vault**: Holds STEEM that users have powered up
2. **Price Calculation Base**: Determines VESTS ↔ STEEM conversion rate
3. **Interest Pool**: Receives 15% of block inflation to distribute to SP holders

### 1.3 Genesis Initial State

**File:** [src/core/chain/database.cpp:2369-2378](../src/core/chain/database.cpp#L2369-L2378)

```cpp
void database::init_genesis( uint64_t init_supply )
{
   // ...
   create< dynamic_global_property_object >( [&]( dynamic_global_property_object& p )
   {
      p.current_witness = STEEM_GENESIS_WITNESS_NAME;
      p.time = STEEM_GENESIS_TIME;
      p.current_supply = asset( init_supply, STEEM_SYMBOL );
      p.virtual_supply = p.current_supply;
      // total_vesting_fund_steem and total_vesting_shares not explicitly initialized
      // Start with default value of 0
   });
}
```

**Immediately after genesis block creation:**
```
- total_vesting_fund_steem: 0 STEEM
- total_vesting_shares: 0 VESTS
- Vesting pool: Completely empty
```

---

## 2. VESTS Price Determination

### 2.1 Price Calculation Function

**File:** [src/core/chain/include/steem/chain/global_property_object.hpp:53-59](../src/core/chain/include/steem/chain/global_property_object.hpp#L53-L59)

```cpp
price get_vesting_share_price() const
{
   // If vesting pool is empty, use default ratio
   if ( total_vesting_fund_steem.amount == 0 || total_vesting_shares.amount == 0 )
      return price ( asset( 1000, STEEM_SYMBOL ), asset( 1000000, VESTS_SYMBOL ) );

   // Calculate from actual vesting pool ratio
   return price( total_vesting_shares, total_vesting_fund_steem );
}
```

### 2.2 Price Calculation Formula

**Case 1: When vesting pool is empty (initial state)**
```
Default price = 1000 STEEM : 1,000,000 VESTS
i.e., 1 STEEM = 1,000 VESTS
```

**Case 2: When vesting pool has funds**
```
VESTS price = total_vesting_fund_steem / total_vesting_shares

Example:
total_vesting_fund_steem = 1,150,000 STEEM
total_vesting_shares = 1,000,000,000 VESTS
→ 1 VESTS = 1,150,000 / 1,000,000,000 = 0.00115 STEEM
→ 1 STEEM = 1,000,000,000 / 1,150,000 ≈ 869.57 VESTS
```

### 2.3 Reward VESTS Price

**File:** [src/core/chain/include/steem/chain/global_property_object.hpp:61-65](../src/core/chain/include/steem/chain/global_property_object.hpp#L61-L65)

```cpp
price get_reward_vesting_share_price() const
{
   return price( total_vesting_shares + pending_rewarded_vesting_shares,
                 total_vesting_fund_steem + pending_rewarded_vesting_steem );
}
```

When distributing rewards, the price is calculated including pending rewards.

---

## 3. Power Up

### 3.1 Power Up Process

**File:** [src/core/chain/database.cpp:1134-1165](../src/core/chain/database.cpp#L1134-L1165)

```cpp
asset database::create_vesting( const account_object& to_account, asset liquid, bool to_reward_balance )
{
   // 1. Get current VESTS price
   const auto& cprops = get_dynamic_global_properties();
   price vesting_share_price = to_reward_balance ?
      cprops.get_reward_vesting_share_price() :
      cprops.get_vesting_share_price();

   // 2. Convert STEEM to VESTS
   asset new_vesting = liquid * vesting_share_price;

   // 3. Add VESTS to user account
   adjust_balance( to_account, new_vesting );

   // 4. Update global vesting pool
   modify( cprops, [&]( dynamic_global_property_object& props )
   {
      props.total_vesting_fund_steem += liquid;       // Add STEEM
      props.total_vesting_shares += new_vesting;      // Issue VESTS
   });

   // 5. Update witness voting power
   adjust_proxied_witness_votes( to_account, new_vesting.amount );

   return new_vesting;
}
```

### 3.2 Power Up Example

**Scenario: User powers up 1000 STEEM**

```
Before (initial state):
├─ Vesting Pool
│  ├─ total_vesting_fund_steem: 0 STEEM
│  └─ total_vesting_shares: 0 VESTS
├─ VESTS price: 1 STEEM = 1,000 VESTS (default)
└─ User balance: 1000 STEEM

Processing:
├─ 1. Calculate VESTS: 1000 STEEM × 1000 = 1,000,000 VESTS
├─ 2. Add 1,000,000 VESTS to user account
├─ 3. total_vesting_fund_steem += 1000 STEEM
└─ 4. total_vesting_shares += 1,000,000 VESTS

After:
├─ Vesting Pool
│  ├─ total_vesting_fund_steem: 1,000 STEEM
│  └─ total_vesting_shares: 1,000,000 VESTS
├─ VESTS price: 1000 / 1,000,000 = 1 STEEM per 1000 VESTS (same)
└─ User balance: 1,000,000 VESTS (Steem Power)
```

---

## 4. Power Down

### 4.1 Power Down Mechanism

In Steem, power down proceeds in **13 equal weekly installments**.

**Constant definitions:**
```cpp
#define STEEM_VESTING_WITHDRAW_INTERVALS      13
#define STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS (60*60*24*7)  // 1 week
```

### 4.2 Power Down Execution Code

**File:** [src/core/chain/database.cpp:1482-1510](../src/core/chain/database.cpp#L1482-L1510)

```cpp
void database::process_vesting_withdrawals()
{
   // Iterate through accounts powering down
   // ...

   // Convert VESTS to STEEM
   auto converted_steem = asset( to_convert, VESTS_SYMBOL ) * cprops.get_vesting_share_price();

   // Update user account
   modify( from_account, [&]( account_object& a )
   {
      a.vesting_shares.amount -= to_withdraw;   // Deduct VESTS
      a.balance += converted_steem;             // Pay STEEM
      a.withdrawn += to_withdraw;

      if( a.withdrawn >= a.to_withdraw || a.vesting_shares.amount == 0 )
      {
         // Power down complete
         a.vesting_withdraw_rate.amount = 0;
         a.next_vesting_withdrawal = fc::time_point_sec::maximum();
      }
      else
      {
         // Schedule next week
         a.next_vesting_withdrawal += fc::seconds( STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS );
      }
   });

   // Update global vesting pool
   modify( cprops, [&]( dynamic_global_property_object& o )
   {
      o.total_vesting_fund_steem -= converted_steem;  // Remove STEEM
      o.total_vesting_shares.amount -= to_convert;    // Burn VESTS
   });
}
```

### 4.3 Power Down Example

**Scenario: User powers down entire 1,000,000 VESTS**

```
Before:
├─ Vesting Pool
│  ├─ total_vesting_fund_steem: 1,150 STEEM
│  └─ total_vesting_shares: 1,000,000 VESTS
├─ VESTS price: 1.15 STEEM per 1000 VESTS
└─ User holding: 1,000,000 VESTS

Power down setup:
├─ Total withdrawal: 1,000,000 VESTS
├─ 13 week split: 1,000,000 / 13 ≈ 76,923 VESTS per week
└─ Weekly withdrawal: 76,923 VESTS × 1.15 / 1000 ≈ 88.46 STEEM

Week 1 withdrawal:
├─ VESTS burned: 76,923 VESTS
├─ STEEM paid: 88.46 STEEM
├─ total_vesting_fund_steem: 1,150 - 88.46 = 1,061.54 STEEM
├─ total_vesting_shares: 1,000,000 - 76,923 = 923,077 VESTS
└─ New VESTS price: 1,061.54 / 923,077 ≈ 1.15 STEEM per 1000 VESTS

... (repeat for 13 weeks)

After (13 weeks later):
├─ Vesting Pool
│  ├─ total_vesting_fund_steem: 0 STEEM
│  └─ total_vesting_shares: 0 VESTS
└─ User balance: 1,150 STEEM (principal + 15% profit)
```

---

## 5. Automatic Interest Through Inflation

### 5.1 Block Inflation Distribution

**File:** [src/core/chain/database.cpp:1830-1854](../src/core/chain/database.cpp#L1830-L1854)

```cpp
void database::process_funds()
{
   // 1. Calculate current inflation rate
   int64_t current_inflation_rate = std::max(
      start_inflation_rate - inflation_rate_adjustment,
      inflation_rate_floor
   );

   // 2. New STEEM created per block
   auto new_steem = ( props.virtual_supply.amount * current_inflation_rate ) /
                    ( int64_t( STEEM_100_PERCENT ) * int64_t( STEEM_BLOCKS_PER_YEAR ) );

   // 3. Reward distribution ratio
   auto content_reward = ( new_steem * STEEM_CONTENT_REWARD_PERCENT ) / STEEM_100_PERCENT;  // 75%
   auto vesting_reward = ( new_steem * STEEM_VESTING_FUND_PERCENT ) / STEEM_100_PERCENT;   // 15%
   auto witness_reward = new_steem - content_reward - vesting_reward;                       // 10%

   // 4. Add 15% to vesting pool (KEY!)
   modify( props, [&]( dynamic_global_property_object& p )
   {
      p.total_vesting_fund_steem += asset( vesting_reward, STEEM_SYMBOL );  // Only STEEM increases
      p.current_supply           += asset( new_steem, STEEM_SYMBOL );
      p.virtual_supply           += asset( new_steem, STEEM_SYMBOL );
   });
   // Note: total_vesting_shares does NOT increase!
}
```

### 5.2 Inflation Constants

**File:** [src/core/protocol/include/steem/protocol/config.hpp](../src/core/protocol/include/steem/protocol/config.hpp)

| Constant | Value | Description |
|----------|-------|-------------|
| `STEEM_CONTENT_REWARD_PERCENT` | 75% | Content creator rewards |
| `STEEM_VESTING_FUND_PERCENT` | 15% | Vesting pool (SP holders) rewards |
| **Witness rewards** | 10% | Remainder (block producers) |
| `STEEM_INFLATION_RATE_START_PERCENT` | 978 (9.78%) | Initial inflation rate |
| `STEEM_INFLATION_RATE_STOP_PERCENT` | 95 (0.95%) | Final inflation rate |
| `STEEM_INFLATION_NARROWING_PERIOD` | 250,000 blocks | 0.01% decrease period |

### 5.3 Automatic Interest Principle

**Core Mechanism:**

```
Every block:
1. 15% of inflation added to vesting pool
2. total_vesting_fund_steem increases ↑
3. total_vesting_shares stays same →
4. Result: VESTS price rises ↑
```

**Why it benefits SP holders:**

```
Before:
- Holding: 1,000,000 VESTS
- VESTS price: 1.00 STEEM per 1000 VESTS
- Actual value: 1,000 STEEM

1 year later (15% inflation accumulated):
- Holding: 1,000,000 VESTS (unchanged)
- VESTS price: 1.15 STEEM per 1000 VESTS (15% increase)
- Actual value: 1,150 STEEM (15% increase!)

→ Automatic 15% return without doing anything
```

---

## 6. Real-World Scenario Examples

### Scenario 1: First Power Up After Genesis

```
Block 1 (Genesis):
├─ total_vesting_fund_steem: 0 STEEM
├─ total_vesting_shares: 0 VESTS
└─ VESTS default price: 1 STEEM = 1,000 VESTS

Block 2:
├─ Witness (genesis) receives block production reward
│  ├─ Witness reward: 100 STEEM (paid as vesting)
│  ├─ VESTS issued: 100 × 1000 = 100,000 VESTS
│  ├─ total_vesting_fund_steem: 100 STEEM
│  └─ total_vesting_shares: 100,000 VESTS
├─ 15% inflation added to vesting pool
│  ├─ Vesting reward: 15 STEEM
│  ├─ total_vesting_fund_steem: 100 + 15 = 115 STEEM
│  └─ total_vesting_shares: 100,000 VESTS (unchanged)
└─ New VESTS price: 115 / 100,000 = 1.15 STEEM per 1000 VESTS

Result:
- genesis account holds: 100,000 VESTS
- Actual value: 100,000 × 1.15 / 1000 = 115 STEEM
- Profit: 15 STEEM (15%)
```

### Scenario 2: Multiple User Power Up and Inflation

```
Initial state:
├─ total_vesting_fund_steem: 115 STEEM
├─ total_vesting_shares: 100,000 VESTS
└─ VESTS price: 1.15 STEEM per 1000 VESTS

User A powers up 1000 STEEM:
├─ Calculate VESTS: 1000 / (1.15 / 1000) ≈ 869,565 VESTS
├─ total_vesting_fund_steem: 115 + 1000 = 1,115 STEEM
├─ total_vesting_shares: 100,000 + 869,565 = 969,565 VESTS
└─ New price: 1,115 / 969,565 ≈ 1.15 STEEM per 1000 VESTS (maintained)

1000 blocks later (inflation accumulated: 150 STEEM):
├─ total_vesting_fund_steem: 1,115 + 150 = 1,265 STEEM
├─ total_vesting_shares: 969,565 VESTS (unchanged)
└─ New price: 1,265 / 969,565 ≈ 1.30 STEEM per 1000 VESTS

Value by holder:
├─ genesis (100,000 VESTS): 100,000 × 1.30 / 1000 = 130 STEEM
│  └─ Profit: 30 STEEM (30%)
└─ User A (869,565 VESTS): 869,565 × 1.30 / 1000 ≈ 1,130 STEEM
   └─ Profit: 130 STEEM (13%)
```

### Scenario 3: Power Down and Inflation Running Simultaneously

```
Initial state:
├─ total_vesting_fund_steem: 1,265 STEEM
├─ total_vesting_shares: 969,565 VESTS
├─ VESTS price: 1.30 STEEM per 1000 VESTS
└─ User A: holding 869,565 VESTS

User A starts full power down:
├─ 13 week split: 869,565 / 13 ≈ 66,890 VESTS/week
└─ Weekly withdrawal: 66,890 × 1.30 / 1000 ≈ 86.96 STEEM

Week 1:
├─ VESTS burned: 66,890
├─ STEEM paid: 86.96 STEEM
├─ total_vesting_fund_steem: 1,265 - 86.96 = 1,178.04 STEEM
├─ total_vesting_shares: 969,565 - 66,890 = 902,675 VESTS
├─ Block inflation (7 days × ~0.5 STEEM/day): +3.5 STEEM
├─ total_vesting_fund_steem: 1,178.04 + 3.5 = 1,181.54 STEEM
└─ New price: 1,181.54 / 902,675 ≈ 1.31 STEEM per 1000 VESTS

After 13 weeks (complete):
├─ Total STEEM withdrawn: ~1,130 STEEM (accounting for inflation)
├─ Vesting Pool
│  ├─ total_vesting_fund_steem: ~135 STEEM (only genesis remains)
│  └─ total_vesting_shares: 100,000 VESTS
└─ genesis value continues to increase
```

---

## Summary

### Core Mechanisms

1. **Vesting Pool Initialization**
   - Genesis: 0 STEEM, 0 VESTS
   - Default price: 1 STEEM = 1,000 VESTS

2. **Dynamic Growth**
   - Power Up: Add STEEM to pool + issue VESTS
   - Inflation: Only add STEEM to pool (15% every block)
   - Power Down: Remove STEEM from pool + burn VESTS

3. **Automatic Interest**
   - SP holding = VESTS holding
   - Every block inflation → VESTS price rises
   - Automatic profit without doing anything

### Formula Summary

```cpp
// VESTS price
VESTS_price = total_vesting_fund_steem / total_vesting_shares

// Power Up (STEEM → VESTS)
new_vests = steem_amount / VESTS_price

// Power Down (VESTS → STEEM)
steem_amount = vests_amount × VESTS_price

// Expected annual return (based on 15% inflation)
APR ≈ 15% (only vesting pool increases, total VESTS unchanged)
```

### Design Intent

1. **Encourage Long-term Holding**: 13-week power down prevents short-term speculation
2. **Automatic Interest**: Distribute inflation to SP holders
3. **Voting Rights**: SP = witness/content voting power
4. **Network Stability**: Long-term holders participate in governance

### Code Location References

- **Vesting Pool Definition:** [src/core/chain/include/steem/chain/global_property_object.hpp:46-47](../src/core/chain/include/steem/chain/global_property_object.hpp#L46-L47)
- **VESTS Price Calculation:** [src/core/chain/include/steem/chain/global_property_object.hpp:53-59](../src/core/chain/include/steem/chain/global_property_object.hpp#L53-L59)
- **Power Up Logic:** [src/core/chain/database.cpp:1134-1165](../src/core/chain/database.cpp#L1134-L1165)
- **Power Down Logic:** [src/core/chain/database.cpp:1482-1510](../src/core/chain/database.cpp#L1482-L1510)
- **Inflation Distribution:** [src/core/chain/database.cpp:1830-1854](../src/core/chain/database.cpp#L1830-L1854)
- **Constant Definitions:** [src/core/protocol/include/steem/protocol/config.hpp](../src/core/protocol/include/steem/protocol/config.hpp)

This design implements a **"Staking as Earning"** mechanism where the Steem blockchain provides automatic interest to token holders while encouraging long-term participation.
