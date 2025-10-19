# tags_api Plugin

Query content discussions, trending tags, and tag-based content discovery on the Steem blockchain.

## Overview

The `tags_api` plugin provides read-only access to content organized by tags and various sorting methods. It enables applications to discover content, query discussions, retrieve trending tags, and access post metadata with voting information.

**Plugin Type**: API Plugin
**Dependencies**: [tags](tags.md), [json_rpc](json_rpc.md)
**Memory**: Minimal (queries existing state)
**Default**: Disabled

## Purpose

- **Content Discovery**: Find posts by tags and sorting criteria
- **Trending Tags**: Query popular tags and their statistics
- **Discussion Queries**: Retrieve posts with complete metadata
- **Vote Information**: Access detailed voting data for posts
- **Author Analytics**: Track tag usage by authors

## Configuration

### Enable Plugin

```ini
# In config.ini
plugin = tags_api
```

Or via command line:

```bash
steemd --plugin=tags_api
```

### Prerequisites

The `tags_api` requires the `tags` state plugin:

```ini
# Enable state tracking plugin first
plugin = tags
plugin = tags_api
```

**Important**: The `tags` plugin must track state from the beginning or replay is required.

### No Configuration Options

This API plugin has no configurable parameters. All configuration is handled by the underlying `tags` state plugin.

## API Methods

All methods are called via JSON-RPC at the endpoint configured by the `webserver` plugin (default: `http://localhost:8090`).

### get_trending_tags

Get list of trending tags sorted by activity.

**Request Parameters**:
- `start_tag` (string) - Starting tag for pagination (empty string for first page)
- `limit` (uint32, optional) - Maximum results (max: 100, default: 100)

**Returns**: Array of tag statistics

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"tags_api.get_trending_tags",
  "params":{
    "start_tag":"",
    "limit":10
  }
}'
```

**Response**:
```json
{
  "result": {
    "tags": [
      {
        "name": "steemit",
        "total_payouts": {"amount": "123456789", "precision": 3, "nai": "@@000000013"},
        "net_votes": 45678,
        "top_posts": 1234,
        "comments": 5678,
        "trending": "98765432109876543210"
      },
      {
        "name": "cryptocurrency",
        "total_payouts": {"amount": "98765432", "precision": 3, "nai": "@@000000013"},
        "net_votes": 34567,
        "top_posts": 987,
        "comments": 4321,
        "trending": "87654321098765432109"
      }
    ]
  }
}
```

### get_tags_used_by_author

Get tags used by a specific author with usage counts.

**Request Parameters**:
- `author` (string) - Author account name

**Returns**: Array of tags with post counts

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"tags_api.get_tags_used_by_author",
  "params":{
    "author":"alice"
  }
}'
```

**Response**:
```json
{
  "result": {
    "tags": [
      {"tag": "technology", "count": 45},
      {"tag": "steemit", "count": 32},
      {"tag": "photography", "count": 18}
    ]
  }
}
```

### get_discussion

Get a single discussion (post or comment) by author and permlink.

**Request Parameters**:
- `author` (string) - Author account name
- `permlink` (string) - Post permlink

**Returns**: Discussion object with full metadata

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"tags_api.get_discussion",
  "params":{
    "author":"alice",
    "permlink":"my-awesome-post"
  }
}'
```

**Response**: Returns complete discussion object (see Discussion Structure below)

### get_content_replies

Get all replies to a specific post or comment.

**Request Parameters**:
- `author` (string) - Parent author
- `permlink` (string) - Parent permlink

**Returns**: Array of reply discussions

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"tags_api.get_content_replies",
  "params":{
    "author":"alice",
    "permlink":"my-awesome-post"
  }
}'
```

### Discussion Query Methods

All discussion query methods use the `discussion_query` parameter structure:

**Query Parameters**:
- `tag` (string) - Tag to filter by
- `limit` (uint32) - Maximum results (max: 100)
- `filter_tags` (array) - Tags to exclude
- `select_authors` (array) - Only include these authors
- `select_tags` (array) - Only include these tags
- `truncate_body` (uint32) - Truncate body to N bytes (0 = no truncation)
- `start_author` (string) - Starting author for pagination
- `start_permlink` (string) - Starting permlink for pagination
- `parent_author` (string, optional) - Filter by parent author
- `parent_permlink` (string, optional) - Filter by parent permlink

