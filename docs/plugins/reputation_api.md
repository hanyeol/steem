# reputation_api Plugin

Query account reputation scores on the Steem blockchain.

## Overview

The `reputation_api` plugin provides read-only access to account reputation data. Reputation is a measure of an account's standing in the community based on voting patterns and interactions.

**Plugin Type**: API Plugin
**Dependencies**: [reputation](reputation.md), [json_rpc](json_rpc.md)
**Memory**: Minimal (queries existing state)
**Default**: Disabled

## Purpose

- **Reputation Queries**: Retrieve reputation scores for accounts
- **Batch Lookups**: Query multiple accounts efficiently
- **User Trust Metrics**: Assess account credibility
- **Spam Detection**: Identify low-reputation accounts

## Configuration

### Enable Plugin

```ini
# In config.ini
plugin = reputation_api
```

Or via command line:

```bash
steemd --plugin=reputation_api
```

### Prerequisites

The `reputation_api` requires the `reputation` state plugin:

```ini
# Enable state tracking plugin first
plugin = reputation
plugin = reputation_api
```

**Important**: The `reputation` plugin must track state from the beginning or replay is required.

### Dependencies Chain

```
chain -> reputation -> reputation_api
```

The reputation plugin also depends on the `follow` plugin:

```ini
plugin = chain
plugin = follow
plugin = reputation
plugin = reputation_api
```

### No Configuration Options

This API plugin has no configurable parameters. All computation is handled by the underlying `reputation` state plugin.

## Reputation System

### How Reputation Works

Reputation is calculated based on:
- **Upvotes received**: Increases reputation
- **Downvotes received**: Decreases reputation
- **Voter reputation**: Votes from high-rep accounts have more impact
- **Voter stake**: Votes weighted by STEEM Power

### Reputation Formula

The raw reputation score is calculated as:

```
reputation = log10(abs(rshares)) - 9

Where rshares = cumulative vote rshares (vote weight * voter SP)
```

### Display Representation

Raw reputation (stored as large integer) is converted to display format (0-100 scale):

```javascript
function calculateDisplayReputation(raw_reputation) {
    if (raw_reputation == 0) return 25;

    let rep = String(raw_reputation);
    let neg = rep.charAt(0) === '-';
    rep = neg ? rep.substring(1) : rep;

    let out = Math.log10(Math.abs(raw_reputation));
    out = Math.max(out - 9, 0);
    out = (neg ? -1 : 1) * out;
    out = (out * 9) + 25;

    return Math.floor(out);
}
```

### Reputation Ranges

| Display Score | Meaning | Typical User |
|---------------|---------|--------------|
| 25 | New account | Just registered |
| 25-35 | Low reputation | New or flagged users |
| 35-50 | Building reputation | Active newer users |
| 50-60 | Established | Regular community members |
| 60-70 | High reputation | Respected contributors |
| 70-80 | Very high | Influential users |
| 80+ | Exceptional | Top contributors, witnesses |

### Properties

- **Starts at 25**: New accounts begin with reputation 25
- **Logarithmic scale**: Harder to gain reputation at higher levels
- **Difficult to decrease**: Requires sustained downvoting
- **Cannot go below 0**: Minimum display reputation is 0
- **No maximum**: Theoretically unlimited (practically capped ~100)

## API Methods

All methods are called via JSON-RPC at the endpoint configured by the `webserver` plugin (default: `http://localhost:8090`).

### get_account_reputations

Get reputation scores for multiple accounts.

**Request Parameters**:
- `account_lower_bound` (string) - Starting account name (alphabetically)
- `limit` (uint32, optional) - Maximum results to return (max: 1000, default: 1000)

**Returns**: Array of account reputation objects

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"reputation_api.get_account_reputations",
  "params":{
    "account_lower_bound":"alice",
    "limit":10
  }
}'
```

**Response**:
```json
{
  "result": {
    "reputations": [
      {
        "account": "alice",
        "reputation": "374891317739480"
      },
      {
        "account": "bob",
        "reputation": "123456789012345"
      },
      {
        "account": "charlie",
        "reputation": "987654321098765"
      }
    ]
  }
}
```

**Response Fields**:
- `account` - Account name
- `reputation` - Raw reputation score (large integer as string)

### Query Patterns

**Single account lookup**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"reputation_api.get_account_reputations",
  "params":{
    "account_lower_bound":"alice",
    "limit":1
  }
}'
```

**Batch lookup** (multiple specific accounts):
```javascript
// Query alphabetically and filter results
async function getBatchReputations(accounts) {
  const sorted = accounts.sort();
  const result = await reputation_api.get_account_reputations({
    account_lower_bound: sorted[0],
    limit: 1000
  });

  // Filter to only requested accounts
  const accountSet = new Set(accounts);
  return result.reputations.filter(r => accountSet.has(r.account));
}
```

