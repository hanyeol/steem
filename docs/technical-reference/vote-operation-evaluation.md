# Vote Operation Evaluation

This document provides a detailed technical explanation of the `vote_evaluator::do_apply` function, which implements the core voting logic in the Steem blockchain.

## Table of Contents

- [Overview](#overview)
- [Vote Operation Structure](#vote-operation-structure)
- [Core Constants](#core-constants)
- [Evaluation Flow](#evaluation-flow)
- [Voting Power Mechanics](#voting-power-mechanics)
- [Understanding Reward Shares (rshares)](#understanding-reward-shares-rshares)
- [Reward Share Calculation](#reward-share-calculation)
- [Curation Reward Calculation](#curation-reward-calculation)
- [Vote Changes](#vote-changes)
- [Edge Cases and Special Conditions](#edge-cases-and-special-conditions)
- [Implementation Details](#implementation-details)
- [Related Code](#related-code)

## Overview

The vote evaluator processes vote operations on the Steem blockchain, handling upvotes, downvotes, and vote changes. It manages:

- Voting power consumption and regeneration
- Reward share (rshares) calculation based on stake and voting power
- Curation reward weight calculation with reverse auction mechanics
- Vote tracking and change limits
- Integration with the comment reward system

**Source Location**: [libraries/chain/steem_evaluator.cpp:1150](libraries/chain/steem_evaluator.cpp#L1150)

## Vote Operation Structure

```cpp
struct vote_operation : public base_operation
{
   account_name_type    voter;      // Account casting the vote
   account_name_type    author;     // Comment author
   string               permlink;   // Comment permlink
   int16_t              weight = 0; // Vote weight (-10000 to 10000)

   void validate() const;
   void get_required_posting_authorities(flat_set<account_name_type>& a) const
   {
      a.insert(voter);
   }
};
```

**Fields:**
- `voter`: The account casting the vote (requires posting authority)
- `author`: The author of the comment/post being voted on
- `permlink`: Permanent link identifier of the comment/post
- `weight`: Vote strength from -10000 (100% downvote) to +10000 (100% upvote), 0 to remove vote

## Core Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `STEEM_MIN_VOTE_INTERVAL_SEC` | 3 seconds | Minimum time between votes by the same account |
| `STEEM_VOTE_REGENERATION_SECONDS` | 432,000 (5 days) | Time to regenerate 100% voting power |
| `STEEM_VOTE_DUST_THRESHOLD` | 50,000,000 | Minimum rshares for vote to count (50 VESTS) |
| `STEEM_UPVOTE_LOCKOUT` | 12 hours | Time before payout when upvotes are locked |
| `STEEM_MAX_VOTE_CHANGES` | 5 | Maximum number of times a vote can be changed |
| `STEEM_REVERSE_AUCTION_WINDOW_SECONDS` | 1,800 (30 minutes) | Window for reverse auction on curation rewards |
| `STEEM_100_PERCENT` | 10,000 | 100% in basis points |

### STEEM_MIN_VOTE_INTERVAL_SEC

**Value**: `3` seconds
**Source**: [libraries/protocol/include/steem/protocol/config.hpp:106](libraries/protocol/include/steem/protocol/config.hpp#L106)

**Purpose**: Rate limiting to prevent spam and abuse.

**Implementation**:
```cpp
int64_t elapsed_seconds = (_db.head_block_time() - voter.last_vote_time).to_seconds();
FC_ASSERT(elapsed_seconds >= STEEM_MIN_VOTE_INTERVAL_SEC,
    "Can only vote once every 3 seconds.");
```

**Details**:
- Enforces a minimum 3-second interval between consecutive votes by the same account
- Applies to all vote types (new votes, vote changes, upvotes, downvotes)
- Prevents rapid-fire voting attacks and reduces blockchain spam
- The timer resets with every vote cast

**Example**:
```
Time 0:00 - Alice votes on Post A ✓
Time 0:02 - Alice tries to vote on Post B ✗ (only 2 seconds elapsed)
Time 0:03 - Alice votes on Post B ✓ (3 seconds elapsed)
```

---

### STEEM_VOTE_REGENERATION_SECONDS

**Value**: `432,000` seconds (5 days)
**Source**: [libraries/protocol/include/steem/protocol/config.hpp:103](libraries/protocol/include/steem/protocol/config.hpp#L103)

**Purpose**: Controls voting power regeneration rate.

**Implementation**:
```cpp
int64_t regenerated_power = (STEEM_100_PERCENT * elapsed_seconds) / STEEM_VOTE_REGENERATION_SECONDS;
int64_t current_power = std::min(int64_t(voter.voting_power + regenerated_power),
                                 int64_t(STEEM_100_PERCENT));
```

**Details**:
- Voting power regenerates linearly from 0% to 100% over 5 days
- Regeneration rate: **20% per day** or **0.833% per hour**
- Regeneration is continuous, calculated per second
- Capped at 100% - excess regeneration is discarded
- Designed to encourage users to spread votes over time

**Regeneration Timeline**:

| Time Elapsed | Voting Power Regenerated |
|--------------|--------------------------|
| 1 hour | 0.833% |
| 12 hours | 10% |
| 1 day | 20% |
| 2.5 days | 50% |
| 5 days | 100% |

**Practical Example**:
```
Day 0, 00:00 - User has 100% voting power
Day 0, 12:00 - User makes 10 full-power votes, power drops to ~80%
Day 1, 12:00 - Power regenerates to ~100% (80% + 20%)
```

---

### STEEM_VOTE_DUST_THRESHOLD

**Value**: `50,000,000` raw units (50 VESTS, since VESTS has 6 decimal places)
**Source**: [libraries/protocol/include/steem/protocol/config.hpp:107](libraries/protocol/include/steem/protocol/config.hpp#L107)

**Purpose**: Filters out economically insignificant votes.

**Implementation**:
```cpp
int64_t abs_rshares = ((uint128_t(effective_vesting) * used_power) / STEEM_100_PERCENT).to_uint64();
abs_rshares -= STEEM_VOTE_DUST_THRESHOLD;
abs_rshares = std::max(int64_t(0), abs_rshares);

FC_ASSERT(abs_rshares > 0, "Cannot vote with 0 rshares.");
```

**Details**:
- Subtracted from every vote's calculated rshares
- If resulting rshares ≤ 0, the vote is rejected
- Prevents blockchain bloat from micro-votes with negligible economic value
- Applied **after** calculating rshares from vesting shares and voting power
- Same threshold applies to both upvotes and downvotes

**Minimum Stake Required**:

To cast a valid vote with 100% vote weight (~2% voting power consumption), users need approximately **250,000,000 VESTS** of effective vesting shares.

**Example Calculation**:
```
User has 1,000,000 VESTS effective vesting
100% vote weight, used_power = 2,000

abs_rshares = (1,000,000 * 2,000) / 10,000 = 200,000
abs_rshares -= 50,000,000 = -49,800,000
abs_rshares = max(0, -49,800,000) = 0

Vote REJECTED: "Cannot vote with 0 rshares."
```

**Why This Matters**:
- Protects against spam from accounts with negligible stake
- Reduces storage requirements for the blockchain
- Encourages users to power up (stake STEEM) to participate in voting

---

### STEEM_UPVOTE_LOCKOUT

**Value**: `12 hours` (43,200 seconds)
**Source**: [libraries/protocol/include/steem/protocol/config.hpp:25](libraries/protocol/include/steem/protocol/config.hpp#L25)

**Purpose**: Prevents last-minute vote manipulation before payout.

**Implementation**:
```cpp
if (rshares > 0)  // Only for upvotes or increased upvotes
{
    FC_ASSERT(_db.head_block_time() < comment.cashout_time - STEEM_UPVOTE_LOCKOUT,
        "Cannot increase payout within last twelve hours before payout.");
}
```

**Details**:
- Applies only to upvotes and vote changes that **increase** payout
- Downvotes are **always allowed**, even in the lockout period
- Triggered when:
  - Creating a new upvote (rshares > 0)
  - Changing an existing vote to have higher rshares
- NOT triggered when:
  - Downvoting
  - Reducing an upvote
  - Removing a vote (changing to weight 0)

**Timeline Example**:
```
Post created: Day 0, 00:00
Cashout time: Day 7, 00:00
Lockout starts: Day 6, 12:00 (12 hours before payout)

Day 6, 11:59 - Can upvote ✓
Day 6, 12:00 - Upvotes locked ✗
Day 6, 12:01 - Can still downvote ✓
Day 7, 00:00 - Post pays out, voting disabled
```

**Rationale**:
- Prevents coordinated upvote attacks just before payout
- Gives community time to respond with downvotes if needed
- Reduces effectiveness of vote-buying schemes
- **Asymmetric design**: Upvotes locked, downvotes allowed for community protection

---

### STEEM_MAX_VOTE_CHANGES

**Value**: `5`
**Source**: [libraries/protocol/include/steem/protocol/config.hpp:104](libraries/protocol/include/steem/protocol/config.hpp#L104)

**Purpose**: Limits vote manipulation and curation gaming.

**Implementation**:
```cpp
FC_ASSERT(itr->num_changes < STEEM_MAX_VOTE_CHANGES,
    "Voter has used the maximum number of vote changes on this comment.");
```

**Details**:
- Each account can change their vote on a specific comment up to 5 times
- Initial vote does **not** count toward the limit
- Only subsequent changes increment `num_changes`
- Applies per comment, not globally
- After 5 changes, the vote is locked until post payout

**Vote Change Tracking**:
```
Initial vote:    num_changes = 0 (doesn't count)
1st change:      num_changes = 1
2nd change:      num_changes = 2
3rd change:      num_changes = 3
4th change:      num_changes = 4
5th change:      num_changes = 5
6th change:      REJECTED ✗ "Voter has used the maximum number of vote changes"
```

**Critical Penalty**:
```cpp
// When changing a vote
cv.weight = 0;  // No curation reward for changed votes
cv.num_changes += 1;
```

Changed votes receive **zero curation rewards**. This is the primary anti-gaming mechanism.

**Rationale**:
- Prevents users from constantly adjusting votes to game curation rewards
- Allows genuine mistakes to be corrected (5 changes is generous)
- Balances flexibility with anti-abuse protection

---

### STEEM_REVERSE_AUCTION_WINDOW_SECONDS

**Value**: `1,800` seconds (30 minutes)
**Source**: [libraries/protocol/include/steem/protocol/config.hpp:105](libraries/protocol/include/steem/protocol/config.hpp#L105)

**Purpose**: Discourages bot voting and rewards manual curation.

**Implementation**:
```cpp
uint128_t w(max_vote_weight);
uint64_t delta_t = std::min(
    uint64_t((cv.last_update - comment.created).to_seconds()),
    uint64_t(STEEM_REVERSE_AUCTION_WINDOW_SECONDS)
);

w *= delta_t;
w /= STEEM_REVERSE_AUCTION_WINDOW_SECONDS;
cv.weight = w.to_uint64();
```

**Details**:
- Curation reward weight scales linearly with vote timing
- 0% of curation weight at post creation (t=0)
- 100% of curation weight at 30 minutes after creation
- Penalty only applies to the curation portion, not author rewards
- Lost curation rewards return to the **author**, not other curators

**Curation Weight by Voting Time**:

| Time After Post Creation | Curation Weight | Goes to Author |
|--------------------------|-----------------|----------------|
| 0 seconds | 0% | 100% |
| 5 minutes | 16.7% | 83.3% |
| 10 minutes | 33.3% | 66.7% |
| 15 minutes | 50% | 50% |
| 20 minutes | 66.7% | 33.3% |
| 25 minutes | 83.3% | 16.7% |
| 30+ minutes | 100% | 0% |

**Formula**:
```
actual_curation_weight = max_weight × min(time_elapsed, 1800) / 1800
penalty_to_author = max_weight - actual_curation_weight
```

**Why This Exists**:
1. **Anti-Bot Mechanism**: Automated voting bots that vote immediately receive reduced rewards
2. **Rewards Quality Curation**: Encourages users to read content before voting
3. **Benefits Authors**: Authors get extra rewards from early votes
4. **Encourages Manual Discovery**: First manual curators can still earn if they vote around 30 minutes

**Strategic Voting**:
- **Vote immediately (0 min)**: Good for authors you strongly support (they get all curation rewards)
- **Vote at 15 min**: Balanced - 50% curation reward
- **Vote at 30+ min**: Maximum curation reward, but competing with more voters

**Real-World Example**:
```
Post published: 10:00 AM
Bot votes at 10:00 AM: Gets 0% curation weight
Human reads and votes at 10:15 AM: Gets 50% curation weight
Voter at 10:30 AM: Gets 100% curation weight, but smaller share due to competition
```

**Trade-off**:
- Vote early → Help author more, lower personal curation reward
- Vote at 30+ min → Maximum personal curation, but more competition for the reward pool

---

### STEEM_100_PERCENT

**Value**: `10,000` (basis points)
**Source**: [libraries/protocol/include/steem/protocol/config.hpp:116](libraries/protocol/include/steem/protocol/config.hpp#L116)

**Purpose**: Standard percentage representation across the protocol.

**Usage Throughout Codebase**:
```cpp
// Vote weight validation
FC_ASSERT(abs(weight) <= STEEM_100_PERCENT, "Weight is not a STEEMIT percentage");

// Voting power calculation
int64_t current_power = std::min(int64_t(voter.voting_power + regenerated_power),
                                 int64_t(STEEM_100_PERCENT));

// Rshares calculation
int64_t abs_rshares = ((uint128_t(effective_vesting) * used_power) / STEEM_100_PERCENT).to_uint64();
```

**Details**:
- Represents 100% as 10,000 units (basis points)
- 1% = 100 units
- 0.01% = 1 unit (finest granularity)
- Used consistently for:
  - Vote weights (-10000 to +10000)
  - Voting power (0 to 10000)
  - All percentage calculations throughout the protocol

**Why Basis Points Instead of Floating Point**:
1. **Determinism**: Floating-point arithmetic can yield different results across platforms
2. **Precision**: Integer math is exact and reproducible
3. **Consensus**: All nodes must calculate identical results
4. **Performance**: Integer operations are faster than floating-point

**Common Conversions**:

| Percentage | Basis Points |
|------------|--------------|
| 100% | 10,000 |
| 50% | 5,000 |
| 10% | 1,000 |
| 1% | 100 |
| 0.1% | 10 |
| 0.01% | 1 |

**Example Calculation**:
```cpp
// User votes with 50% weight
vote_weight = 5000;  // 50% of STEEM_100_PERCENT

// Current voting power is 80%
voting_power = 8000;  // 80% of STEEM_100_PERCENT

// Calculate used power proportion
used_power = (voting_power * abs(vote_weight)) / STEEM_100_PERCENT;
// = (8000 * 5000) / 10000 = 4000
```

## Evaluation Flow

The vote evaluator follows this execution flow:

```
1. Retrieve comment and voter account objects from database
2. Check voter eligibility (has not declined voting rights)
3. Check if votes are allowed on the comment (for upvotes)
4. Handle expired posts (payout time = maximum)
   └─> Record vote metadata only, skip all reward calculations
5. Validate vote timing (3-second minimum interval)
6. Calculate voting power regeneration and current power
7. Calculate voting power consumption based on vote weight
8. Calculate effective vesting shares and absolute rshares
9. Branch: New vote vs. vote change
   ├─> New vote: Create comment_vote_object, update comment rshares
   └─> Vote change: Modify existing vote, adjust comment rshares
10. Calculate curation reward weight (if eligible)
11. Update voter's voting power and last vote time
12. Update comment statistics (net_votes, net_rshares, etc.)
```

## Voting Power Mechanics

### Voting Power Representation

Voting power ranges from 0 to 10,000 (0% to 100%) and regenerates linearly over time.

### Regeneration Formula

```cpp
int64_t elapsed_seconds = (current_time - last_vote_time).to_seconds();
int64_t regenerated_power = (STEEM_100_PERCENT * elapsed_seconds) / STEEM_VOTE_REGENERATION_SECONDS;
int64_t current_power = min(voter.voting_power + regenerated_power, STEEM_100_PERCENT);
```

**Example:**
- If 86,400 seconds (1 day) elapsed since last vote:
  - `regenerated_power = (10,000 * 86,400) / 432,000 = 2,000` (20%)
  - Voting power regenerates 20% per day

### Voting Power Consumption

```cpp
int64_t abs_weight = abs(vote_weight);
int64_t used_power = ((current_power * abs_weight) / STEEM_100_PERCENT) * (60*60*24);
int64_t max_vote_denom = vote_power_reserve_rate * STEEM_VOTE_REGENERATION_SECONDS;
used_power = (used_power + max_vote_denom - 1) / max_vote_denom;
```

**Key Points:**
- A 100% vote weight consumes approximately 2% of voting power
- The `vote_power_reserve_rate` (dynamic global property) adjusts consumption
- `(60*60*24)` factor (86,400) ensures backward compatibility with legacy implementation
- Division is rounded up to prevent zero-power votes

**Example with default vote_power_reserve_rate = 50:**
- 100% weight vote: ~2% voting power consumed
- 50% weight vote: ~1% voting power consumed
- 10% weight vote: ~0.2% voting power consumed

## Understanding Reward Shares (rshares)

### What are Reward Shares?

**Reward Shares (rshares)** are the fundamental unit of voting influence in the Steem blockchain. They represent the amount of influence a vote has on a post's rewards, calculated from the voter's stake (VESTS) and voting power consumption.

### Core Concepts

#### rshares (Reward Shares)
- **Definition**: The signed voting influence on a post
- **Range**: Can be positive (upvote) or negative (downvote)
- **Purpose**: Determines the net reward value of a post
- **Calculation**: Based on voter's effective vesting shares and voting power used

```cpp
int64_t rshares = vote_weight < 0 ? -abs_rshares : abs_rshares;
```

**Sign Convention**:
- **Positive rshares**: Upvote (increases post rewards)
- **Negative rshares**: Downvote (decreases post rewards)
- **Zero rshares**: Vote removed or negligible stake

#### abs_rshares (Absolute Reward Shares)
- **Definition**: The absolute magnitude of voting influence, always positive
- **Purpose**: Tracks total voting activity regardless of direction
- **Usage**: Used for calculating curation rewards and voting activity metrics

```cpp
int64_t abs_rshares = ((uint128_t(effective_vesting) * used_power) / STEEM_100_PERCENT).to_uint64();
abs_rshares -= STEEM_VOTE_DUST_THRESHOLD;
abs_rshares = std::max(int64_t(0), abs_rshares);
```

### Key Differences

| Aspect | rshares | abs_rshares |
|--------|---------|-------------|
| **Sign** | Can be positive or negative | Always positive (absolute value) |
| **Purpose** | Determines post payout direction | Measures voting activity magnitude |
| **Usage** | `net_rshares`, `vote_rshares` | `abs_rshares`, curation calculations |
| **Accumulation** | Can cancel out (up vs down) | Always accumulates |

### Comment Object Fields

The `comment_object` tracks multiple rshares-related fields:

#### net_rshares
```cpp
comment.net_rshares += rshares;  // Can be positive or negative
```

- **Definition**: Net reward shares determining actual payout
- **Calculation**: Sum of all upvote rshares minus all downvote rshares
- **Purpose**: Final value determining post rewards
- **Can be negative**: Yes (heavily downvoted posts have negative net_rshares)

**Example**:
```
Initial: net_rshares = 0
Alice upvotes (+1,000,000 rshares): net_rshares = 1,000,000
Bob upvotes (+500,000 rshares): net_rshares = 1,500,000
Charlie downvotes (-300,000 rshares): net_rshares = 1,200,000
```

#### abs_rshares
```cpp
comment.abs_rshares += abs_rshares;  // Always positive addition
```

- **Definition**: Total absolute voting activity on the post
- **Calculation**: Sum of absolute values of all votes
- **Purpose**: Measures total community engagement
- **Can be negative**: No (always accumulates positively)

**Example** (same votes as above):
```
Initial: abs_rshares = 0
Alice upvotes (1,000,000): abs_rshares = 1,000,000
Bob upvotes (500,000): abs_rshares = 1,500,000
Charlie downvotes (300,000 absolute): abs_rshares = 1,800,000
```

#### vote_rshares
```cpp
if (rshares > 0)
    comment.vote_rshares += rshares;  // Only upvotes
```

- **Definition**: Cumulative upvote rshares for curation reward calculation
- **Calculation**: Sum of only positive (upvote) rshares
- **Purpose**: Determines curation reward distribution
- **Excludes**: Downvotes (only counts upvotes)

**Example** (same votes as above):
```
Initial: vote_rshares = 0
Alice upvotes (+1,000,000): vote_rshares = 1,000,000
Bob upvotes (+500,000): vote_rshares = 1,500,000
Charlie downvotes: vote_rshares = 1,500,000 (unchanged, downvotes don't count)
```

#### children_abs_rshares
```cpp
root_comment.children_abs_rshares += abs_rshares;  // For root post
```

- **Definition**: Total absolute voting activity on all replies
- **Calculation**: Sum of abs_rshares from all child comments
- **Purpose**: Tracks engagement on the entire discussion thread
- **Applies to**: Root post only (accumulated from all replies)

### Practical Examples

#### Example 1: Pure Upvoting Scenario
```
Post receives 3 upvotes:
- Vote 1: +1,000,000 rshares
- Vote 2: +750,000 rshares
- Vote 3: +250,000 rshares

Results:
net_rshares = 2,000,000 (sum of all positive)
abs_rshares = 2,000,000 (sum of absolutes)
vote_rshares = 2,000,000 (sum of upvotes only)
```

#### Example 2: Mixed Voting Scenario
```
Post receives upvotes and downvotes:
- Vote 1: +2,000,000 rshares (upvote)
- Vote 2: +1,000,000 rshares (upvote)
- Vote 3: -800,000 rshares (downvote)
- Vote 4: +500,000 rshares (upvote)

Results:
net_rshares = 2,700,000 (2M + 1M - 800K + 500K)
abs_rshares = 4,300,000 (2M + 1M + 800K + 500K, all positive)
vote_rshares = 3,500,000 (2M + 1M + 500K, upvotes only)
```

#### Example 3: Heavily Downvoted Post
```
Post receives more downvotes than upvotes:
- Vote 1: +500,000 rshares (upvote)
- Vote 2: -1,000,000 rshares (downvote)
- Vote 3: -800,000 rshares (downvote)

Results:
net_rshares = -1,300,000 (500K - 1M - 800K, NEGATIVE)
abs_rshares = 2,300,000 (500K + 1M + 800K)
vote_rshares = 500,000 (only the upvote counts)

Payout: 0 STEEM (negative net_rshares → no rewards)
```

### Reward Calculation Impact

The different rshares fields serve distinct purposes in reward distribution:

1. **Post Author Rewards**: Based on `net_rshares` after applying reward curve
   ```cpp
   fc::uint128_t reward_rshares = std::max(comment.net_rshares.value, int64_t(0));
   reward_rshares = util::evaluate_reward_curve(reward_rshares);
   ```

2. **Curation Rewards**: Based on `vote_rshares` (upvotes only)
   ```cpp
   uint64_t weight = util::evaluate_reward_curve(
       comment.vote_rshares.value, curve, content_constant);
   ```

3. **Activity Metrics**: Based on `abs_rshares`
   - Used for trending algorithms
   - Measures total community engagement
   - Includes both upvotes and downvotes

### Why Multiple rshares Fields?

The separation of rshares into multiple fields serves important purposes:

1. **net_rshares**: Determines actual payouts (can go negative)
2. **abs_rshares**: Measures total activity (always positive)
3. **vote_rshares**: Calculates curation rewards (upvotes only)
4. **children_abs_rshares**: Tracks discussion engagement

This design allows:
- **Fair downvoting**: Downvotes reduce rewards without removing curation history
- **Activity tracking**: Total engagement visible even with mixed votes
- **Curation protection**: Curators rewarded based on upvotes, not affected by later downvotes
- **Thread metrics**: Root posts track engagement of entire discussions

## Reward Share Calculation

### Absolute Reward Shares (rshares)

```cpp
auto effective_vesting = db.get_effective_vesting_shares(voter, VESTS_SYMBOL).amount.value;
int64_t abs_rshares = ((uint128_t(effective_vesting) * used_power) / STEEM_100_PERCENT).to_uint64();
abs_rshares -= STEEM_VOTE_DUST_THRESHOLD;
abs_rshares = max(0, abs_rshares);
```

**Components:**
1. **Effective Vesting Shares**: VESTS owned + delegated VESTS - delegated out VESTS
2. **Used Power**: Actual voting power consumed (from previous calculation)
3. **Dust Threshold Subtraction**: Removes 0.05 VESTS worth to filter out dust votes
4. **Sign Application**: Upvote = positive rshares, downvote = negative rshares

**Example:**
- Voter has 1,000,000 VESTS effective vesting
- 100% vote weight, consuming 2,000 (20% of 10,000) used_power
- `abs_rshares = (1,000,000 * 2,000) / 10,000 - 50,000,000 = 200,000,000 - 50,000,000 = 150,000,000`
- If upvote: `rshares = +150,000,000`
- If downvote (weight = -10000): `rshares = -150,000,000`

### Impact on Comment

The rshares affect the comment's reward calculation:

```cpp
comment.net_rshares += rshares;        // Net reward shares (upvotes - downvotes)
comment.abs_rshares += abs_rshares;    // Total absolute voting activity
comment.vote_rshares += rshares;       // For upvotes only
comment.net_votes += (rshares > 0) ? 1 : -1;  // Vote count
```

**Fields:**
- `net_rshares`: Net reward shares determining payout
- `abs_rshares`: Total voting activity (both up and down)
- `vote_rshares`: Sum of upvote rshares (for curation calculation)
- `net_votes`: Net vote count displayed to users

## Curation Reward Calculation

Curation rewards are only calculated for upvotes on posts that haven't paid out yet.

### Eligibility Check

```cpp
bool curation_reward_eligible = rshares > 0
    && (comment.last_payout == fc::time_point_sec())
    && comment.allow_curation_rewards;

if (curation_reward_eligible)
    curation_reward_eligible = db.get_curation_rewards_percent(comment) > 0;
```

### Weight Calculation

The weight determines the curator's share of curation rewards:

```cpp
// Calculate weight based on reward curve
const auto& reward_fund = db.get_reward_fund(comment);
auto curve = reward_fund.curation_reward_curve;
uint64_t old_weight = util::evaluate_reward_curve(
    old_vote_rshares.value, curve, reward_fund.content_constant).to_uint64();
uint64_t new_weight = util::evaluate_reward_curve(
    comment.vote_rshares.value, curve, reward_fund.content_constant).to_uint64();
cv.weight = new_weight - old_weight;
```

**The weight represents the marginal contribution of this vote to the total curation reward pool.**

### Reverse Auction Mechanism

To prevent voting bots from immediately voting for maximum curation rewards:

```cpp
uint128_t w(max_vote_weight);
uint64_t delta_t = min(
    (cv.last_update - comment.created).to_seconds(),
    STEEM_REVERSE_AUCTION_WINDOW_SECONDS
);

w *= delta_t;
w /= STEEM_REVERSE_AUCTION_WINDOW_SECONDS;
cv.weight = w.to_uint64();
```

**How it works:**
- Votes within first 30 minutes have reduced curation weight
- Weight scales linearly from 0% (at creation) to 100% (at 30 minutes)
- Rewards lost to reverse auction return to the author

**Example:**
- Vote at 0 minutes: 0% curation weight (all to author)
- Vote at 15 minutes: 50% curation weight
- Vote at 30+ minutes: 100% curation weight

## Vote Changes

Users can change their vote up to `STEEM_MAX_VOTE_CHANGES` (5) times.

### Vote Change Processing

When modifying an existing vote:

```cpp
// Revert previous vote's effect
comment.net_rshares -= itr->rshares;
comment.abs_rshares += abs_rshares;

// Apply new vote
comment.net_rshares += rshares;

// Update net_votes based on direction change
if (rshares > 0 && itr->rshares < 0)
    comment.net_votes += 2;  // Downvote → Upvote
else if (rshares > 0 && itr->rshares == 0)
    comment.net_votes += 1;  // Neutral → Upvote
else if (rshares == 0 && itr->rshares > 0)
    comment.net_votes -= 1;  // Upvote → Neutral
// ... (more cases)
```

### Vote Change Restrictions

```cpp
FC_ASSERT(itr->num_changes < STEEM_MAX_VOTE_CHANGES,
    "Voter has used the maximum number of vote changes on this comment.");

FC_ASSERT(itr->vote_percent != o.weight,
    "You have already voted in a similar way.");
```

### Curation Weight on Vote Change

When changing a vote, the old curation weight is removed and **no new weight is assigned**:

```cpp
comment.total_vote_weight -= itr->weight;  // Remove old weight

// Update vote object
cv.weight = 0;  // No curation reward for changed votes
cv.num_changes += 1;
```

**This prevents gaming curation rewards by repeatedly changing votes.**

## Edge Cases and Special Conditions

### 1. Expired Posts

When payout time has passed (set to maximum):

```cpp
if (db.calculate_discussion_payout_time(comment) == fc::time_point_sec::maximum())
{
#ifndef CLEAR_VOTES
    // Record vote for tracking but skip all reward calculations
    if (vote exists)
        update vote_percent and last_update
    else
        create minimal comment_vote_object
#endif
    return;  // Early exit
}
```

### 2. Post-Payout Vote Restriction

```cpp
if (itr != comment_vote_idx.end() && itr->num_changes == -1)
{
    FC_ASSERT(false, "Cannot vote again on a comment after payout.");
}
```

**The `num_changes = -1` marks votes that can no longer be changed after payout.**

### 3. Upvote Lockout

```cpp
if (rshares > 0)
{
    FC_ASSERT(db.head_block_time() < comment.cashout_time - STEEM_UPVOTE_LOCKOUT,
        "Cannot increase payout within last twelve hours before payout.");
}
```

**Prevents upvotes in the final 12 hours before payout to reduce last-minute manipulation.**

**Note:** This only applies when:
- Adding a new upvote
- Changing to a stronger upvote (increasing rshares)
- Downvotes are not restricted

### 4. Zero-Weight Vote

```cpp
FC_ASSERT(o.weight != 0, "Vote weight cannot be 0.");
```

**Only applies to new votes. Cannot create a vote with 0 weight, but can change existing vote to 0 to remove it.**

### 5. Insufficient Power

```cpp
FC_ASSERT(used_power <= current_power,
    "Account does not have enough power to vote.");
```

### 6. Minimum Rshares

```cpp
FC_ASSERT(abs_rshares > 0, "Cannot vote with 0 rshares.");
```

**After dust threshold subtraction, vote must have meaningful rshares.**

### 7. Voter Eligibility

```cpp
FC_ASSERT(voter.can_vote, "Voter has declined their voting rights.");
```

### 8. Comment Allows Votes

```cpp
if (o.weight > 0)
    FC_ASSERT(comment.allow_votes, "Votes are not allowed on the comment.");
```

**Only checked for upvotes (weight > 0). Downvotes are always allowed even if comment disabled upvotes.**

## Implementation Details

### Database Modifications

The evaluator makes several database modifications within a transaction:

1. **Voter Account Update**
   ```cpp
   db.modify(voter, [&](account_object& a) {
       a.voting_power = current_power - used_power;
       a.last_vote_time = db.head_block_time();
   });
   ```

2. **Comment Update**
   ```cpp
   db.modify(comment, [&](comment_object& c) {
       c.net_rshares += rshares;
       c.abs_rshares += abs_rshares;
       if (rshares > 0) {
           c.vote_rshares += rshares;
           c.net_votes++;
       } else {
           c.net_votes--;
       }
   });
   ```

3. **Root Comment Update** (for comments on posts)
   ```cpp
   db.modify(root, [&](comment_object& c) {
       c.children_abs_rshares += abs_rshares;
   });
   ```

4. **Comment Vote Object** (new vote)
   ```cpp
   db.create<comment_vote_object>([&](comment_vote_object& cv) {
       cv.voter = voter.id;
       cv.comment = comment.id;
       cv.rshares = rshares;
       cv.vote_percent = o.weight;
       cv.last_update = db.head_block_time();
       cv.weight = calculated_curation_weight;
   });
   ```

5. **Curation Weight Update**
   ```cpp
   db.modify(comment, [&](comment_object& c) {
       c.total_vote_weight += max_vote_weight;
   });
   ```

### Backward Compatibility Note

```cpp
// Less rounding error would occur if we did the division last, but we need
// to maintain backward compatibility with the previous implementation which
// was replaced in #1285
int64_t used_power = ((current_power * abs_weight) / STEEM_100_PERCENT) * (60*60*24);
```

**The `* (60*60*24)` multiplication is done before final division to match historical calculations.**

### Reward Curve Evaluation

The reward curve (currently linear on Steem after HF19) is evaluated to determine:
- Comment payout based on `net_rshares`
- Curation weight based on `vote_rshares`

```cpp
new_rshares = util::evaluate_reward_curve(new_rshares);
old_rshares = util::evaluate_reward_curve(old_rshares);
```

### CLEAR_VOTES Compile Option

When `CLEAR_VOTES` is defined, vote objects for expired posts are not created/stored to save memory:

```cpp
#ifndef CLEAR_VOTES
    // Store vote metadata
#endif
```

**This is typically enabled on witness/seed nodes to reduce memory usage.**

## Related Code

### Key Files

- **[libraries/chain/steem_evaluator.cpp](libraries/chain/steem_evaluator.cpp)** - Vote evaluator implementation
- **[libraries/protocol/include/steem/protocol/steem_operations.hpp](libraries/protocol/include/steem/protocol/steem_operations.hpp)** - Vote operation structure
- **[libraries/protocol/include/steem/protocol/config.hpp](libraries/protocol/include/steem/protocol/config.hpp)** - Core constants
- **[libraries/chain/include/steem/chain/comment_object.hpp](libraries/chain/include/steem/chain/comment_object.hpp)** - Comment object definition
- **[libraries/chain/util/reward.cpp](libraries/chain/util/reward.cpp)** - Reward curve evaluation

### Database Objects

- `account_object` - Voter account with voting_power and last_vote_time
- `comment_object` - Comment/post with rshares, vote counts, and payout info
- `comment_vote_object` - Individual vote record with rshares and curation weight
- `dynamic_global_property_object` - Global parameters like vote_power_reserve_rate

### Related Operations

- `comment_operation` - Creates posts and comments that can be voted on
- `vote_operation` - Covered in this document
- `comment_options_operation` - Can disable voting on comments
- `decline_voting_rights_operation` - Can disable account's ability to vote

## See Also

- [Reputation System](reputation-system.md) - How votes affect reputation
- [Chain Parameters](chain-parameters.md) - Global blockchain parameters
- [System Architecture](system-architecture.md) - Overall system design
