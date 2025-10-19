# follow_api Plugin

Query follow relationships, social feeds, and blog content on the Steem blockchain.

## Overview

The `follow_api` plugin provides read-only access to social graph data and content feeds. It enables applications to query follower/following relationships, retrieve personalized content feeds, and access blog entries with reblog information.

**Plugin Type**: API Plugin
**Dependencies**: [follow](follow.md), [json_rpc](json_rpc.md)
**Memory**: Minimal (queries existing state)
**Default**: Disabled

## Purpose

- **Social Graph Queries**: Retrieve follower and following lists for accounts
- **Feed Generation**: Access personalized content feeds based on follows
- **Blog Access**: Query account blogs including reblogs
- **Reblog Tracking**: Identify who reblogged specific content
- **Reputation Data**: Query account reputation scores

## Configuration

### Enable Plugin

```ini
# In config.ini
plugin = follow_api
```

Or via command line:

```bash
steemd --plugin=follow_api
```

### Prerequisites

The `follow_api` requires the `follow` state plugin to be enabled:

```ini
plugin = follow
plugin = follow_api
```

**Important**: The `follow` plugin must track state from the beginning or replay is required.

### No Configuration Options

This API plugin has no configurable parameters. All configuration is handled by the underlying `follow` state plugin.

## API Methods

All methods are called via JSON-RPC at the endpoint configured by the `webserver` plugin (default: `http://localhost:8090`).

### get_followers

Get accounts following a specific account.

**Request Parameters**:
- `account` (string) - Account name to query
- `start` (string) - Starting account name for pagination (empty string for first page)
- `type` (string) - Follow type filter: "blog", "ignore", or "" for all
- `limit` (uint32) - Maximum results to return (max: 1000, default: 1000)

**Returns**: Array of follow relationships with follower information

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"follow_api.get_followers",
  "params":{
    "account":"alice",
    "start":"",
    "type":"blog",
    "limit":10
  }
}'
```

**Response**:
```json
{
  "result": {
    "followers": [
      {
        "follower": "bob",
        "following": "alice",
        "what": ["blog"]
      },
      {
        "follower": "charlie",
        "following": "alice",
        "what": ["blog"]
      }
    ]
  }
}
```

### get_following

Get accounts that a specific account is following.

**Request Parameters**:
- `account` (string) - Account name to query
- `start` (string) - Starting account name for pagination
- `type` (string) - Follow type filter: "blog", "ignore", or ""
- `limit` (uint32) - Maximum results (max: 1000, default: 1000)

**Returns**: Array of follow relationships with following information

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"follow_api.get_following",
  "params":{
    "account":"alice",
    "start":"",
    "type":"blog",
    "limit":10
  }
}'
```

### get_follow_count

Get follower and following counts for an account.

**Request Parameters**:
- `account` (string) - Account name to query

**Returns**: Object with follower and following counts

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"follow_api.get_follow_count",
  "params":{
    "account":"alice"
  }
}'
```

**Response**:
```json
{
  "result": {
    "account": "alice",
    "follower_count": 150,
    "following_count": 75
  }
}
```

### get_feed_entries

Get feed entries (author/permlink) for an account's personalized feed.

**Request Parameters**:
- `account` (string) - Account name whose feed to retrieve
- `start_entry_id` (uint32) - Starting entry ID for pagination (0 for newest)
- `limit` (uint32) - Maximum results (max: 500, default: 500)

**Returns**: Array of feed entries with basic post information

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"follow_api.get_feed_entries",
  "params":{
    "account":"alice",
    "start_entry_id":0,
    "limit":20
  }
}'
```

**Response**:
```json
{
  "result": {
    "feed": [
      {
        "author": "bob",
        "permlink": "my-latest-post",
        "reblog_by": ["charlie"],
        "reblog_on": "2025-10-19T12:00:00",
        "entry_id": 1234
      }
    ]
  }
}
```

### get_feed

Get full feed with complete comment objects for an account.

**Request Parameters**:
- `account` (string) - Account name
- `start_entry_id` (uint32) - Starting entry ID for pagination
- `limit` (uint32) - Maximum results (max: 500, default: 500)

**Returns**: Array of feed entries with full comment data

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"follow_api.get_feed",
  "params":{
    "account":"alice",
    "start_entry_id":0,
    "limit":10
  }
}'
```

**Response**: Returns complete comment objects including title, body, votes, payouts, etc.

### get_blog_entries

Get blog entries (author/permlink) from an account's blog.

**Request Parameters**:
- `account` (string) - Account name whose blog to retrieve
- `start_entry_id` (uint32) - Starting entry ID (0 for newest)
- `limit` (uint32) - Maximum results (max: 500, default: 500)

**Returns**: Array of blog entries

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"follow_api.get_blog_entries",
  "params":{
    "account":"alice",
    "start_entry_id":0,
    "limit":20
  }
}'
```

### get_blog

Get full blog with complete comment objects.

**Request Parameters**:
- `account` (string) - Account name
- `start_entry_id` (uint32) - Starting entry ID
- `limit` (uint32) - Maximum results (max: 500, default: 500)

