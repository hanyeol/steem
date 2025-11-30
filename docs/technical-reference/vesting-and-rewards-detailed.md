# STEEM Vesting and Rewards System

## Table of Contents
1. [Overview](#overview)
2. [Vesting System](#vesting-system)
3. [Reward Pool System](#reward-pool-system)
4. [Reward Calculation Mechanism](#reward-calculation-mechanism)
5. [Dynamic Global Properties](#dynamic-global-properties)
6. [Code Implementation Details](#code-implementation-details)
7. [Real-World Examples](#real-world-examples)

---

## Overview

The STEEM blockchain distributes rewards to content creators and curators through a "Proof of Brain" algorithm. The core of this system is **Vesting** and **Reward Pool**, which work closely together.

### Core Concepts

- **STEEM**: The blockchain's base token (liquid)
- **VESTS (Vesting Shares)**: Internal representation unit of STEEM Power
- **STEEM Power**: Vested STEEM (illiquid, grants influence)
- **Reward Pool**: STEEM storage to be distributed to content creators and curators

---

## Vesting System

### 1. Purpose of Vesting

The vesting system achieves the following objectives:

1. **Long-term Participation Incentive**: Locking STEEM encourages users to contribute to the network long-term
2. **Granting Influence**: Vested STEEM is converted to Voting Power, allowing influence over content
3. **Inflation Rewards**: STEEM Power holders earn additional STEEM through inflation

### 2. Relationship Between STEEM and VESTS

#### Price Determination Formula

```
vesting_share_price = total_vesting_shares / total_vesting_fund_steem
```

Code implementation ([global_property_object.hpp:53-59](src/core/chain/include/steem/chain/global_property_object.hpp#L53-L59)):

```cpp
price get_vesting_share_price() const
{
   if ( total_vesting_fund_steem.amount == 0 || total_vesting_shares.amount == 0 )
      return price ( asset( 1000, STEEM_SYMBOL ), asset( 1000000, VESTS_SYMBOL ) );

   return price( total_vesting_shares, total_vesting_fund_steem );
}
```

#### Initial Values

Network initial state ([config.hpp:35,63](src/core/protocol/include/steem/protocol/config.hpp#L35)):

**Display values**:
- `1 STEEM = 1,000 VESTS` (base ratio)
- `STEEM_INIT_SUPPLY = 0` (genesis supply)
- This ratio changes over time due to inflation and rewards

**Internal values (in satoshi units)**:

At genesis block start:
```cpp
// Dynamic Global Properties initial state
total_vesting_fund_steem.amount = 0        // 0.000 STEEM (0 satoshi)
total_vesting_shares.amount = 0            // 0.000000 VESTS (0 micro-vests)
total_reward_fund_steem.amount = 0         // 0.000 STEEM
total_reward_shares2 = 0                   // No claims
pending_rewarded_vesting_shares.amount = 0 // 0.000000 VESTS
pending_rewarded_vesting_steem.amount = 0  // 0.000 STEEM

current_supply.amount = 0                  // 0.000 STEEM
virtual_supply.amount = 0                  // 0.000 STEEM
current_sbd_supply.amount = 0              // 0.000 SBD
```

Until the first vesting occurs, the default ratio applies ([global_property_object.hpp:55-56](src/core/chain/include/steem/chain/global_property_object.hpp#L55-L56)):
```cpp
if ( total_vesting_fund_steem.amount == 0 || total_vesting_shares.amount == 0 )
   return price( asset( 1000, STEEM_SYMBOL ), asset( 1000000, VESTS_SYMBOL ) );
   //            ^^^^                         ^^^^^^^^
   //            1.000 STEEM                  1.000000 VESTS
   //            (1,000 satoshi)              (1,000,000 micro-vests)
```

#### Satoshi Unit Representation

Precision of STEEM and VESTS ([asset_symbol.hpp:13-15](src/core/protocol/include/steem/protocol/asset_symbol.hpp#L13-L15)):
- **STEEM precision**: 3 (= 10³ = 1,000)
  - 1 STEEM = 1,000 satoshi (minimum unit = 0.001 STEEM)
- **VESTS precision**: 6 (= 10⁶ = 1,000,000)
  - 1 VESTS = 1,000,000 micro-vests (minimum unit = 0.000001 VESTS)

**Base ratio in minimum units**:
```
1.000 STEEM = 1,000.000000 VESTS
1,000 satoshi = 1,000,000,000 micro-vests

Ratio: 1 satoshi STEEM = 1,000,000 micro-vests
      (0.001 of 1 STEEM = 1 VESTS)
```

**Actual internal stored values**:
```cpp
// Actual integer values used in code
asset( 1000, STEEM_SYMBOL )     // = 1.000 STEEM (display value)
asset( 1000000, VESTS_SYMBOL )  // = 1.000000 VESTS (display value)

// Initial ratio setup (global_property_object.hpp:56)
price( asset( 1000, STEEM_SYMBOL ), asset( 1000000, VESTS_SYMBOL ) )
// = 1,000 / 1,000,000 = 0.001 STEEM/VESTS
// Inverse = 1,000 VESTS/STEEM
```

#### Dynamic Changes

Vesting price changes due to:
- New STEEM being vested
- Rewards paid in STEEM Power
- Power Down converting VESTS to STEEM

### 3. STEEM Power Creation Process

When a user converts STEEM to STEEM Power ([database.cpp:1111-1130](src/core/chain/database.cpp#L1111-L1130)):

```cpp
asset database::create_vesting( const account_object& to_account, asset liquid, bool to_reward_balance )
{
   auto calculate_new_vesting = [ liquid ] ( price vesting_share_price ) -> asset
   {
      /**
       *  The ratio of total_vesting_shares / total_vesting_fund_steem should not
       *  change as a result of the user adding funds
       *
       *  V / C  = (V+Vn) / (C+Cn)
       *
       *  Simplifying yields Vn = (V * Cn ) / C
       *
       *  If Cn equals liquid.amount, we need to calculate the
       *  new vesting shares (Vn) the user should receive.
       *
       *  128-bit operations are required due to multiplication of 64-bit numbers.
       */
      asset new_vesting = liquid * ( vesting_share_price );
      return new_vesting;
   }
}
```

#### Calculation Example

Current state:
- `total_vesting_fund_steem = 100,000 STEEM`
- `total_vesting_shares = 100,000,000 VESTS`
- Ratio: `1 STEEM = 1,000 VESTS`

User powers up 100 STEEM:
- New VESTS = `100 STEEM * 1,000 = 100,000 VESTS`
- Updated `total_vesting_fund_steem = 100,100 STEEM`
- Updated `total_vesting_shares = 100,100,000 VESTS`
- Ratio maintained at `1 STEEM = 1,000 VESTS`

### 4. Reward Vesting Price

Price calculation considering future rewards ([global_property_object.hpp:61-65](src/core/chain/include/steem/chain/global_property_object.hpp#L61-L65)):

```cpp
price get_reward_vesting_share_price() const
{
   return price( total_vesting_shares + pending_rewarded_vesting_shares,
      total_vesting_fund_steem + pending_rewarded_vesting_steem );
}
```

This price is calculated including pending rewards not yet claimed.

### 5. Power Down

Process of converting STEEM Power back to liquid STEEM:

- **Period**: 13 weeks (STEEM_VESTING_WITHDRAW_INTERVALS = 13)
- **Interval**: Weekly (STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS = 604,800 seconds)
- **Method**: Converts 1/13 to STEEM each week
- **Cancellable**: Can be cancelled anytime during power down

Configuration constants ([config.hpp:92-93](src/core/protocol/include/steem/protocol/config.hpp#L92-L93)):

```cpp
#define STEEM_VESTING_WITHDRAW_INTERVALS      13
#define STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS (60*60*24*7) // 1 week per interval
```

---

## Reward Pool System

### 1. Reward Pool Structure

STEEM's reward system is managed through `reward_fund_object` ([steem_objects.hpp:257-276](src/core/chain/include/steem/chain/steem_objects.hpp#L257-L276)):

```cpp
class reward_fund_object : public object< reward_fund_object_type, reward_fund_object >
{
   reward_fund_id_type     id;
   reward_fund_name_type   name;                    // Pool name (e.g., "post", "comment")
   asset                   reward_balance;           // Currently available reward STEEM
   fc::uint128_t           recent_claims;            // Recent claim amount (with decay)
   time_point_sec          last_update;              // Last update time
   uint128_t               content_constant;         // Reward curve constant
   uint16_t                percent_curation_rewards; // Curation reward percentage
   uint16_t                percent_content_rewards;  // Content reward percentage
   protocol::curve_id      author_reward_curve;      // Reward curve type
   protocol::curve_id      curation_reward_curve;    // Curation curve type
};
```

### 2. Types of Reward Pools

STEEM can have multiple reward pools, the main ones being:

- **Post Reward Pool** (`STEEM_POST_REWARD_FUND_NAME = "post"`)
  - Rewards for main posts
- **Comment Reward Pool** (`STEEM_COMMENT_REWARD_FUND_NAME = "comment"`)
  - Rewards for comments

### 3. Reward Pool Funding

Reward pools are funded through inflation:

- **Content reward percentage**: 75% of inflation (STEEM_CONTENT_REWARD_PERCENT = 7,500)
- **Vesting fund percentage**: 15% of inflation (STEEM_VESTING_FUND_PERCENT = 1,500)

Configuration constants ([config.hpp:117-118](src/core/protocol/include/steem/protocol/config.hpp#L117-L118)):

```cpp
#define STEEM_CONTENT_REWARD_PERCENT          (75*STEEM_1_PERCENT) // 75% of inflation
#define STEEM_VESTING_FUND_PERCENT            (15*STEEM_1_PERCENT) // 15% of inflation
```

### 4. Recent Claims and Decay

`recent_claims` tracks recent reward claims and decays over time.

Decay period: 15 days (STEEM_RECENT_RSHARES_DECAY_TIME)

Code implementation ([database.cpp:1715-1734](src/core/chain/database.cpp#L1715-L1734)):

```cpp
void database::process_comment_cashout()
{
   // Decay recent rshares for each reward fund
   for( auto itr = reward_idx.begin(); itr != reward_idx.end(); ++itr )
   {
      modify( *itr, [&]( reward_fund_object& rfo )
      {
         fc::microseconds decay_time = STEEM_RECENT_RSHARES_DECAY_TIME;

         // Exponential decay formula
         rfo.recent_claims -= ( rfo.recent_claims * ( head_block_time() - rfo.last_update ).to_seconds() )
                              / decay_time.to_seconds();
         rfo.last_update = head_block_time();
      });
   }
}
```

#### Decay Formula

```
new_claims = old_claims - (old_claims * elapsed_time / decay_time)
```

This is a linear approximation of exponential decay.

---

## Reward Calculation Mechanism

### 1. Reward Shares (rshares)

Votes are converted to `rshares` (reward shares):

- Voting Power
- Amount of STEEM Power
- Vote weight (1% ~ 100%)

### 2. Reward Curve

`rshares` are converted to "claimable reward shares" through a reward curve.

#### Curve Types

STEEM supports 4 reward curves ([reward.cpp:68-95](src/core/chain/util/reward.cpp#L68-L95)):

```cpp
uint128_t evaluate_reward_curve( const uint128_t& rshares,
                                  const protocol::curve_id& curve,
                                  const uint128_t& content_constant )
{
   uint128_t result = 0;

   switch( curve )
   {
      case protocol::quadratic:
         {
            // Quadratic curve: (rshares + s)^2 - s^2
            uint128_t rshares_plus_s = rshares + content_constant;
            result = rshares_plus_s * rshares_plus_s - content_constant * content_constant;
         }
         break;

      case protocol::quadratic_curation:
         {
            // Curation quadratic curve: rshares^2 / (2α + rshares)
            uint128_t two_alpha = content_constant * 2;
            result = uint128_t( rshares.lo, 0 ) / ( two_alpha + rshares );
         }
         break;

      case protocol::linear:
         // Linear: input as-is
         result = rshares;
         break;

      case protocol::square_root:
         // Square root: sqrt(rshares)
         result = approx_sqrt( rshares );
         break;
   }

   return result;
}
```

#### 1) Quadratic

```
result = (rshares + s)^2 - s^2
```

Where `s = STEEM_CONTENT_CONSTANT = 2,000,000,000,000`

**Characteristics**:
- Gentle increase at initial rshares
- Rewards increase exponentially as rshares grow
- More rewards concentrated on high-quality content

**Example**:
- rshares = 1,000,000
- result = (1,000,000 + 2,000,000,000,000)^2 - (2,000,000,000,000)^2
- ≈ 4,000,000,000,000,000,000

#### 2) Linear

```
result = rshares
```

**Characteristics**:
- Simplest form
- Proportional relationship between rshares and rewards
- Fair distribution to all content

#### 3) Square Root

```
result = sqrt(rshares)
```

**Characteristics**:
- Reward growth rate decreases as rshares increase
- Limits influence of minority whales
- More equal reward distribution

**Example**:
- rshares = 1,000,000
- result = sqrt(1,000,000) = 1,000

#### 4) Quadratic Curation

```
result = rshares^2 / (2α + rshares)
```

Where `α = content_constant`

**Characteristics**:
- Dedicated to curation rewards
- More rewards for early voters

### 3. total_reward_shares2

This value is the sum of reward claims for all content.

#### Update Process

1. When a new vote occurs:
   ```
   curve_rshares = evaluate_reward_curve(rshares)
   total_reward_shares2 += curve_rshares
   ```

2. When content receives rewards:
   ```
   total_reward_shares2 -= curve_rshares
   ```

#### Meaning of "2"

The "2" in the variable name is for historical reasons:
- Initially only quadratic curve was used (square = ^2)
- Now supports multiple curves but keeps the name for compatibility

### 4. Actual Reward Calculation

When content rewards are paid ([reward.cpp:38-66](src/core/chain/util/reward.cpp#L38-L66)):

```cpp
uint64_t get_rshare_reward( const comment_reward_context& ctx )
{
   FC_ASSERT( ctx.rshares > 0 );
   FC_ASSERT( ctx.total_reward_shares2 > 0 );

   // Calculate with 256-bit precision
   u256 rf(ctx.total_reward_fund_steem.amount.value);
   u256 total_claims = to256( ctx.total_reward_shares2 );

   // Calculate claims for this content
   u256 claim = to256( evaluate_reward_curve( ctx.rshares.value,
                                               ctx.reward_curve,
                                               ctx.content_constant ) );

   // Apply reward weight (e.g., 5000/10000 for 50% payout)
   claim = ( claim * ctx.reward_weight ) / STEEM_100_PERCENT;

   // Proportional distribution: (reward_fund * claim) / total_claims
   u256 payout_u256 = ( rf * claim ) / total_claims;

   uint64_t payout = static_cast< uint64_t >( payout_u256 );

   // Dust cleanup: zero if below minimum payout
   if( is_comment_payout_dust( ctx.current_steem_price, payout ) )
      payout = 0;

   // Apply maximum SBD limit
   asset max_steem = to_steem( ctx.current_steem_price, ctx.max_sbd );
   payout = std::min( payout, uint64_t( max_steem.amount.value ) );

   return payout;
}
```

#### Calculation Formula

```
payout = (total_reward_fund_steem * curve_rshares * reward_weight) / (total_reward_shares2 * STEEM_100_PERCENT)
```

#### Example Calculation

Current state:
- `total_reward_fund_steem = 1,000,000 STEEM`
- `total_reward_shares2 = 100,000,000,000,000`
- Content's `curve_rshares = 1,000,000,000,000`
- `reward_weight = 10,000` (100%)

Calculation:
```
payout = (1,000,000 * 1,000,000,000,000 * 10,000) / (100,000,000,000,000 * 10,000)
       = 10,000 STEEM
```

### 5. Author and Curator Distribution

Once total rewards are calculated, they're distributed to authors and curators:

- **Curators**: 25% of rewards (typically)
- **Authors**: 75% of rewards

Author rewards are further split:
- **STEEM Power**: 50%
- **SBD**: 50% (or STEEM, depending on SBD print rate)

### 6. Reverse Auction

Votes during the first 30 minutes have some curator rewards returned to the author:

- **Period**: 30 minutes (STEEM_REVERSE_AUCTION_WINDOW_SECONDS = 1,800 seconds)
- **Mechanism**: Earlier votes return more rewards to the author

Configuration constant ([config.hpp:99](src/core/protocol/include/steem/protocol/config.hpp#L99)):

```cpp
#define STEEM_REVERSE_AUCTION_WINDOW_SECONDS  (60*30) // 30 minutes
```

---

## Dynamic Global Properties

### Dynamic Global Property Object

All vesting and reward-related state is managed in `dynamic_global_property_object`.

#### Vesting-Related Properties

| Property | Type | Description | Initial Value |
|----------|------|-------------|---------------|
| `total_vesting_fund_steem` | `asset` | Total amount of vested STEEM | 0 STEEM |
| `total_vesting_shares` | `asset` | Total amount of all existing VESTS | 0 VESTS |

**Meaning**:
- The ratio of these two values determines STEEM ↔ VESTS exchange rate
- All vesting operations update both values simultaneously to maintain the ratio

#### Reward Pool-Related Properties

| Property | Type | Description | Initial Value |
|----------|------|-------------|---------------|
| `total_reward_fund_steem` | `asset` | Total STEEM in reward pool | 0 STEEM |
| `total_reward_shares2` | `fc::uint128` | Sum of all claimable reward shares | 0 |

**Meaning**:
- `total_reward_fund_steem`: Total amount of currently distributable rewards
- `total_reward_shares2`: Sum of reward claims for all content
- Actual reward = `(reward_fund * my_claim) / total_reward_shares2`

#### Pending Reward-Related Properties

| Property | Type | Description | Initial Value |
|----------|------|-------------|---------------|
| `pending_rewarded_vesting_shares` | `asset` | VESTS not yet paid out | 0 VESTS |
| `pending_rewarded_vesting_steem` | `asset` | STEEM backing pending VESTS | 0 STEEM |

**Meaning**:
- VESTS waiting to be claimed
- Used in future exchange rate calculations (`get_reward_vesting_share_price()`)
- Moved to `total_vesting_*` upon actual claim

### Update Scenarios

#### Scenario 1: User Powers Up STEEM

**Before**:
```
total_vesting_fund_steem = 100,000 STEEM
total_vesting_shares = 100,000,000 VESTS
```

**User action**: Power up 1,000 STEEM

**After**:
```
total_vesting_fund_steem = 101,000 STEEM
total_vesting_shares = 101,000,000 VESTS
User account: +1,000,000 VESTS
```

#### Scenario 2: Voting on Content

**Before**:
```
total_reward_shares2 = 50,000,000,000,000
```

**Vote action**: rshares = 1,000,000

**After**:
```
curve_rshares = evaluate_reward_curve(1,000,000) = 1,000,000,000 (assumed)
total_reward_shares2 = 50,001,000,000,000
```

#### Scenario 3: Content Reward Payout

**Before**:
```
total_reward_fund_steem = 1,000,000 STEEM
total_reward_shares2 = 100,000,000,000,000
total_vesting_fund_steem = 100,000 STEEM
total_vesting_shares = 100,000,000 VESTS
pending_rewarded_vesting_shares = 0 VESTS
pending_rewarded_vesting_steem = 0 STEEM
```

**Reward payout**: Content reward 100 STEEM (50 STEEM as STEEM Power, 50 STEEM as SBD)

**During**:
```
1. Deduct from reward pool
   total_reward_fund_steem = 999,900 STEEM

2. Deduct claims
   total_reward_shares2 -= content's curve_rshares

3. Convert 50 STEEM to VESTS and add to pending queue
   pending_rewarded_vesting_shares = 50,000 VESTS
   pending_rewarded_vesting_steem = 50 STEEM
```

**After** (user claims):
```
total_vesting_fund_steem = 100,050 STEEM
total_vesting_shares = 100,050,000 VESTS
pending_rewarded_vesting_shares = 0 VESTS
pending_rewarded_vesting_steem = 0 STEEM
User account: +50,000 VESTS
```

---

## Code Implementation Details

### Key File Locations

#### 1. Data Structures

- **[global_property_object.hpp](src/core/chain/include/steem/chain/global_property_object.hpp)**
  - `dynamic_global_property_object` definition
  - Vesting price calculation functions

- **[steem_objects.hpp](src/core/chain/include/steem/chain/steem_objects.hpp)**
  - `reward_fund_object` definition
  - Reward pool-related objects

#### 2. Reward Calculation

- **[util/reward.hpp](src/core/chain/include/steem/chain/util/reward.hpp)**
  - `comment_reward_context` structure
  - Reward calculation function declarations

- **[util/reward.cpp](src/core/chain/util/reward.cpp)**
  - `evaluate_reward_curve()` implementation
  - `get_rshare_reward()` implementation
  - Square root approximation function

#### 3. Database Operations

- **[database.cpp](src/core/chain/database.cpp)**
  - `create_vesting()` - VESTS creation
  - `process_comment_cashout()` - Reward payout processing
  - `adjust_total_payout()` - Payout record update
  - `pay_curators()` - Curator reward distribution

#### 4. Configuration Constants

- **[protocol/config.hpp](src/core/protocol/include/steem/protocol/config.hpp)**
  - All system constant definitions
  - Inflation, rewards, timing-related settings

### Core Function Flow

#### Vesting Creation Flow

```
transfer_to_vesting_operation
    ↓
transfer_to_vesting_evaluator::do_apply()
    ↓
database::create_vesting()
    ↓
[Price calculation]
    ↓
[Global properties update]
    ↓
[User account VESTS increase]
```

#### Reward Payout Flow

```
process_comment_cashout()
    ↓
[Reward pool decay processing]
    ↓
[Find content for cashout]
    ↓
get_rshare_reward()
    ↓
[Curator reward calculation]
pay_curators()
    ↓
[Author reward calculation]
    ↓
[STEEM Power conversion]
create_vesting()
    ↓
adjust_total_payout()
```

### Important Validation Logic

#### 1. Overflow Prevention

Use 256-bit integers for reward calculation:

```cpp
u256 rf(ctx.total_reward_fund_steem.amount.value);
u256 total_claims = to256( ctx.total_reward_shares2 );
u256 claim = to256( evaluate_reward_curve(...) );
u256 payout_u256 = ( rf * claim ) / total_claims;
```

#### 2. Minimum Payout Validation

Prevent dust payouts:

```cpp
#define STEEM_MIN_PAYOUT_SBD (asset(20,SBD_SYMBOL))

inline bool is_comment_payout_dust( const price& p, uint64_t steem_payout )
{
   return to_sbd( p, asset( steem_payout, STEEM_SYMBOL ) ) < STEEM_MIN_PAYOUT_SBD;
}
```

SBD value less than 0.020 SBD is not paid

#### 3. Ratio Consistency Maintenance

Always maintain ratio when creating/destroying vesting:

```cpp
// On creation
Vn = (V * Cn) / C

// On destruction
Cn = (C * Vn) / V
```

---

## Real-World Examples

### Example 1: Popular Post Reward Calculation

#### Initial State
```
total_reward_fund_steem: 800,000 STEEM
total_reward_shares2: 5,000,000,000,000,000
content_constant: 2,000,000,000,000
reward_curve: quadratic
```

#### Post State
```
Total rshares: 50,000,000,000
Number of voters: 500
Time elapsed since posting: 7 days (cashout time)
```

#### Calculation Process

**Step 1: Calculate Curve rshares**

```
curve_rshares = (50,000,000,000 + 2,000,000,000,000)^2 - (2,000,000,000,000)^2
              ≈ 204,100,000,000,000,000
```

**Step 2: Proportional Reward Distribution**

```
claim = 204,100,000,000,000,000
total_claims = 5,000,000,000,000,000

payout = (800,000 * 204,100,000,000,000,000) / 5,000,000,000,000,000
       = 32,656 STEEM
```

**Step 3: Curator/Author Distribution**

```
Curator reward: 32,656 * 0.25 = 8,164 STEEM
Author reward: 32,656 * 0.75 = 24,492 STEEM
```

**Step 4: Author Reward Split**

```
STEEM Power: 24,492 * 0.50 = 12,246 STEEM → approximately 12,246,000 VESTS
SBD: 24,492 * 0.50 = 12,246 SBD
```

**Step 5: Global Properties Update**

```
total_reward_fund_steem: 800,000 - 32,656 = 767,344 STEEM
total_reward_shares2: 5,000,000,000,000,000 - 204,100,000,000,000,000
                    = 4,795,900,000,000,000
pending_rewarded_vesting_steem: +12,246 STEEM
pending_rewarded_vesting_shares: +12,246,000 VESTS
```

### Example 2: Small Comment Reward

#### Initial State
```
total_reward_fund_steem: 800,000 STEEM
total_reward_shares2: 5,000,000,000,000,000
content_constant: 2,000,000,000,000
reward_curve: linear (assuming comments use linear curve)
```

#### Comment State
```
Total rshares: 100,000
Number of voters: 5
```

#### Calculation Process

**Step 1: Calculate Curve rshares (linear)**

```
curve_rshares = 100,000 (as-is for linear)
```

**Step 2: Proportional Reward Distribution**

```
payout = (800,000 * 100,000) / 5,000,000,000,000,000
       = 0.016 STEEM
```

**Step 3: Dust Validation**

```
0.016 STEEM ≈ 0.016 SBD < 0.020 SBD (STEEM_MIN_PAYOUT_SBD)
→ Reward not paid (payout = 0)
```

This comment doesn't receive rewards due to being below minimum payout.

### Example 3: Large Power Up

#### Initial State
```
total_vesting_fund_steem: 500,000,000 STEEM
total_vesting_shares: 500,000,000,000 VESTS
Ratio: 1 STEEM = 1,000 VESTS
```

#### Whale Action
```
User powers up 10,000,000 STEEM
```

#### Calculation Process

**Step 1: Calculate New VESTS**

```
vesting_share_price = 500,000,000,000 / 500,000,000 = 1,000 VESTS/STEEM

new_vests = 10,000,000 * 1,000 = 10,000,000,000 VESTS
```

**Step 2: Global Properties Update**

```
total_vesting_fund_steem: 500,000,000 + 10,000,000 = 510,000,000 STEEM
total_vesting_shares: 500,000,000,000 + 10,000,000,000 = 510,000,000,000 VESTS

New ratio: 510,000,000,000 / 510,000,000 = 1,000 VESTS/STEEM (maintained)
```

**Step 3: User Account Update**

```
User balance: -10,000,000 STEEM
User vesting_shares: +10,000,000,000 VESTS
```

The ratio is maintained exactly so existing holders are not disadvantaged.

### Example 4: 13-Week Power Down

#### Initial State
```
User VESTS: 13,000,000 VESTS
total_vesting_fund_steem: 510,000,000 STEEM
total_vesting_shares: 510,000,000,000 VESTS
Ratio: 1 STEEM = 1,000 VESTS
```

#### Power Down Start
```
User starts power down of all VESTS
```

#### Calculation Process

**Calculate Weekly Payout**

```
VESTS per week = 13,000,000 / 13 = 1,000,000 VESTS
STEEM per week = 1,000,000 / 1,000 = 1,000 STEEM
```

**After Week 1 Payout**

```
total_vesting_fund_steem: 510,000,000 - 1,000 = 509,999,000 STEEM
total_vesting_shares: 510,000,000,000 - 1,000,000 = 509,999,000,000 VESTS
User VESTS: 13,000,000 - 1,000,000 = 12,000,000 VESTS
User STEEM: +1,000 STEEM

New ratio: 509,999,000,000 / 509,999,000 ≈ 1,000 VESTS/STEEM (approximately maintained)
```

**After 13 Weeks (complete)**

```
total_vesting_fund_steem: 510,000,000 - 13,000 = 509,987,000 STEEM
total_vesting_shares: 510,000,000,000 - 13,000,000 = 509,987,000,000 VESTS
User VESTS: 0 VESTS
User STEEM: +13,000 STEEM
```

### Example 5: Reward Pool Decay

#### Initial State (Day 0)
```
recent_claims: 1,000,000,000,000,000
last_update: Day 0 00:00
```

#### After Day 1 (no activity)
```
elapsed_time = 1 day = 86,400 seconds
decay_time = 15 days = 1,296,000 seconds

decay_amount = 1,000,000,000,000,000 * 86,400 / 1,296,000
             = 66,666,666,666,667

new_recent_claims = 1,000,000,000,000,000 - 66,666,666,666,667
                  = 933,333,333,333,333
```

**Decay rate**: approximately 6.67% per day

#### After Day 15 (no activity)
```
Theoretically e^(-1) ≈ 36.8% remains
With actual linear approximation, approaches zero
```

---

## Summary

### Key Concepts Summary

1. **Vesting System**
   - STEEM ↔ VESTS exchange rate changes dynamically
   - Ratio is consistently maintained in all operations
   - 13-week power down provides liquidity

2. **Reward Pool**
   - Funded by inflation
   - Recent claims decay over 15-day cycle
   - Fair distribution through proportional allocation

3. **Reward Curves**
   - Quadratic: Concentrates on high-quality content
   - Linear: Equal distribution
   - Square Root: Limits whale influence
   - Choice determined by hardfork and community consensus

4. **Dynamic Global Properties**
   - Central storage for all vesting/reward state
   - Updated in real-time
   - Queryable via API

### Design Philosophy

STEEM's vesting and reward system follows these principles:

1. **Fairness**: Proportional distribution gives fair opportunity to all participants
2. **Transparency**: All calculations are verifiable on-chain
3. **Long-term Incentives**: Vesting encourages long-term participation
4. **Flexibility**: Various reward curves reflect community needs
5. **Sustainability**: Decreasing inflation maintains long-term value

### Additional References

- [dynamic-global-properties.md](dynamic-global-properties.md) - Complete list of dynamic global properties
- [system-architecture.md](system-architecture.md) - Overall system architecture
- [reputation-system.md](reputation-system.md) - Reputation system
- [docs/development/create-operation.md](../development/create-operation.md) - How to add new operations

---

**Document Version**: 1.0
**Last Updated**: 2025-11-21
**Related Code Version**: STEEM 0.0.0
