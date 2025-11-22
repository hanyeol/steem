# process_funds() Function Reference

## Overview

The `process_funds()` function is a core blockchain maintenance function called every block to manage the Steem inflation mechanism and distribute newly created tokens to various stakeholders. It implements the blockchain's monetary policy by calculating and distributing inflation rewards.

**Location:** [libraries/chain/database.cpp:1813](../libraries/chain/database.cpp#L1813)

## Purpose

This function handles:
1. Calculating the current inflation rate based on block height
2. Creating new STEEM tokens according to the inflation schedule
3. Distributing inflation rewards to three categories:
   - Content rewards (for authors and curators)
   - Vesting fund (for STEEM Power holders)
   - Witness rewards (for block producers)

## Inflation Model

### Initial Parameters

The Steem blockchain uses a **decreasing inflation model** that starts high and gradually reduces over time:

- **Starting inflation rate:** 9.78% per year (at block 7,000,000) - `STEEM_INFLATION_RATE_START_PERCENT = 978`
- **Ending inflation rate:** 0.95% per year (floor rate) - `STEEM_INFLATION_RATE_STOP_PERCENT = 95`
- **Reduction rate:** 0.01% per 250,000 blocks - `STEEM_INFLATION_NARROWING_PERIOD = 250000`
- **Duration:** Approximately 20.5 years
- **Completion:** Block 220,750,000

**Historical Note:** The code comment states "9.5%" but the actual implementation uses 978 basis points, which equals **9.78%**. This value was set on November 16, 2016 (commit 147d50e2) when the fixed inflation (9.5%) was changed to a decreasing inflation model, and has never been modified since.

### Inflation Rate Calculation

```cpp
int64_t start_inflation_rate = STEEM_INFLATION_RATE_START_PERCENT; // 978 (9.78%)
int64_t inflation_rate_adjustment = head_block_num() / STEEM_INFLATION_NARROWING_PERIOD;
int64_t inflation_rate_floor = STEEM_INFLATION_RATE_STOP_PERCENT; // 95 (0.95%)

int64_t current_inflation_rate = std::max(
    start_inflation_rate - inflation_rate_adjustment,
    inflation_rate_floor
);
```

The inflation rate decreases linearly:
- Every **250,000 blocks** (~29 days), the rate decreases by **0.01%**
- The rate never goes below **0.95%** (the floor)

### Per-Block Token Creation

```cpp
auto new_steem = (props.virtual_supply.amount * current_inflation_rate)
                 / (STEEM_100_PERCENT * STEEM_BLOCKS_PER_YEAR);
```

Where:
- `props.virtual_supply` = Current total virtual supply of STEEM
- `current_inflation_rate` = Current inflation percentage (basis points)
- `STEEM_BLOCKS_PER_YEAR` = 10,512,000 (365 days × 24 hours × 60 minutes × 20 blocks/minute)
- `STEEM_100_PERCENT` = 10,000 (represents 100.00%)

## Reward Distribution

The newly created STEEM is distributed according to fixed percentages:

### Distribution Breakdown

| Category | Percentage | Constant | Description |
|----------|-----------|----------|-------------|
| **Content Rewards** | 75% | `STEEM_CONTENT_REWARD_PERCENT` | For authors, curators, and comment rewards |
| **Vesting Fund** | 15% | `STEEM_VESTING_FUND_PERCENT` | For STEEM Power interest |
| **Witness Rewards** | 10% | (Remainder) | For block producers |

### Calculation Code

```cpp
// 75% to content creators and curators
auto content_reward = (new_steem * STEEM_CONTENT_REWARD_PERCENT) / STEEM_100_PERCENT;
content_reward = pay_reward_funds(content_reward);

// 15% to vesting fund (STEEM Power)
auto vesting_reward = (new_steem * STEEM_VESTING_FUND_PERCENT) / STEEM_100_PERCENT;

// Remaining 10% to witnesses
auto witness_reward = new_steem - content_reward - vesting_reward;
```

## Content Reward Distribution

The content reward is further distributed via the `pay_reward_funds()` function:

```cpp
share_type database::pay_reward_funds(share_type reward)
{
   const auto& reward_idx = get_index<reward_fund_index, by_id>();
   share_type used_rewards = 0;

   for (auto itr = reward_idx.begin(); itr != reward_idx.end(); ++itr)
   {
      // Calculate proportional reward based on fund's percentage
      auto r = (reward * itr->percent_content_rewards) / STEEM_100_PERCENT;

      // Add to the reward fund's balance
      modify(*itr, [&](reward_fund_object& rfo)
      {
         rfo.reward_balance += asset(r, STEEM_SYMBOL);
      });

      used_rewards += r;
      FC_ASSERT(used_rewards <= reward); // Sanity check
   }

   return used_rewards;
}
```

This function:
1. Iterates through all reward fund objects
2. Allocates a percentage of content rewards to each fund
3. Updates each fund's balance
4. Returns the total amount distributed

## Witness Reward Calculation

Witness rewards are **weighted** based on the witness type to ensure fair distribution between top witnesses and backup witnesses.

### Witness Types

1. **Top 20 Witnesses** - Elected by stake-weighted voting
2. **Timeshare/Backup Witnesses** - Scheduled witnesses outside top 20

### Weight-Based Calculation

```cpp
const auto& cwit = get_witness(props.current_witness);
witness_reward *= STEEM_MAX_WITNESSES; // Multiply by 21

if (cwit.schedule == witness_object::timeshare)
   witness_reward *= wso.timeshare_weight;      // Default: 5
else if (cwit.schedule == witness_object::top20)
   witness_reward *= wso.top20_weight;          // Default: 1

witness_reward /= wso.witness_pay_normalization_factor;  // Default: 25
```

### Default Weight Configuration

From [witness_objects.hpp:171-173](../libraries/chain/include/steem/chain/witness_objects.hpp#L171-L173):

```cpp
uint8_t  top20_weight = 1;
uint8_t  timeshare_weight = 5;
uint32_t witness_pay_normalization_factor = 25;
```

### Reward Calculation Breakdown

For **Top 20 witnesses**:
```
witness_reward = (base_reward * 21 * 1) / 25
               = base_reward * 0.84
```

For **Timeshare/Backup witnesses**:
```
witness_reward = (base_reward * 21 * 5) / 25
               = base_reward * 4.2
```

This means backup witnesses receive **5× more per block** than top 20 witnesses, but since they produce far fewer blocks (1 block per round vs 20 blocks per round), **top 20 witnesses still earn approximately 4× more per round overall**, providing stronger incentives for top witnesses while still maintaining participation incentives for backup witnesses.

### Important: Variable Block Inflation

Note that **the total STEEM created varies per block** due to the witness reward adjustment:

```cpp
// Line 1847: Actual creation amount recalculated
new_steem = content_reward + vesting_reward + witness_reward;
```

- **Top 20 block**: 6.69 + 1.34 + 0.7476 = **8.7776 STEEM** created
- **Timeshare block**: 6.69 + 1.34 + 3.738 = **11.768 STEEM** created

**Per-block distribution ratios:**
- Top 20 block: Content 76.2%, Vesting 15.3%, Witness 8.5%
- Timeshare block: Content 56.8%, Vesting 11.4%, Witness 31.8%

**However, averaged over a round (21 blocks):**
```
Total created = (20 × 8.7776) + (1 × 11.768) = 187.32 STEEM
Average witness reward = (14.952 + 3.738) / 187.32 = 9.98% ≈ 10%
```

**Conclusion: The long-term average maintains the 75% : 15% : 10% ratio.**

## Global State Updates

After calculating all rewards, the function updates the global blockchain state:

```cpp
modify(props, [&](dynamic_global_property_object& p)
{
   p.total_vesting_fund_steem += asset(vesting_reward, STEEM_SYMBOL);
   p.current_supply           += asset(new_steem, STEEM_SYMBOL);
   p.virtual_supply           += asset(new_steem, STEEM_SYMBOL);
});
```

### Updated Properties

1. **`total_vesting_fund_steem`** - Accumulates vesting rewards for STEEM Power interest
2. **`current_supply`** - Total circulating STEEM supply
3. **`virtual_supply`** - Virtual supply used for inflation calculations (includes SBD debt)

## Witness Vesting Creation

Finally, the witness reward is converted to STEEM Power (vesting shares) and credited to the block producer:

```cpp
const auto& producer_reward = create_vesting(
    get_account(cwit.owner),
    asset(witness_reward, STEEM_SYMBOL)
);

push_virtual_operation(
    producer_reward_operation(cwit.owner, producer_reward)
);
```

This:
1. Creates vesting shares for the witness
2. Emits a virtual operation for indexing/tracking purposes

## Example Calculation

Assuming:
- Virtual supply: 1,000,000,000 STEEM
- Block height: 10,000,000
- Current inflation: 9.68% (978 - 40 = 938 basis points)

### Step 1: Calculate per-block inflation
```
new_steem = (1,000,000,000 × 938) / (10,000 × 10,512,000)
          = 938,000,000,000 / 105,120,000,000
          ≈ 8.92 STEEM per block
```

### Step 2: Distribute rewards
```
content_reward  = 8.92 × 0.75 = 6.69 STEEM  (to reward funds)
vesting_reward  = 8.92 × 0.15 = 1.34 STEEM  (to vesting fund)
witness_reward  = 8.92 - 6.69 - 1.34 = 0.89 STEEM (to witness)
```

### Step 3: Calculate witness payout

**For top 20 witness:**
```
witness_reward = (0.89 × 21 × 1) / 25 = 0.7476 STEEM (per block)
```

**For timeshare/backup witness:**
```
witness_reward = (0.89 × 21 × 5) / 25 = 3.738 STEEM (per block)
```

**Per round rewards (21 blocks):**
- Top 20 witness (each): 20 blocks × 0.7476 = 14.952 STEEM per round
- Backup witness (each): 1 block × 3.738 = 3.738 STEEM per round
- Ratio: ~4:1 (top 20 earns 4× more per round)

### Step 4: Update global state
```
total_vesting_fund_steem += 1.34 STEEM
current_supply           += 8.92 STEEM
virtual_supply           += 8.92 STEEM
```

## Key Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `STEEM_INFLATION_RATE_START_PERCENT` | 978 | Starting inflation rate (9.78%) |
| `STEEM_INFLATION_RATE_STOP_PERCENT` | 95 | Floor inflation rate (0.95%) |
| `STEEM_INFLATION_NARROWING_PERIOD` | 250,000 | Blocks between rate decreases |
| `STEEM_CONTENT_REWARD_PERCENT` | 7,500 | 75% of inflation |
| `STEEM_VESTING_FUND_PERCENT` | 1,500 | 15% of inflation |
| `STEEM_BLOCKS_PER_YEAR` | 10,512,000 | Blocks per year (3 sec intervals) |
| `STEEM_MAX_WITNESSES` | 21 | Maximum active witnesses |
| `STEEM_100_PERCENT` | 10,000 | Represents 100.00% |

## Execution Context

This function is called:
- **When:** Every block during block processing
- **By:** `database::apply_block()` → `process_funds()`
- **Frequency:** Every 3 seconds (one block)
- **Thread safety:** Protected by database write lock

## Related Functions

- [`pay_reward_funds()`](../libraries/chain/database.cpp#L1955) - Distributes content rewards to reward funds
- [`create_vesting()`](../libraries/chain/database.cpp) - Converts STEEM to STEEM Power
- [`process_conversions()`](../libraries/chain/database.cpp#L1984) - Handles SBD conversions
- [`process_savings_withdraws()`](../libraries/chain/database.cpp#L1860) - Processes savings withdrawals

## Historical Notes

### Inflation Model Change History

1. **Initial Design (Before November 2016)**
   - Fixed inflation: 9.5% (`STEEMIT_INFLATION_RATE_PERCENT = 95 * STEEMIT_1_TENTH_PERCENT`)
   - The **102% inflation** mentioned in the function comment is from a very early version and was deprecated long ago

2. **Current Design (Since November 16, 2016 - Commit 147d50e2)**
   - Decreasing inflation model introduced
   - Start: **9.78%** (`STEEM_INFLATION_RATE_START_PERCENT = 978`)
   - End: **0.95%** (`STEEM_INFLATION_RATE_STOP_PERCENT = 95`)
   - Decreases by 0.01% every 250,000 blocks
   - **Important:** Code comment states "9.5%" but actual value is 9.78%

3. **Constant Name Changes**
   - September 2017: `STEEMIT_` → `STEEM_` prefix change (no value change)
   - September 2017: Directory structure change `steemit/` → `steem/` (no value change)

### Code Location

**File:** [libraries/protocol/include/steem/protocol/config.hpp:113-115](../libraries/protocol/include/steem/protocol/config.hpp#L113-L115)

```cpp
#define STEEM_INFLATION_RATE_START_PERCENT    (978) // Fixes block 7,000,000 to 9.5%
#define STEEM_INFLATION_RATE_STOP_PERCENT     (95) // 0.95%
#define STEEM_INFLATION_NARROWING_PERIOD      (250000) // Narrow 0.01% every 250k blocks
```

**Calculation:**
```
978 basis points / 10000 = 9.78% (not 9.5% as the comment suggests)
```

## See Also

- [Witness Reward System](./witness-rewards-bootstrap.md)
- [Chain Parameters Reference](./chain-parameters.md)
- [Vesting System](./vesting-system.md)
- [Reward Fund Documentation](./reward-funds.md)