**Returns**: Array of blog entries with full comment data

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"follow_api.get_blog",
  "params":{
    "account":"alice",
    "start_entry_id":0,
    "limit":10
  }
}'
```

### get_account_reputations

Get reputation scores for multiple accounts.

**Request Parameters**:
- `account_lower_bound` (string) - Starting account name (alphabetically)
- `limit` (uint32) - Maximum results (max: 1000, default: 1000)

**Returns**: Array of account reputation objects

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"follow_api.get_account_reputations",
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
        "reputation": "123456789012345"
      },
      {
        "account": "bob",
        "reputation": "987654321098765"
      }
    ]
  }
}
```

### get_reblogged_by

Get list of accounts that reblogged a specific post.

**Request Parameters**:
- `author` (string) - Post author
- `permlink` (string) - Post permlink

**Returns**: Array of account names

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"follow_api.get_reblogged_by",
  "params":{
    "author":"alice",
    "permlink":"my-awesome-post"
  }
}'
```

**Response**:
```json
{
  "result": {
    "accounts": ["bob", "charlie", "dave"]
  }
}
```

### get_blog_authors

Get authors who have been reblogged on a specific blog.

**Request Parameters**:
- `blog_account` (string) - Blog account name

**Returns**: Array of authors with reblog counts

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"follow_api.get_blog_authors",
  "params":{
    "blog_account":"alice"
  }
}'
```

**Response**:
```json
{
  "result": {
    "blog_authors": [
      {
        "author": "bob",
        "count": 15
      },
      {
        "author": "charlie",
        "count": 8
      }
    ]
  }
}
```

## Data Structures

### Follow Types

Follow relationships can be of different types:
- `blog` - Follow for feed inclusion
- `ignore` - Mute/ignore user
- Empty array - Unfollowed

### Entry IDs

Feed and blog entries use monotonically increasing entry IDs for pagination. Use `0` to start from the newest entries, then use the `entry_id` from results to paginate.

### Reputation Scores

Reputation is stored as a large integer (share_type). Convert to display format:

```javascript
function calculateReputation(raw_reputation) {
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

## Use Cases

### Social Media Application

Build a Twitter/Facebook-like feed:

```ini
plugin = follow
plugin = follow_api
plugin = database_api
plugin = condenser_api
```

**Features enabled**:
- Follow/unfollow users
- Personalized feeds
- User profiles with follower counts
- Reblog functionality

### Content Discovery

Discover popular content creators:

```bash
# Get highly followed accounts
follow_api.get_followers -> sort by follower_count

# Find prolific rebloggers
follow_api.get_blog_authors -> sort by count
```

### Analytics Platform

Track social metrics:
- Follower growth over time
- Engagement (reblog counts)
- Network analysis (follower graphs)
- Reputation tracking

## Performance Notes

### Pagination

Always use pagination for large result sets:

```javascript
// Correct: Paginated query
let entries = [];
let start_id = 0;
while (true) {
    let result = await get_blog_entries({
        account: "alice",
        start_entry_id: start_id,
        limit: 100
    });

    if (result.blog.length === 0) break;

    entries.push(...result.blog);
    start_id = result.blog[result.blog.length - 1].entry_id + 1;
}
```

### Feed vs Blog

- **Feed**: Content from followed accounts (personalized)
- **Blog**: Content authored or reblogged by specific account

Feeds are generally larger than blogs for active users.

### Query Optimization

- Use `get_feed_entries` / `get_blog_entries` when you only need author/permlink
- Use `get_feed` / `get_blog` when you need full comment data
- Cache follow counts (update infrequently)
- Batch reputation queries when possible

## Troubleshooting

### Empty Results

**Problem**: Queries return empty arrays

**Causes**:
- Account has no followers/following
- `follow` state plugin not enabled
- Need to replay blockchain with `follow` plugin

**Solution**:
```bash
# Verify follow plugin is active
curl -s http://localhost:8090 -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_active_witnesses"}' | jq

# Check if follow plugin loaded
grep "follow" witness_node_data_dir/config.ini
```

### Reputation Always Zero

**Problem**: `get_account_reputations` returns 0 for all accounts

**Causes**:
- `reputation` state plugin not enabled
- Plugin started mid-chain (must replay)

**Solution**:
```ini
# Enable reputation plugin (requires follow)
plugin = follow
plugin = reputation
plugin = follow_api
```

Then replay the blockchain.

### Feed/Blog Size Limits

**Problem**: Can't retrieve all feed entries

**Cause**: Feed size is limited by `follow` plugin configuration

**Solution**: The `follow` plugin has `max_feed_size` configuration (default: 500). Older entries are pruned. This is by design to manage memory.

## Related Documentation

- [follow Plugin](follow.md) - State plugin that tracks social graph
- [reputation Plugin](reputation.md) - Reputation calculation plugin
- [database_api](database_api.md) - Core blockchain data queries
- [condenser_api](condenser_api.md) - Legacy API compatibility

## Source Code

- **API Implementation**: [libraries/plugins/apis/follow_api/follow_api.cpp](../../libraries/plugins/apis/follow_api/follow_api.cpp)
- **API Header**: [libraries/plugins/apis/follow_api/include/steem/plugins/follow_api/follow_api.hpp](../../libraries/plugins/apis/follow_api/include/steem/plugins/follow_api/follow_api.hpp)
- **State Plugin**: [libraries/plugins/follow/follow_plugin.cpp](../../libraries/plugins/follow/follow_plugin.cpp)

## License

See [LICENSE.md](../../LICENSE.md)