### get_discussions_by_trending

Get posts sorted by trending score.

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"tags_api.get_discussions_by_trending",
  "params":{
    "tag":"steemit",
    "limit":20
  }
}'
```

### get_discussions_by_created

Get posts sorted by creation time (newest first).

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"tags_api.get_discussions_by_created",
  "params":{
    "tag":"technology",
    "limit":20
  }
}'
```

### get_discussions_by_active

Get posts sorted by last activity (comments).

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"tags_api.get_discussions_by_active",
  "params":{
    "tag":"photography",
    "limit":20
  }
}'
```

### get_discussions_by_cashout

Get posts sorted by payout time (ending soon first).

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"tags_api.get_discussions_by_cashout",
  "params":{
    "tag":"contest",
    "limit":20
  }
}'
```

### get_discussions_by_votes

Get posts sorted by net votes (highest first).

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"tags_api.get_discussions_by_votes",
  "params":{
    "tag":"news",
    "limit":20
  }
}'
```

### get_discussions_by_children

Get posts sorted by number of comments (most discussed first).

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"tags_api.get_discussions_by_children",
  "params":{
    "tag":"debate",
    "limit":20
  }
}'
```

### get_discussions_by_hot

Get posts sorted by "hot" algorithm (recent + trending).

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"tags_api.get_discussions_by_hot",
  "params":{
    "tag":"gaming",
    "limit":20
  }
}'
```

### get_discussions_by_feed

Get posts from accounts followed by specified user.

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"tags_api.get_discussions_by_feed",
  "params":{
    "tag":"alice",
    "limit":20
  }
}'
```

Note: For `get_discussions_by_feed`, the `tag` parameter is actually the account name.

### get_discussions_by_blog

Get posts from a specific author's blog (including reblogs).

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"tags_api.get_discussions_by_blog",
  "params":{
    "tag":"alice",
    "limit":20
  }
}'
```

### get_discussions_by_comments

Get recent comments by an author.

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"tags_api.get_discussions_by_comments",
  "params":{
    "tag":"alice",
    "limit":20
  }
}'
```

### get_discussions_by_promoted

Get posts sorted by promoted balance (SBD spent on promotion).

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"tags_api.get_discussions_by_promoted",
  "params":{
    "tag":"promotion",
    "limit":20
  }
}'
```

### get_post_discussions_by_payout

Get root posts (not comments) sorted by pending payout.

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"tags_api.get_post_discussions_by_payout",
  "params":{
    "tag":"contest",
    "limit":20
  }
}'
```

### get_comment_discussions_by_payout

Get comments (not root posts) sorted by pending payout.

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"tags_api.get_comment_discussions_by_payout",
  "params":{
    "tag":"",
    "limit":20
  }
}'
```

### get_replies_by_last_update

Get replies sorted by last update time.

**Request Parameters**:
- `start_parent_author` (string) - Starting parent author for pagination
- `start_permlink` (string) - Starting permlink for pagination
- `limit` (uint32) - Maximum results (max: 100, default: 100)

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"tags_api.get_replies_by_last_update",
  "params":{
    "start_parent_author":"alice",
    "start_permlink":"my-post",
    "limit":50
  }
}'
```

### get_discussions_by_author_before_date

Get posts by a specific author before a given date.

**Request Parameters**:
- `author` (string) - Author account name
- `start_permlink` (string) - Starting permlink for pagination
- `before_date` (timestamp) - Only posts before this date
- `limit` (uint32) - Maximum results (max: 100, default: 100)

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"tags_api.get_discussions_by_author_before_date",
  "params":{
    "author":"alice",
    "start_permlink":"",
    "before_date":"2025-10-19T00:00:00",
    "limit":20
  }
}'
```

### get_active_votes

Get detailed voting information for a post.

**Request Parameters**:
- `author` (string) - Post author
- `permlink` (string) - Post permlink

**Returns**: Array of vote objects

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"tags_api.get_active_votes",
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
    "votes": [
      {
        "voter": "bob",
        "weight": 10000,
        "rshares": 123456789,
        "percent": 10000,
        "reputation": "374891317739480",
        "time": "2025-10-19T12:00:00"
      },
      {
        "voter": "charlie",
        "weight": 5000,
        "rshares": 67890123,
        "percent": 5000,
        "reputation": "298765432109876",
        "time": "2025-10-19T12:05:00"
      }
    ]
  }
}
```