**Pagination** (all accounts):
```javascript
async function getAllReputations() {
  let reputations = [];
  let start = "";

  while (true) {
    const result = await reputation_api.get_account_reputations({
      account_lower_bound: start,
      limit: 1000
    });

    if (result.reputations.length === 0) break;

    reputations.push(...result.reputations);

    // Next page starts after last account
    const last = result.reputations[result.reputations.length - 1];
    start = last.account + "\x00"; // Next account alphabetically
  }

  return reputations;
}
```

## Data Structures

### Raw Reputation Value

Reputation is stored as `share_type` (128-bit signed integer):

```
Type: string (in JSON)
Range: -2^127 to 2^127-1
Typical values: 0 to 10^15 for most users
```

### Conversion Functions

**JavaScript**:
```javascript
function formatReputation(rawRep) {
  const raw = parseInt(rawRep);
  if (raw === 0) return 25;

  const abs = Math.abs(raw);
  const sign = raw < 0 ? -1 : 1;
  const log = Math.log10(abs);
  const rep = Math.max(log - 9, 0);
  return Math.floor(sign * rep * 9 + 25);
}

// Usage
const raw = "374891317739480";
const display = formatReputation(raw); // Returns 65
```

**Python**:
```python
import math

def format_reputation(raw_rep):
    if raw_rep == 0:
        return 25

    abs_rep = abs(int(raw_rep))
    sign = -1 if raw_rep < 0 else 1
    log_rep = math.log10(abs_rep)
    rep = max(log_rep - 9, 0)
    return int(sign * rep * 9 + 25)
```

## Use Cases

### Social Media Application

Display user reputation scores:

```ini
plugin = reputation
plugin = reputation_api
plugin = database_api
plugin = tags_api
```

**Features**:
- User profile badges (reputation-based)
- Comment sorting by author reputation
- Trust indicators
- Spam filtering

### Content Moderation

Filter low-reputation content:

```javascript
async function getQualityPosts(tag, minReputation = 35) {
  // Get posts by tag
  const discussions = await tags_api.get_discussions_by_trending({
    tag: tag,
    limit: 100
  });

  // Get author reputations
  const authors = discussions.map(d => d.author);
  const reps = await getBatchReputations(authors);

  // Filter by reputation
  const repMap = new Map(reps.map(r => [
    r.account,
    formatReputation(r.reputation)
  ]));

  return discussions.filter(d =>
    repMap.get(d.author) >= minReputation
  );
}
```

### User Discovery

Find high-reputation accounts:

```javascript
async function getTopReputationAccounts(limit = 100) {
  const allReps = await getAllReputations();

  // Convert to display reputation and sort
  const withDisplay = allReps.map(r => ({
    account: r.account,
    raw: r.reputation,
    display: formatReputation(r.reputation)
  }));

  return withDisplay
    .sort((a, b) => b.display - a.display)
    .slice(0, limit);
}
```

### Reputation Analytics

Track reputation changes over time:

```javascript
// Store reputation snapshots
async function trackReputationHistory(accounts) {
  const timestamp = new Date();
  const reps = await getBatchReputations(accounts);

  // Save to database
  await db.reputationHistory.insertMany(
    reps.map(r => ({
      account: r.account,
      reputation: formatReputation(r.reputation),
      raw: r.reputation,
      timestamp: timestamp
    }))
  );
}
```

### Spam Detection

Identify suspicious accounts:

```javascript
async function detectSpamAccounts(accounts) {
  const reps = await getBatchReputations(accounts);

  return reps
    .filter(r => formatReputation(r.reputation) < 25)
    .map(r => ({
      account: r.account,
      reputation: formatReputation(r.reputation),
      reason: "Low reputation (possible spam)"
    }));
}
```

## Performance Notes

### Batch Queries

Always batch reputation lookups:

**Good** - Single query for multiple accounts:
```javascript
const reps = await reputation_api.get_account_reputations({
  account_lower_bound: "alice",
  limit: 100
});
```

**Bad** - Multiple queries:
```javascript
// Inefficient: N queries
for (const account of accounts) {
  const rep = await reputation_api.get_account_reputations({
    account_lower_bound: account,
    limit: 1
  });
}
```

### Caching

Reputation changes slowly. Cache aggressively:

```javascript
class ReputationCache {
  constructor(ttl = 3600000) { // 1 hour default
    this.cache = new Map();
    this.ttl = ttl;
  }

  async get(account) {
    const cached = this.cache.get(account);
    if (cached && Date.now() - cached.timestamp < this.ttl) {
      return cached.reputation;
    }

    const result = await reputation_api.get_account_reputations({
      account_lower_bound: account,
      limit: 1
    });

    if (result.reputations.length > 0) {
      const rep = formatReputation(result.reputations[0].reputation);
      this.cache.set(account, {
        reputation: rep,
        timestamp: Date.now()
      });
      return rep;
    }

    return 25; // Default for new accounts
  }
}
```

### Pagination Strategy

For large datasets, paginate efficiently:

