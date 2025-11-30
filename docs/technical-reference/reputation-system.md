# Reputation System

This document explains how the reputation system works in Steem blockchain, including calculation, updates, rules, and display formatting.

## Overview

Steem uses a **reputation system** to measure user credibility and influence within the community. Reputation is:
- **Not stored on-chain** in the core blockchain state
- **Tracked by the reputation plugin** as a derived statistic
- **Calculated from voting patterns** (upvotes and downvotes)
- **Displayed as a score** (typically 0-99, though it can extend beyond)

## Reputation Storage

### Reputation Object

The reputation plugin stores reputation data separately from core chain state ([reputation_objects.hpp:23](../src/plugins/reputation/include/steem/plugins/reputation/reputation_objects.hpp#L23)):

```cpp
class reputation_object {
    id_type           id;
    account_name_type account;
    share_type        reputation;  // Raw reputation value
};
```

**Key properties:**
- `account` - Account name
- `reputation` - Raw reputation score (can be positive or negative)
- Indexed by account name for fast lookup
- Stored in memory-mapped database (chainbase)

### Why Plugin-Based?

Reputation is implemented as a **plugin** rather than core consensus:
- **Flexibility**: Rules can be changed without hardfork
- **Performance**: Doesn't affect block validation speed
- **Experimentation**: Can be disabled or replaced
- **Non-consensus**: Different nodes can use different reputation algorithms

## Reputation Calculation

### Basic Formula

Reputation is derived from **rshares** (reward shares) received from votes ([reputation_plugin.cpp:68](../src/plugins/reputation/reputation_plugin.cpp#L68)):

```cpp
// Raw reputation delta from a vote
rep_delta = cv->rshares >> 6;  // Right shift by 6 (divide by 64)
```

**Breakdown:**
1. **rshares** = Voting power translated to reward shares
2. **Division by 64** = Reduces precision (removes noise from vesting shares)
3. **Accumulation** = Sum of all rep_delta values

### Rshares Calculation

When a user votes, rshares are calculated based on ([steem_evaluator.cpp:1345](../src/core/chain/steem_evaluator.cpp#L1345)):

**Formula:**
```cpp
// Effective vesting shares (Steem Power)
vesting_shares = voter.vesting_shares + received_vesting_shares;

// Used voting power (percentage)
used_power = (current_power * vote_weight) / STEEM_100_PERCENT * (60*60*24);
used_power = used_power / (vote_power_reserve_rate * VOTE_REGENERATION_SECONDS);

// Absolute rshares
abs_rshares = (vesting_shares * used_power) / STEEM_100_PERCENT;

// Signed rshares (positive for upvote, negative for downvote)
rshares = (vote_weight < 0) ? -abs_rshares : abs_rshares;
```

**Factors affecting rshares:**
- **Vesting Shares (SP)**: More SP = more rshares
- **Voting Power**: Current voting power (regenerates over time)
- **Vote Weight**: Vote strength (-10000 to +10000, i.e., -100% to +100%)
- **Dust Threshold**: Minimum vote size (after HF20: `STEEM_VOTE_DUST_THRESHOLD`)

### Reputation Update Process

Reputation updates occur in two phases:

**Phase 1: Pre-Operation (Undo previous vote)** ([reputation_plugin.cpp:54](../src/plugins/reputation/reputation_plugin.cpp#L54))
```cpp
void pre_operation(const vote_operation& op) {
    // If revoting, remove old vote's reputation effect
    if (old_vote_exists) {
        old_rep_delta = old_vote.rshares >> 6;
        author.reputation -= old_rep_delta;
    }
}
```

**Phase 2: Post-Operation (Apply new vote)** ([reputation_plugin.cpp:112](../src/plugins/reputation/reputation_plugin.cpp#L112))
```cpp
void post_operation(const vote_operation& op) {
    // Apply new vote's reputation effect
    new_rep_delta = new_vote.rshares >> 6;
    author.reputation += new_rep_delta;
}
```

## Reputation Rules

The reputation system enforces two main rules to prevent abuse ([reputation_plugin.cpp:76](../src/plugins/reputation/reputation_plugin.cpp#L76)):

### Rule #1: Negative Reputation Cannot Affect Others

```cpp
// Rule #1: Must have non-negative reputation to affect another user's reputation
if (voter_rep < 0) return; // No effect
```

**Rationale:**
- Prevents spam accounts from damaging legitimate users
- Users with negative reputation are "quarantined"
- Must rebuild positive reputation before influencing others

### Rule #2: Downvote Requires Higher Reputation

```cpp
// Rule #2: If downvoting, voter must have more reputation than author
if (rshares < 0 && voter_rep <= author_rep) return; // No effect
```

**Rationale:**
- Prevents low-reputation users from attacking high-reputation users
- Downvotes require "reputation authority"
- Encourages building reputation before downvoting

### Upvote Rules

**Upvotes are more permissive:**
- Any non-negative reputation can upvote anyone
- Increases author's reputation proportional to voter's SP
- No reputation comparison required

## Display Reputation Score

### Raw to Display Conversion

The raw reputation value is converted to a user-friendly display score (typically 0-99):

**Common formula (used by frontends):**
```python
def reputation_to_display(raw_reputation):
    if raw_reputation == 0:
        return 25

    neg = raw_reputation < 0
    raw_reputation = abs(raw_reputation)

    # Logarithmic scale
    leading_digits = len(str(raw_reputation)) - 1
    log_value = math.log10(raw_reputation)

    # Calculate display score
    score = (log_value - leading_digits) * 9 + leading_digits

    if neg:
        score = 50 - score
    else:
        score = 50 + score

    return max(score, -9)  # Minimum display value
```

**Characteristics:**
- **Logarithmic scale**: Each order of magnitude adds ~9 points
- **Base score**: 25 for new users (raw reputation = 0)
- **Typical range**: 0-99 (though can extend beyond)
- **Negative values**: Displayed as negative (e.g., -5, -10)

### Display Score Examples

| Raw Reputation | Display Score | Description |
|----------------|---------------|-------------|
| 0 | 25 | New account, no votes received |
| 1,000 | ~30 | Slight positive reputation |
| 10,000 | ~39 | Moderate positive reputation |
| 100,000 | ~48 | Good reputation |
| 1,000,000 | ~57 | Strong reputation |
| 10,000,000 | ~66 | Very strong reputation |
| 100,000,000 | ~75 | Excellent reputation |
| 1,000,000,000 | ~84 | Outstanding reputation |
| -1,000 | ~20 | Slight negative reputation |
| -10,000 | ~11 | Moderate negative reputation |
| -100,000 | ~2 | Strong negative reputation |

## Reputation API

### Querying Reputation

**Via reputation_api** ([reputation_api.hpp:27](../src/plugins/apis/reputation_api/include/steem/plugins/reputation_api/reputation_api.hpp#L27)):

```json
// Request
{
  "jsonrpc": "2.0",
  "method": "reputation_api.get_account_reputations",
  "params": {
    "account_lower_bound": "alice",
    "limit": 100
  },
  "id": 1
}

// Response
{
  "result": {
    "reputations": [
      {
        "account": "alice",
        "reputation": "123456789"
      },
      {
        "account": "bob",
        "reputation": "987654321"
      }
    ]
  }
}
```

### API Methods

**`get_account_reputations(account_lower_bound, limit)`**
- Returns reputation data for accounts
- `account_lower_bound` - Start from this account (alphabetical)
- `limit` - Maximum number of results (default: 1000)
- Returns array of `{account, reputation}` objects

## Reputation Dynamics

### Building Reputation

**Positive actions:**
- Receive upvotes from high-SP users
- Receive upvotes from users with good reputation
- Create quality content that earns upvotes
- Engage positively with community

**Growth rate:**
- Logarithmic scale means diminishing returns
- Harder to increase at higher levels
- Requires consistent quality contribution

### Losing Reputation

**Negative actions:**
- Receive downvotes from high-SP users
- Receive downvotes from high-reputation users
- Create low-quality or spam content
- Violate community standards

**Damage limitation:**
- Low-reputation users can't damage your reputation (Rule #1)
- Users with lower reputation than you can't downvote you effectively (Rule #2)
- Protects established users from spam attacks

### Reputation Recovery

**Recovering from negative reputation:**
1. Stop behaviors causing downvotes
2. Create quality content
3. Earn upvotes from established users
4. Gradually rebuild positive reputation
5. Once positive, can influence others again (Rule #1)

## Reputation Use Cases

### Content Platforms

**Post visibility:**
- High reputation posts shown more prominently
- Low/negative reputation filtered or hidden
- Spam detection based on reputation score

**User trust:**
- Reputation as social proof
- Influences follower/subscriber decisions
- Affects curation rewards distribution

### Community Moderation

**Downvote authority:**
- High-reputation users have more moderation power
- Low-reputation users limited in downvoting (Rule #2)
- Creates hierarchy of trust

**Spam prevention:**
- Negative reputation users quarantined (Rule #1)
- Reduces effectiveness of spam attacks
- Self-regulating community moderation

### Economic Impact

**Indirect effects:**
- Reputation doesn't directly affect rewards
- High reputation may attract more votes
- Low reputation may deter voters
- Psychological and social influence

## Implementation Details

### Plugin Architecture

**Initialization** ([reputation_plugin.cpp:196](../src/plugins/reputation/reputation_plugin.cpp#L196)):
```cpp
void plugin_initialize(const variables_map& options) {
    // Register operation handlers
    _pre_apply_operation_conn = db.add_pre_apply_operation_handler(
        [&](const operation_notification& note) {
            my->pre_operation(note);
        }
    );

    _post_apply_operation_conn = db.add_post_apply_operation_handler(
        [&](const operation_notification& note) {
            my->post_operation(note);
        }
    );

    // Add reputation index to database
    add_plugin_index<reputation_index>(db);
}
```

### Operation Hooks

**Vote operation handling:**
1. `pre_operation` - Remove old vote's reputation effect
2. Vote is processed by core blockchain
3. `post_operation` - Apply new vote's reputation effect

**Visitor pattern:**
```cpp
struct post_operation_visitor {
    void operator()(const vote_operation& op) {
        // Handle vote operation
    }

    template<typename T>
    void operator()(const T&) {
        // Ignore other operations
    }
};
```

### Database Indexes

**Reputation index** ([reputation_objects.hpp:46](../src/plugins/reputation/include/steem/plugins/reputation/reputation_objects.hpp#L46)):
```cpp
typedef multi_index_container<
    reputation_object,
    indexed_by<
        ordered_unique<tag<by_id>, member<..., id>>,
        ordered_unique<tag<by_account>, member<..., account>>
    >
> reputation_index;
```

**Lookup methods:**
- By ID: `O(log n)` lookup
- By account: `O(log n)` lookup
- Persistent storage in chainbase

## Configuration

### Enabling Reputation Plugin

```ini
# config.ini
plugin = reputation
plugin = reputation_api
```

**Dependencies:**
- Requires `chain` plugin
- Optional: `reputation_api` for RPC queries

### Build Options

No special build flags required. The reputation plugin is part of the standard build.

## Edge Cases and Special Scenarios

### Initial Account Creation

**New accounts:**
- Start with raw reputation = 0
- Display score = 25
- Can receive votes from anyone
- Can upvote others (but low impact due to low SP)

### Self-Voting

**Self-upvotes:**
- Technically allowed
- Affect reputation (limited by own SP)
- Generally discouraged by community
- Some frontends hide self-votes from reputation calculation

### Vote Changes

**Changing vote:**
- Pre-operation removes old vote's reputation effect
- Post-operation applies new vote's reputation effect
- Net change = new_rep - old_rep

**Removing vote:**
- Treated as weight = 0
- Removes reputation effect
- Returns reputation to pre-vote state

### Payout Window

**After payout:**
- Votes after payout don't affect reputation (old behavior)
- After HF12: Votes recorded but reputation already calculated
- Vote changes may still update reputation in some configurations

## Security Considerations

### Sybil Attacks

**Protection mechanisms:**
- Rule #1: Negative reputation can't affect others
- Rule #2: Low reputation can't downvote high reputation
- Requires significant SP to influence reputation
- SP acquisition has economic cost

**Attack difficulty:**
- Creating multiple accounts: Limited by account creation fee
- Gaining SP: Requires capital investment or time
- Building reputation: Requires community acceptance

### Reputation Manipulation

**Challenges:**
- Circular voting detected by community
- Vote buying detectable and discouraged
- Bid-bot votes may not affect reputation proportionally
- Community downvotes can counteract manipulation

### Privacy Considerations

**Reputation visibility:**
- All votes are public on blockchain
- Reputation calculation is deterministic
- Can be recalculated from blockchain history
- No privacy for voting patterns

## Reputation vs Consensus

### Non-Consensus Nature

**Important distinction:**
- Reputation is **NOT** part of blockchain consensus
- Different nodes can have different reputation values
- Reputation doesn't affect block validity
- Plugin can be disabled without breaking consensus

**Implications:**
- Can be changed without hardfork
- Different UIs can display different reputation scores
- Not suitable for consensus-critical logic
- Used only for user experience and social signals

### Alternative Algorithms

**Possible variations:**
- Different vote weight calculations
- Alternative display formulas
- Time-decay factors
- Category-specific reputation
- Custom reputation rules

Frontends can implement their own reputation calculations while still using the blockchain's voting data.

## Monitoring and Debugging

### Logs

```bash
# Enable reputation plugin logging
log-logger = {"name":"reputation","level":"debug","appender":"stderr"}
```

### Queries

```bash
# Get reputation for specific account
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"reputation_api.get_account_reputations",
  "params":{"account_lower_bound":"alice","limit":1},
  "id":1
}' http://localhost:8080

# Check reputation plugin is loaded
curl -s http://localhost:8080 | jq '.plugins[]' | grep reputation
```

### Common Issues

**Issue: Reputation not updating**
- Check: Is reputation plugin enabled?
- Check: Are vote operations being processed?
- Check: Database indexes properly initialized?

**Issue: Unexpected reputation values**
- Check: Vote rshares calculation
- Check: Reputation rules (negative voter, insufficient reputation)
- Check: Vote weight and voting power

**Issue: Reputation display mismatch**
- Check: Frontend display formula
- Check: Raw vs display conversion
- Check: Precision and rounding

## Related Documentation

- [Plugin Development Guide](plugin.md) - Creating custom plugins
- [Block Confirmation Process](block-confirmation.md) - How votes are confirmed
- [P2P Implementation](p2p-implementation.md) - Network propagation of votes

## Summary

**Key Takeaways:**
- ✅ Reputation derived from vote rshares (SP-weighted)
- ✅ Stored by plugin, not in core consensus
- ✅ Two main rules: negative rep quarantine, downvote requires higher rep
- ✅ Logarithmic display scale (typically 25-99)
- ✅ Non-consensus: Can vary between nodes/frontends
- ✅ Social signal, not economic value
- ✅ Self-regulating community moderation system

**Reputation Formula (Simplified):**
```
Raw Reputation = Σ(vote_rshares >> 6)
Display Score = f(log₁₀(|Raw Reputation|))  // Logarithmic scale around 50
```
