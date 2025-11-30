# Curation Rewards

## Overview

Curation Rewards are rewards distributed to users who vote on content in the Steem blockchain. This system provides incentives for users who discover and vote on quality content early, encouraging the community to effectively curate valuable content.

## Key Components

### 1. Reward Distribution Ratio

When content reaches the cashout time, the total rewards are distributed between the author and curators:

```
Total Rewards = Author Rewards + Curation Rewards
```

The curation reward percentage is stored in the `percent_curation_rewards` field of the `reward_fund_object`, typically set to a certain percentage of total rewards (e.g., 25%).

**Code Reference**: [database.cpp:1671](../src/core/chain/database.cpp#L1671)

```cpp
share_type curation_tokens = ( ( reward_tokens * get_curation_rewards_percent( comment ) ) / STEEM_100_PERCENT ).to_uint64();
share_type author_tokens = reward_tokens.to_uint64() - curation_tokens;
```

### 2. Curation Weight Calculation

The curation weight for each vote is determined by the following factors:

#### 2.1 Base Weight

The base weight of a vote is calculated using the reward curve:

```
W(R_N) - W(R_N-1)
```

Where:
- `W(R)` = Reward curve function
- `R_N` = Cumulative vote_rshares up to current vote
- `R_N-1` = Cumulative vote_rshares before current vote

**Code Reference**: [steem_evaluator.cpp:1304-1306](../src/core/chain/steem_evaluator.cpp#L1304-L1306)

```cpp
uint64_t old_weight = util::evaluate_reward_curve( old_vote_rshares.value, curve, reward_fund.content_constant ).to_uint64();
uint64_t new_weight = util::evaluate_reward_curve( comment.vote_rshares.value, curve, reward_fund.content_constant ).to_uint64();
cv.weight = new_weight - old_weight;
```

#### 2.2 Reverse Auction Mechanism

During the initial 30 minutes after content creation, a Reverse Auction Window is applied. Votes during this period have their weight reduced proportionally to the elapsed time:

```
Adjusted Weight = Base Weight × (Elapsed Time / 30 minutes)
```

**Constant Definition**: [config.hpp:105](../src/core/protocol/include/steem/protocol/config.hpp#L105)
```cpp
#define STEEM_REVERSE_AUCTION_WINDOW_SECONDS  (60*30) /// 30 minutes
```

**Code Reference**: [steem_evaluator.cpp:1310-1316](../src/core/chain/steem_evaluator.cpp#L1310-L1316)

```cpp
uint128_t w(max_vote_weight);
uint64_t delta_t = std::min( uint64_t((cv.last_update - comment.created).to_seconds()),
                              uint64_t(STEEM_REVERSE_AUCTION_WINDOW_SECONDS) );

w *= delta_t;
w /= STEEM_REVERSE_AUCTION_WINDOW_SECONDS;
cv.weight = w.to_uint64();
```

#### Examples

- **Vote at 0 minutes**: 0% of weight applied (all rewards return to author)
- **Vote at 15 minutes**: 50% of weight applied
- **Vote after 30 minutes**: 100% of weight applied

Curation rewards reduced by the reverse auction are returned to the author.

### 3. Reward Curves

Various reward curves can be used for curation weight calculation:

#### 3.1 Quadratic Curation Curve

```
W(R) = R² / (2α + R)
```

Where `α` (alpha) is the `content_constant`.

**Code Reference**: [reward.cpp:80-84](../src/core/chain/util/reward.cpp#L80-L84)

```cpp
case protocol::quadratic_curation:
{
   uint128_t two_alpha = content_constant * 2;
   result = uint128_t( rshares.lo, 0 ) / ( two_alpha + rshares );
}
break;
```

#### 3.2 Other Curves

- **Linear**: `W(R) = R`
- **Quadratic**: `W(R) = (R + S)² - S²` (mainly used for author rewards)
- **Square Root**: `W(R) = √R`

**Full Implementation**: [reward.cpp:68-95](../src/core/chain/util/reward.cpp#L68-L95)

### 4. Individual Curator Reward Distribution

Each curator receives rewards proportional to their weight relative to the total curation rewards:

```
Curator Reward = (Curator Weight / Total Weight) × Total Curation Rewards
```

**Code Reference**: [database.cpp:1618-1628](../src/core/chain/database.cpp#L1618-L1628)

```cpp
for( auto& item : proxy_set )
{
   uint128_t weight( item->weight );
   auto claim = ( ( max_rewards.value * weight ) / total_weight ).to_uint64();
   if( claim > 0 )
   {
      unclaimed_rewards -= claim;
      const auto& voter = get( item->voter );
      auto reward = create_vesting( voter, asset( claim, STEEM_SYMBOL ), true );

      push_virtual_operation( curation_reward_operation( voter.name, reward, c.author, to_string( c.permlink ) ) );
      // ...
   }
}
```

## Vote Processing Flow

### 1. Rshares Calculation

When voting, rshares are calculated based on the user's voting power and vesting shares:

**Code Reference**: [steem_evaluator.cpp:1190-1210](../src/core/chain/steem_evaluator.cpp#L1190-L1210)

```cpp
int64_t regenerated_power = (STEEM_100_PERCENT * elapsed_seconds) / STEEM_VOTE_REGENERATION_SECONDS;
int64_t current_power = std::min( int64_t(voter.voting_power + regenerated_power), int64_t(STEEM_100_PERCENT) );

int64_t used_power = ((current_power * abs_weight) / STEEM_100_PERCENT) * (60*60*24);
used_power = (used_power + max_vote_denom - 1) / max_vote_denom;

auto effective_vesting = _db.get_effective_vesting_shares( voter, VESTS_SYMBOL ).amount.value;
int64_t abs_rshares = ((uint128_t( effective_vesting ) * used_power) / (STEEM_100_PERCENT)).to_uint64();

abs_rshares -= STEEM_VOTE_DUST_THRESHOLD;
abs_rshares = std::max( int64_t(0), abs_rshares );
```

### 2. Comment Object Update

When a vote is processed, the `comment_object` is updated:

**Code Reference**: [steem_evaluator.cpp:1245-1254](../src/core/chain/steem_evaluator.cpp#L1245-L1254)

```cpp
_db.modify( comment, [&]( comment_object& c ){
   c.net_rshares += rshares;
   c.abs_rshares += abs_rshares;
   if( rshares > 0 )
      c.vote_rshares += rshares;
   if( rshares > 0 )
      c.net_votes++;
   else
      c.net_votes--;
});
```

### 3. Comment Vote Object Creation

Each vote is stored as a `comment_vote_object`:

**Database Object**: [comment_object.hpp:141-159](../src/core/chain/include/steem/chain/comment_object.hpp#L141-L159)

```cpp
class comment_vote_object : public object< comment_vote_object_type, comment_vote_object>
{
   id_type           id;
   account_id_type   voter;
   comment_id_type   comment;
   uint64_t          weight = 0;        // Curation weight
   int64_t           rshares = 0;       // Vote rshares
   int16_t           vote_percent = 0;  // Vote percentage (-10000 ~ 10000)
   time_point_sec    last_update;       // Vote time
   int8_t            num_changes = 0;   // Number of vote changes
};
```

## Cashout Process

### 1. Determining Cashout Time

Content is automatically paid out after a certain time. When `cashout_time` reaches the current block time, the payout is processed.

**Code Reference**: [database.cpp:1826-1836](../src/core/chain/database.cpp#L1826-L1836)

```cpp
while( current != cidx.end() && current->cashout_time <= head_block_time() )
{
   auto fund_id = get_reward_fund( *current ).id._id;
   ctx.total_reward_shares2 = funds[ fund_id ].recent_claims;
   ctx.total_reward_fund_steem = funds[ fund_id ].reward_balance;

   bool forward_curation_remainder = false;

   funds[ fund_id ].steem_awarded += cashout_comment_helper( ctx, *current, forward_curation_remainder );

   current = cidx.begin();
}
```

### 2. Reward Calculation and Distribution

The `cashout_comment_helper` function calculates total rewards and distributes them to curators and the author:

**Full Process**: [database.cpp:1652-1763](../src/core/chain/database.cpp#L1652-L1763)

1. **Calculate Total Rewards**: Calculate rewards from the reward fund based on rshares
2. **Separate Curation Rewards**: Allocate a percentage of total rewards as curation tokens
3. **Pay Curators**: Distribute to each curator using the `pay_curators` function
4. **Handle Remainder**: Unclaimed rewards optionally return to the author
5. **Pay Author**: Pay remaining amount after beneficiary allocation

### 3. Virtual Operation Generation

During payout, a `curation_reward_operation` virtual operation is generated:

**Protocol Definition**: [steem_virtual_operations.hpp:23-33](../src/core/protocol/include/steem/protocol/steem_virtual_operations.hpp#L23-L33)

```cpp
struct curation_reward_operation : public virtual_operation
{
   account_name_type curator;
   asset             reward;
   account_name_type comment_author;
   string            comment_permlink;
};
```

## Curation Eligibility Requirements

To receive curation rewards, all of the following conditions must be met:

**Code Reference**: [steem_evaluator.cpp:1294-1297](../src/core/chain/steem_evaluator.cpp#L1294-L1297)

```cpp
bool curation_reward_eligible = rshares > 0 &&
                                (comment.last_payout == fc::time_point_sec()) &&
                                comment.allow_curation_rewards;

if( curation_reward_eligible )
   curation_reward_eligible = _db.get_curation_rewards_percent( comment ) > 0;
```

1. **Positive rshares**: Only upvotes qualify (downvotes excluded)
2. **First payout**: When `last_payout` is not set (no repeat payouts)
3. **Curation allowed**: Author has set `allow_curation_rewards = true`
4. **Positive reward percentage**: Reward fund's curation percentage is greater than 0

## Special Cases

### 1. Disabling Curation

Authors can disable curation rewards through `comment_options_operation`:

**Comment Object Field**: [comment_object.hpp:106](../src/core/chain/include/steem/chain/comment_object.hpp#L106)

```cpp
bool allow_curation_rewards = true;
```

When curation is disabled, all rewards go to the author:

**Code Reference**: [database.cpp:1601-1605](../src/core/chain/database.cpp#L1601-L1605)

```cpp
if( !c.allow_curation_rewards )
{
   unclaimed_rewards = 0;
   max_rewards = 0;
}
```

### 2. Unclaimed Rewards

When vote weight is very small and rewards become 0 in satoshi units, those rewards remain as unclaimed rewards:

**Code Reference**: [database.cpp:1620-1624](../src/core/chain/database.cpp#L1620-L1624)

```cpp
auto claim = ( ( max_rewards.value * weight ) / total_weight ).to_uint64();
if( claim > 0 )
{
   unclaimed_rewards -= claim;
   // ...
}
```

Handling of unclaimed rewards is determined by the `forward_curation_remainder` flag:
- `true`: Return to author
- `false`: Remain in reward fund

### 3. Vote Changes

Users can change their vote up to 5 times:

**Constant Definition**: [config.hpp:104](../src/core/protocol/include/steem/protocol/config.hpp#L104)
```cpp
#define STEEM_MAX_VOTE_CHANGES  5
```

When votes are changed, curation weight is recalculated and previous weight is set to 0.

### 4. Post-Payout Votes

After content receives its first payout, votes are recorded but weight is 0:

**Code Reference**: [steem_evaluator.cpp:1160-1182](../src/core/chain/steem_evaluator.cpp#L1160-L1182)

```cpp
if( _db.calculate_discussion_payout_time( comment ) == fc::time_point_sec::maximum() )
{
   // After payout complete - only record vote, no reward calculation
   _db.create< comment_vote_object >( [&]( comment_vote_object& cvo )
   {
      cvo.voter = voter.id;
      cvo.comment = comment.id;
      cvo.vote_percent = o.weight;
      cvo.last_update = _db.head_block_time();
   });
   return;
}
```

## Statistics and Tracking

### Comment Object Fields

Curation-related statistics are stored in `comment_object`:

**Data Fields**: [comment_object.hpp:80-94](../src/core/chain/include/steem/chain/comment_object.hpp#L80-L94)

```cpp
share_type        net_rshares;           // Net rshares (upvotes - downvotes)
share_type        abs_rshares;           // Absolute value sum
share_type        vote_rshares;          // Positive rshares only (for curation weight calculation)
uint64_t          total_vote_weight;     // Total curation weight
asset             curator_payout_value;  // Total curation rewards paid (in SBD)
```

### Account Object Fields

Individual account curation reward statistics:

**Data Fields**: [account_object.hpp](../src/core/chain/include/steem/chain/account_object.hpp)

```cpp
share_type        curation_rewards;  // Cumulative curation rewards (when not in LOW_MEM mode)
```

## Implementation Reference

### Core Files

1. **Vote Processing**: [src/core/chain/steem_evaluator.cpp:1150-1400](../src/core/chain/steem_evaluator.cpp#L1150)
   - `vote_evaluator::do_apply()` - Vote operation processing

2. **Curator Payout**: [src/core/chain/database.cpp:1582-1643](../src/core/chain/database.cpp#L1582)
   - `database::pay_curators()` - Distribute rewards to curators

3. **Payout Processing**: [src/core/chain/database.cpp:1652-1851](../src/core/chain/database.cpp#L1652)
   - `database::cashout_comment_helper()` - Calculate and distribute total rewards
   - `database::process_comment_cashout()` - Process all content ready for payout

4. **Reward Calculation**: [src/core/chain/util/reward.cpp](../src/core/chain/util/reward.cpp)
   - `evaluate_reward_curve()` - Calculate reward curve
   - `get_rshare_reward()` - Convert rshares to rewards

5. **Protocol Definitions**:
   - [src/core/protocol/include/steem/protocol/steem_virtual_operations.hpp:23-33](../src/core/protocol/include/steem/protocol/steem_virtual_operations.hpp#L23) - `curation_reward_operation`
   - [src/core/chain/include/steem/chain/comment_object.hpp](../src/core/chain/include/steem/chain/comment_object.hpp) - Data structures

## Key Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `STEEM_REVERSE_AUCTION_WINDOW_SECONDS` | 1800 (30 minutes) | Reverse auction window period |
| `STEEM_MAX_VOTE_CHANGES` | 5 | Maximum number of vote changes |
| `STEEM_MIN_VOTE_INTERVAL_SEC` | 3 | Minimum vote interval (seconds) |
| `STEEM_VOTE_DUST_THRESHOLD` | 50000000 | Vote dust threshold |
| `STEEM_VOTE_REGENERATION_SECONDS` | 432000 (5 days) | Full voting power regeneration time |
| `STEEM_100_PERCENT` | 10000 | 100% representation (basis points) |

## Formula Summary

### 1. Total Curation Rewards

```
Curation Tokens = Total Rewards × Curation Percentage / 10000
```

### 2. Individual Curator Rewards

```
Individual Reward = Curation Tokens × Individual Weight / Total Weight
```

### 3. Reverse Auction Adjusted Weight

```
Adjusted Weight = Base Weight × min(Elapsed_Seconds, 1800) / 1800
```

### 4. Curation Weight (Quadratic Curation Curve)

```
W(R) = R² / (2α + R)
Weight = W(Current_vote_rshares) - W(Previous_vote_rshares)
```

## Example Scenarios

### Scenario 1: Normal Curation

1. Post is created (time = 0)
2. Alice votes after 30 minutes → 100% weight applied
3. Bob votes after 60 minutes → 100% weight applied
4. Payout after 7 days:
   - Total rewards: 10 STEEM
   - Curation rewards (25%): 2.5 STEEM
   - Alice weight: 600, Bob weight: 400, Total weight: 1000
   - Alice reward: 2.5 × 600/1000 = 1.5 STEEM
   - Bob reward: 2.5 × 400/1000 = 1.0 STEEM

### Scenario 2: Reverse Auction Impact

1. Post is created (time = 0)
2. Charlie votes immediately → 0% weight applied (all rewards go to author)
3. Dave votes after 15 minutes → 50% weight applied
4. Eve votes after 30 minutes → 100% weight applied
5. Payout after 7 days:
   - Charlie reward: 0 STEEM (returned to author due to reverse auction)
   - Dave reward: Proportional to reduced weight
   - Eve reward: Proportional to full weight

## Related Documentation

- [Vote Operation Evaluation](vote-operation-evaluation.md) - Vote operation evaluation process
- [System Architecture](system-architecture.md) - Overall system architecture
- [Reward Distribution](../operations/reward-distribution.md) - Reward distribution mechanism

## Notes

1. Curation rewards are always paid in **Vesting Shares (VESTS)** form.
2. The reverse auction mechanism prevents early bot voting and encourages genuine curation.
3. Weight calculation is designed to prevent overflow within 64-bit integer range.
4. Curation rewards only occur on the first payout; subsequent votes do not receive rewards.