## Discussion Structure

Discussion objects contain extensive metadata:

```json
{
  "id": 12345,
  "author": "alice",
  "permlink": "my-awesome-post",
  "category": "technology",
  "parent_author": "",
  "parent_permlink": "technology",
  "title": "My Awesome Post",
  "body": "Post content here...",
  "json_metadata": "{\"tags\":[\"technology\",\"blockchain\"],\"image\":[\"url\"]}",
  "created": "2025-10-19T10:00:00",
  "last_update": "2025-10-19T10:30:00",
  "depth": 0,
  "children": 15,
  "net_rshares": 123456789,
  "abs_rshares": 123456789,
  "vote_rshares": 123456789,
  "children_abs_rshares": 987654321,
  "cashout_time": "2025-10-26T10:00:00",
  "max_cashout_time": "2025-10-26T10:00:00",
  "total_vote_weight": 100000,
  "reward_weight": 10000,
  "total_payout_value": {"amount": "0", "precision": 3, "nai": "@@000000013"},
  "curator_payout_value": {"amount": "0", "precision": 3, "nai": "@@000000013"},
  "author_rewards": 0,
  "net_votes": 45,
  "root_author": "alice",
  "root_permlink": "my-awesome-post",
  "max_accepted_payout": {"amount": "1000000000", "precision": 3, "nai": "@@000000013"},
  "percent_steem_dollars": 10000,
  "allow_replies": true,
  "allow_votes": true,
  "allow_curation_rewards": true,

  "url": "/technology/@alice/my-awesome-post",
  "root_title": "My Awesome Post",
  "pending_payout_value": {"amount": "12345", "precision": 3, "nai": "@@000000013"},
  "total_pending_payout_value": {"amount": "15678", "precision": 3, "nai": "@@000000013"},
  "active_votes": [...],
  "replies": ["alice/reply-1", "bob/reply-2"],
  "author_reputation": "374891317739480",
  "promoted": {"amount": "1000", "precision": 3, "nai": "@@000000013"},
  "body_length": 1234,
  "reblogged_by": ["charlie", "dave"],
  "first_reblogged_by": "charlie",
  "first_reblogged_on": "2025-10-19T11:00:00"
}
```

**Key Fields**:
- `author`, `permlink` - Unique identifier
- `title`, `body` - Content
- `category` - Primary tag
- `json_metadata` - Additional metadata (tags, images, etc.)
- `children` - Number of comments
- `net_votes` - Upvotes minus downvotes
- `pending_payout_value` - Estimated payout
- `active_votes` - Detailed vote information
- `author_reputation` - Author's reputation score

## Use Cases

### Content Discovery Application

Build a content browsing interface:

```ini
plugin = tags
plugin = tags_api
plugin = database_api
plugin = reputation_api
```

**Features**:
- Trending posts by tag
- Multiple sort options
- Tag exploration
- Author profiles

### Social Media Frontend

Complete Steem frontend:

```ini
plugin = tags
plugin = tags_api
plugin = follow
plugin = follow_api
plugin = reputation
plugin = reputation_api
plugin = network_broadcast_api
```

**Features**:
- Content feeds (trending, hot, new)
- User blogs and feeds
- Voting and commenting
- Tag-based navigation

### Analytics Platform

Content and tag analytics:

```javascript
// Track trending tags over time
async function analyzeTrendingTags() {
  const tags = await tags_api.get_trending_tags({
    start_tag: "",
    limit: 100
  });

  return tags.tags.map(t => ({
    name: t.name,
    posts: t.top_posts,
    comments: t.comments,
    votes: t.net_votes,
    payouts: formatAsset(t.total_payouts)
  }));
}

// Author content analysis
async function analyzeAuthor(author) {
  const [tags, posts] = await Promise.all([
    tags_api.get_tags_used_by_author({author}),
    tags_api.get_discussions_by_blog({tag: author, limit: 100})
  ]);

  return {
    total_posts: posts.discussions.length,
    tags: tags.tags,
    avg_votes: posts.discussions.reduce((sum, p) =>
      sum + p.net_votes, 0) / posts.discussions.length
  };
}
```

### Content Curation Bot

Automated content discovery:

```javascript
async function findQualityContent(tag, minVotes = 10) {
  const discussions = await tags_api.get_discussions_by_trending({
    tag: tag,
    limit: 100
  });

  return discussions.discussions.filter(d =>
    d.net_votes >= minVotes &&
    d.children >= 5 &&
    d.body_length >= 500
  );
}
```

## Performance Notes

### Query Optimization

**Pagination**: Use `start_author` and `start_permlink` for large result sets
```javascript
let allPosts = [];
let startAuthor = "";
let startPermlink = "";

while (true) {
  const result = await tags_api.get_discussions_by_created({
    tag: "technology",
    limit: 100,
    start_author: startAuthor,
    start_permlink: startPermlink
  });

  if (result.discussions.length === 0) break;

  allPosts.push(...result.discussions);

  const last = result.discussions[result.discussions.length - 1];
  startAuthor = last.author;
  startPermlink = last.permlink;
}
```

**Body Truncation**: Reduce payload size for list views
```javascript
// Get summaries only (first 500 chars)
const summaries = await tags_api.get_discussions_by_trending({
  tag: "news",
  limit: 20,
  truncate_body: 500
});
```

**Filter Tags**: Exclude unwanted tags
```javascript
const discussions = await tags_api.get_discussions_by_trending({
  tag: "steemit",
  limit: 20,
  filter_tags: ["spam", "nsfw"]
});
```

### Caching Strategy

Discussion data changes frequently. Cache based on update patterns:

| Data Type | Cache Duration | Reason |
|-----------|----------------|--------|
| Trending tags | 5-10 minutes | Slow changes |
| Hot posts | 1-3 minutes | Moderate updates |
| New posts | 30-60 seconds | Fast creation |
| Post details | Until payout | Content locked after payout |
| Active votes | 10-30 seconds | Real-time voting |

### Memory Considerations

Discussion queries can return large payloads:
- Full discussion: ~5-10KB per post
- With votes: +1-5KB per post
- With body: +variable (body length)

Optimize for mobile/slow connections by using truncation.

## Troubleshooting

### No Results for Tag

**Problem**: `get_discussions_by_*` returns empty array

**Causes**:
- Tag doesn't exist or has no posts
- `tags` plugin not enabled
- Plugin started mid-chain

**Solution**: Verify tag exists first:
```bash
curl -s http://localhost:8090 -d '{"jsonrpc":"2.0","id":1,"method":"tags_api.get_trending_tags","params":{"start_tag":"","limit":100}}'
```

### Pagination Not Working

**Problem**: Same results returned repeatedly

**Cause**: Incorrect pagination parameters

**Solution**: Use exact author/permlink from last result:
```javascript
// Correct
const last = result.discussions[result.discussions.length - 1];
nextQuery.start_author = last.author;
nextQuery.start_permlink = last.permlink;

// Incorrect - off-by-one errors
nextQuery.start_permlink = last.permlink + "a";
```

### Discussion Missing Fields

**Problem**: Some discussion fields are null or missing

**Causes**:
- Post not fully indexed yet
- Comment (not root post)
- Payout already occurred

**Solution**: Check `depth` field:
- `depth: 0` = root post
- `depth: > 0` = comment

Comments have limited fields (no title, etc.)

### Slow Queries

**Problem**: Discussion queries take multiple seconds

**Causes**:
- Large result sets
- Complex filtering
- High server load

**Solutions**:
1. Reduce `limit` parameter
2. Use `truncate_body` to reduce payload
3. Query during off-peak hours
4. Use more specific tags (less content)

## Related Documentation

- [tags Plugin](tags.md) - State plugin configuration
- [follow_api](follow_api.md) - User feeds and blogs
- [reputation_api](reputation_api.md) - Author reputation
- [database_api](database_api.md) - Core blockchain queries
- [condenser_api](condenser_api.md) - Legacy compatibility API

## Source Code

- **API Implementation**: [libraries/plugins/apis/tags_api/tags_api.cpp](../../libraries/plugins/apis/tags_api/tags_api.cpp)
- **API Header**: [libraries/plugins/apis/tags_api/include/steem/plugins/tags_api/tags_api.hpp](../../libraries/plugins/apis/tags_api/include/steem/plugins/tags_api/tags_api.hpp)
- **State Plugin**: [libraries/plugins/tags/tags_plugin.cpp](../../libraries/plugins/tags/tags_plugin.cpp)

## License

See [LICENSE.md](../../LICENSE.md)
