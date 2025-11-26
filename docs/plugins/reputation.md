# reputation Plugin

Calculate and track user reputation scores based on voting activity on the Steem blockchain.

## Overview

The `reputation` plugin maintains a reputation system that rewards positive contributions and penalizes spam. It processes vote operations to calculate reputation changes based on voting power, implements anti-abuse rules to prevent gaming, and provides a social signal of account trustworthiness.

**Plugin Type**: State Tracking
**Dependencies**: [chain](chain.md)
**Memory**: Low (~10-50MB depending on active users)
**Default**: Disabled (often integrated into follow plugin)

**Note**: The reputation system is also implemented in the `follow` plugin. Most nodes should use the `follow` plugin which includes reputation tracking, rather than this standalone plugin.

## Purpose

- **Reputation Tracking**: Calculate and store reputation scores for all accounts
- **Vote-Based Scoring**: Reputation changes based on voting power (rshares)
- **Anti-Spam Protection**: Negative reputation prevents abuse
- **Social Proof**: Indicates account trustworthiness and contribution quality
- **Anti-Gaming Rules**: Prevents reputation manipulation

## Configuration

### Config File Options

```ini
# Enable reputation plugin
plugin = reputation

# No additional configuration options
```

### Command Line Options

```bash
steemd --plugin=reputation
```

### Configuration Parameters

The reputation plugin has no configurable parameters. All behavior is determined by the reputation calculation algorithm.

## Reputation System

### How Reputation Works

Reputation is calculated from vote rewards shares (rshares):

```cpp
// When a user receives a vote:
reputation_delta = vote_rshares >> 6;  // Shift away 64x precision
new_reputation = current_reputation + reputation_delta;
```

### Initial Reputation

New accounts start with reputation of `0` (zero).

### Reputation Scale

Raw reputation values are integers (can be positive or negative):

```
Very Low: -1,000,000 to -100,000 (severe spam/abuse)
Low: -100,000 to 0 (spam/low quality)
Neutral: 0 to 10,000 (new accounts)
Medium: 10,000 to 1,000,000 (active users)
High: 1,000,000 to 10,000,000 (established users)
Very High: 10,000,000+ (top contributors)
```

### Display Reputation

UIs typically convert raw reputation to 0-100 scale:

```javascript
function calculateReputation(rawRep) {
    if (rawRep == 0) return 25;

    let neg = rawRep < 0;
    let rep = Math.abs(rawRep);

    let log = Math.log10(rep);
    let out = (log - 9) * 9 + 25;

    if (neg) out = 50 - out;

    return Math.max(out, 0);
}
```

**Examples**:
```
Raw: 0           → Display: 25  (new account)
Raw: 10,000      → Display: 29
Raw: 100,000     → Display: 34
Raw: 1,000,000   → Display: 43
Raw: 10,000,000  → Display: 52
Raw: 100,000,000 → Display: 61
Raw: -100,000    → Display: 16  (negative reputation)
```

## Reputation Rules

### Rule #1: Non-Negative Reputation Required

**Users with negative reputation cannot affect others' reputation.**

```cpp
if (voter_reputation < 0) {
    // Vote does not change author's reputation
    return;
}
```

**Purpose**: Prevents spam accounts from griefing legitimate users.

**Example**:
```
Alice (rep: 1,000,000) receives upvote from Bob (rep: -10,000)
Alice's reputation: UNCHANGED (Bob has negative reputation)
```

### Rule #2: Downvotes Require Higher Reputation

**To downvote and affect reputation, voter must have higher reputation than author.**

```cpp
if (vote_rshares < 0 && voter_reputation <= author_reputation) {
    // Downvote does not change author's reputation
    return;
}
```

**Purpose**: Prevents low-reputation accounts from attacking high-reputation accounts.

**Examples**:

Downvote with higher reputation:
```
Alice (rep: 100,000) downvotes Bob (rep: 50,000)
Bob's reputation: DECREASED (Alice has higher rep)
```

Downvote with equal reputation:
```
Alice (rep: 100,000) downvotes Bob (rep: 100,000)
Bob's reputation: UNCHANGED (Alice doesn't have higher rep)
```

Downvote with lower reputation:
```
Alice (rep: 50,000) downvotes Bob (rep: 100,000)
Bob's reputation: UNCHANGED (Alice has lower rep)
```

### Rule #3: Payout Window Only

**Votes only affect reputation during the payout window (before payout occurs).**

```cpp
if (post_payout_time == fc::time_point_sec::maximum()) {
    // Post already paid out, vote doesn't affect reputation
    return;
}
```

**Purpose**: Reputation changes tied to reward distribution.

**Timeline**:
```
Post created
    ↓
0-7 days: Voting window (votes affect reputation)
    ↓
7 days: Payout occurs
    ↓
After payout: Votes don't affect reputation
```

## Database Objects

### Reputation Object

Stores reputation score for each account:

```cpp
struct reputation_object {
    account_name_type account;  // Account name
    share_type reputation;      // Reputation score (can be negative)
};
```

**Index**: By account name for fast lookups

## Vote Processing

### Pre-Vote Processing

Before applying a new vote, remove old vote's reputation effect:

```cpp
// If user already voted on this post:
if (previous_vote_exists) {
    old_reputation_delta = previous_vote_rshares >> 6;

    // Subtract old reputation change
    author_reputation -= old_reputation_delta;
}
```

### Post-Vote Processing

After applying new vote, add reputation effect:

```cpp
// Calculate new reputation delta
new_reputation_delta = new_vote_rshares >> 6;

// Check reputation rules
if (voter_reputation >= 0) {  // Rule #1
    if (new_vote_rshares >= 0 || voter_reputation > author_reputation) {  // Rule #2
        // Apply reputation change
        author_reputation += new_reputation_delta;
    }
}
```

### Complete Vote Flow

```
Vote operation submitted
        ↓
Check if post still in payout window (Rule #3)
        ↓
Get voter and author reputation
        ↓
Remove old vote reputation effect (if exists)
        ↓
Calculate new reputation delta (rshares >> 6)
        ↓
Check Rule #1 (voter reputation >= 0)
        ↓
Check Rule #2 (upvote OR voter_rep > author_rep)
        ↓
Apply reputation change
        ↓
Store updated reputation
```

## Reputation Changes Examples

### Scenario 1: New Account Receives Upvote

```
Initial state:
- Bob (new account): reputation = 0

Alice (rep: 10,000,000) upvotes Bob with 50,000 rshares

Calculation:
reputation_delta = 50,000 >> 6 = 781
new_reputation = 0 + 781 = 781

Result:
- Bob: reputation = 781
```

### Scenario 2: Downvote from Higher Reputation

```
Initial state:
- Bob: reputation = 100,000
- Alice: reputation = 1,000,000

Alice downvotes Bob with -20,000 rshares

Calculation:
reputation_delta = -20,000 >> 6 = -312
Check: Alice rep (1,000,000) > Bob rep (100,000) ✓
new_reputation = 100,000 + (-312) = 99,688

Result:
- Bob: reputation = 99,688
```

### Scenario 3: Downvote from Lower Reputation (Blocked)

```
Initial state:
- Bob: reputation = 1,000,000
- Charlie: reputation = 10,000

Charlie downvotes Bob with -20,000 rshares

Check: Charlie rep (10,000) > Bob rep (1,000,000) ✗
Rule #2 blocks reputation change

Result:
- Bob: reputation = 1,000,000 (UNCHANGED)
```

### Scenario 4: Vote from Negative Reputation (Blocked)

```
Initial state:
- Bob: reputation = 100,000
- Spammer: reputation = -50,000

Spammer upvotes Bob with 10,000 rshares

Check: Spammer rep (-50,000) >= 0 ✗
Rule #1 blocks reputation change

Result:
- Bob: reputation = 100,000 (UNCHANGED)
```

### Scenario 5: Vote After Payout (Blocked)

```
Initial state:
- Bob: reputation = 100,000
- Post already paid out

Alice upvotes Bob with 50,000 rshares

Check: Post payout_time == maximum ✓
Rule #3 blocks reputation change

Result:
- Bob: reputation = 100,000 (UNCHANGED)
```

## Performance Considerations

### Memory Usage

Minimal memory footprint:

```
reputation_object size: ~32 bytes
Active users: ~1,000,000
Total memory: 1,000,000 × 32 = 32 MB
```

### CPU Impact

Lightweight processing:
- Only triggered by vote operations
- Simple arithmetic (shift, add/subtract)
- ~5-10μs per vote
- Negligible impact on block processing

### Disk I/O

Stored in chainbase shared memory:
- Memory-mapped file
- Automatic persistence
- Read/write through cache

## Monitoring

### Reputation Queries

Query via database or condenser API:

```bash
# Get account reputation
curl -s http://localhost:8090 \
  -d '{
    "jsonrpc":"2.0",
    "id":1,
    "method":"database_api.find_accounts",
    "params":[["alice"]]
  }' | jq -r '.[0].reputation'

# Response: raw reputation value
```

### Plugin Status

Check plugin initialization:

```bash
grep reputation witness_node_data_dir/logs/stderr.txt

# Sample output:
# Initializing reputation plugin
```

### Reputation Distribution

Query multiple accounts:

```bash
# Batch query reputations
curl -s http://localhost:8090 \
  -d '{
    "jsonrpc":"2.0",
    "id":1,
    "method":"reputation_api.get_account_reputations",
    "params":{"account_lower_bound":"","limit":100}
  }'
```

## Troubleshooting

### Reputation Not Changing

**Problem**: User reputation not updating after votes

**Symptoms**:
```
Vote submitted successfully
No error returned
Reputation value unchanged
```

**Causes & Solutions**:

1. **Voter has negative reputation** (Rule #1)
   ```
   Check voter reputation >= 0
   Solution: Voter must rebuild positive reputation first
   ```

2. **Downvoter has lower/equal reputation** (Rule #2)
   ```
   Check downvoter reputation > target reputation
   Solution: Downvoter needs higher reputation to affect target
   ```

3. **Post already paid out** (Rule #3)
   ```
   Check post payout_time != fc::time_point_sec::maximum()
   Solution: Reputation only changes during payout window (0-7 days)
   ```

4. **Vote has no rshares**
   ```
   Check voter has sufficient voting power
   Solution: Wait for voting power to regenerate
   ```

### Negative Reputation

**Problem**: Account stuck with negative reputation

**Symptoms**:
```
Account reputation < 0
Cannot affect others' reputation
Labeled as spam/abuse account
```

**Recovery**:
```
1. Create quality content
2. Receive upvotes from positive-reputation users
3. Accumulate positive rshares over time
4. Reputation gradually increases back to positive
```

**Prevention**:
- Avoid spam/low-quality content
- Don't engage in vote manipulation
- Build genuine community contributions

### Reputation Calculation Differences

**Problem**: UI shows different reputation than raw value

**Symptoms**:
```
Raw reputation: 1,000,000
UI shows: 43
```

**Explanation**:
```
This is normal. UIs use logarithmic display conversion.
Raw values are the source of truth.
Display values are derived for human readability.
```

### Follow Plugin Conflicts

**Problem**: Both reputation and follow plugins enabled

**Symptoms**:
```
Database error: duplicate reputation index
Replay fails
```

**Solution**:
```ini
# Use only ONE of these:
plugin = follow        # Includes reputation
# OR
plugin = reputation    # Standalone

# NOT BOTH
```

## Use Cases

### Standalone Reputation Tracking

Track reputation without social features:

```ini
plugin = chain reputation

# Minimal memory footprint
# No feeds/follows
# Just reputation scores
```

### Integrated Social Platform

Use follow plugin instead (includes reputation):

```ini
plugin = chain follow follow_api

# Includes reputation tracking
# Plus feeds, follows, blogs
```

### API Node

Expose reputation data via API:

```ini
plugin = chain reputation reputation_api

# Or use follow_api which also provides reputation queries
plugin = chain follow follow_api
```

## Relationship to Follow Plugin

**Important**: The `follow` plugin already includes full reputation tracking. Most deployments should use `follow` instead of the standalone `reputation` plugin.

### When to Use Each

**Use `reputation` plugin**:
- Need reputation only (no social features)
- Minimal memory footprint required
- Custom reputation-based application

**Use `follow` plugin** (recommended):
- Building social platform
- Need feeds, follows, and reputation
- Standard Steem application

**Never use both together** - they conflict (duplicate indexes).

## API Integration

Requires `reputation_api` or `follow_api` for RPC access:

```ini
# Option 1: Standalone
plugin = reputation reputation_api

# Option 2: Integrated (recommended)
plugin = follow follow_api
```

**Common APIs**:
- `reputation_api.get_account_reputations` - Batch reputation query
- `follow_api.get_account_reputations` - Same (if using follow plugin)
- `database_api.find_accounts` - Includes reputation in account data

See API documentation for detailed usage.

## Best Practices

### 1. Use Follow Plugin

Unless you have specific needs, use `follow` plugin:

```ini
# Recommended
plugin = follow follow_api

# Not recommended (unless you know why)
plugin = reputation reputation_api
```

### 2. Monitor Reputation Distribution

Track community health:

```bash
# Regular queries of reputation distribution
# Identify spam accounts (negative rep)
# Reward high-reputation contributors
```

### 3. Educate Users on Rules

Help users understand:
- New accounts have limited influence
- Negative reputation prevents affecting others
- Downvotes require higher reputation
- Reputation tied to content quality

### 4. Don't Game the System

Reputation rules prevent:
- Self-voting circles
- Bot upvoting
- Revenge downvoting
- Spam account attacks

## Security Considerations

### Anti-Gaming Measures

Built-in protections:
1. Negative reputation accounts can't affect others
2. Downvotes require reputation superiority
3. Reputation tied to actual rewards (rshares)
4. No direct manipulation possible

### Sybil Resistance

Creating multiple accounts provides limited benefit:
- New accounts have zero reputation
- Zero reputation has minimal influence
- Building reputation requires time and quality content
- High-reputation users have disproportionate influence

### Consensus Impact

**Important**: Reputation is a PLUGIN feature, not consensus:
- Different nodes can have different reputation scores
- Reputation doesn't affect blockchain validity
- Purely social/UI feature
- No impact on rewards distribution

## Related Documentation

- [follow Plugin](follow.md) - Integrated social features with reputation
- [follow_api Plugin](follow_api.md) - API for reputation queries
- [Reputation System](../technical-reference/reputation-system.md) - Technical details
- [chain Plugin](chain.md) - Blockchain database

## Source Code

- **Implementation**: [libraries/plugins/reputation/reputation_plugin.cpp](../../libraries/plugins/reputation/reputation_plugin.cpp)
- **Header**: [libraries/plugins/reputation/include/steem/plugins/reputation/reputation_plugin.hpp](../../libraries/plugins/reputation/include/steem/plugins/reputation/reputation_plugin.hpp)
- **Objects**: [libraries/plugins/reputation/include/steem/plugins/reputation/reputation_objects.hpp](../../libraries/plugins/reputation/include/steem/plugins/reputation/reputation_objects.hpp)

## License

See [LICENSE.md](../../LICENSE.md)
