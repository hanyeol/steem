# Reward Pool Decay Mechanism

## Overview

The **reward pool decay mechanism** is a critical component of Steem's reward distribution system that ensures fair and sustainable reward calculations. It implements a time-based decay of voting influence to prevent historical votes from perpetually affecting current reward distributions.

**Location:** [libraries/chain/database.cpp:1720-1759](../../libraries/chain/database.cpp#L1720-L1759)

## Purpose

The reward pool decay mechanism serves several key purposes:

1. **Removes historical voting influence** - Ensures votes from the past don't indefinitely affect current rewards
2. **Increases rewards during low activity** - When fewer posts compete for rewards, individual posts receive higher payouts
3. **Maintains system fairness** - Recent activity receives appropriate weight in reward calculations
4. **Ensures sustainability** - Prevents infinite accumulation of claims on the reward pool

## Core Components

### Reward Fund Object

Each reward fund tracks the following key properties ([steem_objects.hpp:257-276](../../libraries/chain/include/steem/chain/steem_objects.hpp#L257-L276)):

```cpp
class reward_fund_object : public object<reward_fund_object_type, reward_fund_object>
{
   reward_fund_id_type     id;
   reward_fund_name_type   name;
   asset                   reward_balance = asset(0, STEEM_SYMBOL);  // Available STEEM
   fc::uint128_t           recent_claims = 0;                        // Accumulated rshares
   time_point_sec          last_update;                              // Last decay timestamp
   uint128_t               content_constant = 0;
   uint16_t                percent_curation_rewards = 0;
   uint16_t                percent_content_rewards = 0;
   protocol::curve_id      author_reward_curve;
   // ...
};
```

### Key Fields

| Field | Type | Description |
|-------|------|-------------|
| `reward_balance` | `asset` | Total STEEM tokens available for distribution |
| `recent_claims` | `uint128_t` | Accumulated reward shares (rshares) from recent posts |
| `last_update` | `time_point_sec` | Last time the decay was calculated |

## Decay Algorithm

### Mathematical Formula

The decay follows a **linear decay model** over 15 days:

```
new_recent_claims = old_recent_claims × (1 - elapsed_time / decay_period)
```

Where:
- `elapsed_time` = Current block time - Last update time
- `decay_period` = 15 days = 1,296,000 seconds

### Implementation

From [database.cpp:1728-1734](../../libraries/chain/database.cpp#L1728-L1734):

```cpp
modify(*itr, [&](reward_fund_object& rfo)
{
   fc::microseconds decay_time = STEEM_RECENT_RSHARES_DECAY_TIME;  // 15 days

   rfo.recent_claims -= (rfo.recent_claims * (head_block_time() - rfo.last_update).to_seconds())
                        / decay_time.to_seconds();
   rfo.last_update = head_block_time();
});
```

**Key characteristics:**
- **Linear decay** - Decreases proportionally to elapsed time
- **Complete decay in 15 days** - After 15 days, historical claims reach zero
- **Per-block calculation** - Updated every block (every 3 seconds)

## Operation Workflow

### Step 1: Decay Recent Claims

Every block, for each reward fund:

```cpp
const auto& reward_idx = get_index<reward_fund_index, by_id>();

for (auto itr = reward_idx.begin(); itr != reward_idx.end(); ++itr)
{
   // Decay recent_claims based on elapsed time
   modify(*itr, [&](reward_fund_object& rfo)
   {
      fc::microseconds decay_time = STEEM_RECENT_RSHARES_DECAY_TIME;

      rfo.recent_claims -= (rfo.recent_claims * (head_block_time() - rfo.last_update).to_seconds())
                           / decay_time.to_seconds();
      rfo.last_update = head_block_time();
   });

   // Cache fund state for payout calculations
   reward_fund_context rf_ctx;
   rf_ctx.recent_claims = itr->recent_claims;
   rf_ctx.reward_balance = itr->reward_balance;
   funds.push_back(rf_ctx);
}
```

### Step 2: Add New Claims from Cashout Posts

After decay, add rshares from posts reaching their cashout time:

```cpp
const auto& cidx = get_index<comment_index>().indices().get<by_cashout_time>();
auto current = cidx.begin();

// Add all rshares about to be cashed out to the reward funds
while (current != cidx.end() && current->cashout_time <= head_block_time())
{
   if (current->net_rshares > 0)
   {
      const auto& rf = get_reward_fund(*current);
      funds[rf.id._id].recent_claims += util::evaluate_reward_curve(
         current->net_rshares.value,
         rf.author_reward_curve,
         rf.content_constant
      );
   }
   ++current;
}
```

### Step 3: Calculate Individual Post Rewards

The reward for each post is calculated as:

```
author_reward = (post_rshares / recent_claims) × reward_balance
```

This creates a **proportional distribution** where:
- Posts compete for a share of the reward pool
- Higher `recent_claims` means more competition (lower individual rewards)
- Lower `recent_claims` means less competition (higher individual rewards)

## Practical Examples

### Example 1: Basic Decay Calculation

**Initial state:**
- `recent_claims` = 1,000,000,000
- `reward_balance` = 100,000 STEEM
- `last_update` = Block time at day 0

**After 3 days (259,200 seconds):**

```
decay_ratio = 259,200 / 1,296,000 = 0.2
decayed_amount = 1,000,000,000 × 0.2 = 200,000,000

new_recent_claims = 1,000,000,000 - 200,000,000 = 800,000,000
```

**After 7.5 days (648,000 seconds):**

```
decay_ratio = 648,000 / 1,296,000 = 0.5
decayed_amount = 1,000,000,000 × 0.5 = 500,000,000

new_recent_claims = 1,000,000,000 - 500,000,000 = 500,000,000
```

**After 15 days (1,296,000 seconds):**

```
decay_ratio = 1,296,000 / 1,296,000 = 1.0
decayed_amount = 1,000,000,000 × 1.0 = 1,000,000,000

new_recent_claims = 1,000,000,000 - 1,000,000,000 = 0
```

### Example 2: Impact on Rewards

**Scenario:** A post with 10,000,000 rshares competing for rewards

**Before decay (Day 0):**
```
Post rshares: 10,000,000
Recent claims: 1,000,000,000
Reward balance: 100,000 STEEM

Post reward = (10,000,000 / 1,000,000,000) × 100,000 = 1,000 STEEM
```

**After 3 days of decay:**
```
Post rshares: 10,000,000 (unchanged)
Recent claims: 800,000,000 (decayed by 20%)
Reward balance: 100,000 STEEM (constant inflow from inflation)

Post reward = (10,000,000 / 800,000,000) × 100,000 = 1,250 STEEM
```

**After 7.5 days of decay:**
```
Post rshares: 10,000,000
Recent claims: 500,000,000 (decayed by 50%)
Reward balance: 100,000 STEEM

Post reward = (10,000,000 / 500,000,000) × 100,000 = 2,000 STEEM
```

**Key insight:** As activity decreases and `recent_claims` decay, individual post rewards increase proportionally.

### Example 3: Continuous Activity vs. Decay

**Scenario:** Platform with continuous posting activity

Assume 100 posts per day, each generating 10,000,000 rshares on average.

**Daily new claims:** 100 × 10,000,000 = 1,000,000,000 rshares/day

**Equilibrium calculation:**
```
Daily decay (at equilibrium) ≈ recent_claims / 15 days
Daily new claims = 1,000,000,000

At equilibrium:
recent_claims / 15 = 1,000,000,000
recent_claims = 15,000,000,000
```

The system reaches equilibrium when **decay rate equals new claim rate**.

## Configuration Constants

From [config.hpp:149](../../libraries/protocol/include/steem/protocol/config.hpp#L149):

```cpp
#define STEEM_RECENT_RSHARES_DECAY_TIME  (fc::days(15))  // 15-day decay period
```

| Constant | Value | Description |
|----------|-------|-------------|
| `STEEM_RECENT_RSHARES_DECAY_TIME` | 15 days | Time for complete decay to zero |
| Decay rate | 1/15 per day | Linear decay rate |
| Update frequency | Every block | Decay calculated every 3 seconds |

## Why Linear Decay?

The choice of **linear decay** (as opposed to exponential decay) provides several advantages:

### 1. Predictability
- Simple to understand and calculate
- Deterministic decay schedule
- Easy to reason about reward distributions

### 2. Computational Efficiency
- Simple arithmetic operations (no exponentiation)
- Efficient per-block calculations
- Low computational overhead

### 3. Fair Distribution
- Gradual reduction in influence over time
- No sudden drops in older post influence
- Smooth transition between old and new claims

## Impact on Reward Economics

### High Activity Periods

When many posts are created:
- `recent_claims` increases rapidly
- More competition for the reward pool
- Individual post rewards decrease
- **Result:** Rewards spread across more content

### Low Activity Periods

When few posts are created:
- `recent_claims` decays faster than new claims are added
- Less competition for the reward pool
- Individual post rewards increase
- **Result:** Higher rewards per post, incentivizing content creation

### Self-Balancing Mechanism

The decay creates a **natural equilibrium**:

1. **High activity** → Lower rewards per post → Reduced incentive → Activity decreases
2. **Low activity** → Higher rewards per post → Increased incentive → Activity increases

This implements a **dynamic reward adjustment** without manual intervention.

## Interaction with Inflation

The reward pool receives constant inflation from `process_funds()`:

```cpp
// 75% of inflation goes to content rewards
auto content_reward = (new_steem * STEEM_CONTENT_REWARD_PERCENT) / STEEM_100_PERCENT;
content_reward = pay_reward_funds(content_reward);
```

**Key relationship:**
- `reward_balance` increases from inflation (every block)
- `recent_claims` decays over time (15 days)
- Ratio `reward_balance / recent_claims` determines reward per rshare

## Edge Cases

### 1. Zero Recent Claims

If `recent_claims` reaches zero (no activity for 15+ days):

```cpp
// Division by zero protection in reward calculation
if (recent_claims > 0)
{
   reward = (post_rshares / recent_claims) × reward_balance;
}
else
{
   // First post gets proportional reward based on its rshares
   reward = calculated based on reward curve and available balance;
}
```

### 2. Very Long Gaps Between Blocks

In case of blockchain downtime or restart:
- Decay calculation uses actual elapsed time
- Maximum decay is capped at 100% (recent_claims cannot go negative)
- System naturally recovers as new posts are created

### 3. Integer Overflow Protection

The code uses `uint128_t` for `recent_claims`:
- Supports extremely large values
- Prevents overflow even with high activity
- Sufficient for realistic blockchain scenarios

## Related Systems

### Reward Curve Evaluation

The `evaluate_reward_curve()` function transforms raw rshares:

```cpp
funds[rf.id._id].recent_claims += util::evaluate_reward_curve(
   current->net_rshares.value,
   rf.author_reward_curve,
   rf.content_constant
);
```

This applies a non-linear curve to:
- Reduce impact of vote spam
- Reward higher-quality content
- Prevent gaming the system

### Content Constant

From [config.hpp:150](../../libraries/protocol/include/steem/protocol/config.hpp#L150):

```cpp
#define STEEM_CONTENT_CONSTANT  (uint128_t(uint64_t(2000000000000ll)))
```

The content constant adjusts the reward curve shape, affecting how rshares translate to rewards.

## Monitoring and Debugging

### Key Metrics to Monitor

1. **`recent_claims` trend**
   - Should fluctuate based on activity
   - Sudden drops indicate low activity
   - Sudden spikes indicate high activity

2. **`reward_balance` trend**
   - Should increase steadily from inflation
   - Decreases when rewards are paid
   - Balance indicates available rewards

3. **Reward per rshare ratio**
   - `reward_balance / recent_claims`
   - Indicates reward competitiveness
   - Useful for author reward estimation

### API Queries

Check reward fund state via API:

```json
{
  "jsonrpc": "2.0",
  "method": "database_api.get_reward_fund",
  "params": {"name": "post"},
  "id": 1
}
```

Response includes:
- `recent_claims`: Current accumulated rshares
- `reward_balance`: Available STEEM for distribution
- `last_update`: Last decay calculation time

## Historical Context

The 15-day decay period was chosen to:
- Match the typical content lifecycle on social platforms
- Balance between recent and historical content influence
- Provide sufficient time for content discovery and voting
- Prevent indefinite accumulation of voting power

## Summary

The reward pool decay mechanism is a **linear time-based decay system** that:

1. **Decays `recent_claims` linearly over 15 days**
2. **Removes historical voting influence** after the decay period
3. **Increases individual rewards during low activity** periods
4. **Creates a self-balancing economic system** that adjusts to content creation rates
5. **Ensures fair and sustainable reward distribution** by preventing claim accumulation

This mechanism is fundamental to Steem's "Proof of Brain" philosophy, ensuring that **active, recent content receives appropriate rewards** while maintaining long-term economic sustainability.

## See Also

- [process_funds() Function Reference](./process-funds-function.md) - Inflation and reward distribution
- [Vesting System](./vesting-system.md) - STEEM Power mechanics
- [Vote Operation Evaluation](./vote-operation-evaluation.md) - How votes generate rshares
- [Curation Rewards](./curation-rewards.md) - Curator reward calculations
- [Chain Parameters Reference](./chain-parameters.md) - Blockchain constants