```javascript
// Stream processing for millions of accounts
async function* streamAllReputations() {
  let start = "";

  while (true) {
    const result = await reputation_api.get_account_reputations({
      account_lower_bound: start,
      limit: 1000
    });

    if (result.reputations.length === 0) break;

    for (const rep of result.reputations) {
      yield rep;
    }

    const last = result.reputations[result.reputations.length - 1];
    start = last.account + "\x00";
  }
}

// Usage
for await (const rep of streamAllReputations()) {
  // Process one at a time (memory efficient)
  processReputation(rep);
}
```

## Troubleshooting

### Always Returns Zero

**Problem**: All accounts have reputation 0

**Causes**:
- `reputation` plugin not enabled
- Plugin started mid-chain without replay
- Database corruption

**Solution**:
```bash
# Verify reputation plugin is enabled
grep "reputation" witness_node_data_dir/config.ini

# Check if data exists
curl -s http://localhost:8090 -d '{"jsonrpc":"2.0","id":1,"method":"reputation_api.get_account_reputations","params":{"account_lower_bound":"","limit":10}}'
```

If all zeros, replay blockchain with `reputation` plugin enabled.

### Missing Accounts

**Problem**: Known accounts not returned

**Causes**:
- Account name spelling error
- Account doesn't exist
- `account_lower_bound` skips over account

**Solution**: Query with lower bound before target account:
```javascript
// To find "charlie", query from "c"
const result = await reputation_api.get_account_reputations({
  account_lower_bound: "c",
  limit: 100
});
```

### Negative Reputation

**Problem**: Reputation displays as negative (< 25)

**Cause**: Account has been heavily downvoted

**Explanation**: This is working as designed. Heavily flagged accounts can have reputation below 25, down to 0.

**Display**: Show as warning indicator in UI:
```javascript
function getReputationBadge(reputation) {
  if (reputation < 0) return "Blocked";
  if (reputation < 25) return "Warning";
  if (reputation < 35) return "New";
  if (reputation < 50) return "Member";
  if (reputation < 60) return "Established";
  if (reputation < 70) return "Trusted";
  return "High Reputation";
}
```

## Reputation Mechanics

### Gaining Reputation

Reputation increases through:
1. **Receiving upvotes** on posts/comments
2. **High-rep voters** (more impact)
3. **Staked voters** (STEEM Power weighted)
4. **Consistent quality** content

### Losing Reputation

Reputation decreases through:
1. **Receiving downvotes** (flags)
2. **High-rep flaggers** (more impact)
3. **Sustained flagging** over time

### Reputation Cannot Be Directly Transferred

- Reputation is calculated from voting activity
- Cannot be bought, sold, or transferred
- Reflects community assessment of contributions
- Tied to account, not transferable

### Gaming Resistance

The reputation system is designed to resist gaming:
- Logarithmic scaling makes it harder at high levels
- Self-voting has minimal impact
- Vote circles detected by community
- Downvotes from established users are powerful

## Integration Examples

### React Component

```jsx
import { useEffect, useState } from 'react';

function ReputationBadge({ account }) {
  const [reputation, setReputation] = useState(25);

  useEffect(() => {
    async function fetchReputation() {
      const result = await reputation_api.get_account_reputations({
        account_lower_bound: account,
        limit: 1
      });

      if (result.reputations.length > 0) {
        const rep = formatReputation(result.reputations[0].reputation);
        setReputation(rep);
      }
    }

    fetchReputation();
  }, [account]);

  return (
    <span className={`reputation rep-${Math.floor(reputation / 10)}`}>
      ({reputation})
    </span>
  );
}
```

### API Endpoint

```javascript
// Express.js endpoint
app.get('/api/reputation/:account', async (req, res) => {
  try {
    const result = await reputation_api.get_account_reputations({
      account_lower_bound: req.params.account,
      limit: 1
    });

    if (result.reputations.length === 0) {
      return res.json({ reputation: 25 }); // Default for new accounts
    }

    const rep = formatReputation(result.reputations[0].reputation);
    res.json({
      account: req.params.account,
      reputation: rep,
      raw: result.reputations[0].reputation
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});
```

## Related Documentation

- [reputation Plugin](reputation.md) - State plugin that calculates reputation
- [follow Plugin](follow.md) - Required dependency for reputation
- [database_api](database_api.md) - Account and content queries
- [Reputation System](../technical-reference/reputation-system.md) - Detailed algorithm

## Source Code

- **API Implementation**: [src/plugins/apis/reputation_api/reputation_api.cpp](../../src/plugins/apis/reputation_api/reputation_api.cpp)
- **API Header**: [src/plugins/apis/reputation_api/include/steem/plugins/reputation_api/reputation_api.hpp](../../src/plugins/apis/reputation_api/include/steem/plugins/reputation_api/reputation_api.hpp)
- **State Plugin**: [src/plugins/reputation/reputation_plugin.cpp](../../src/plugins/reputation/reputation_plugin.cpp)

## License

See [LICENSE.md](../../LICENSE.md)
