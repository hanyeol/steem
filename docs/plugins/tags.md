# tags Plugin

Index blockchain content by tags and categories for efficient content discovery and trending algorithms.

## Overview

The `tags` plugin indexes Steem posts and comments by their tags, enabling efficient content discovery through category browsing, trending algorithms, and author-based queries. It implements Reddit-style hot/trending calculations and maintains statistics for each tag.

**Plugin Type**: State (Indexing)
**Dependencies**: [chain](chain.md)
**Memory**: High (2-5GB depending on content volume)
**Default**: Disabled

## Purpose

- **Tag Indexing**: Index posts by user-defined tags from JSON metadata
- **Trending Algorithm**: Calculate hot/trending scores using Reddit-style algorithms
- **Content Discovery**: Enable browsing by tag, creation time, activity, votes
- **Author Statistics**: Track author posting activity per tag
- **Tag Statistics**: Maintain aggregate stats (post count, votes, trending)

## Configuration

### Config File Options

```ini
# Enable tags plugin
plugin = tags

# Note: This is a state-tracking plugin
# Enabling/disabling requires chain replay
```

### Command Line Options

```bash
steemd --plugin=tags
```

### Configuration Parameters

The tags plugin has no configurable parameters. It operates automatically when enabled.

## Tag Indexing

### Tag Extraction

**Source**: Comment JSON metadata field

```json
{
  "tags": ["photography", "nature", "sunset", "travel"],
  "image": ["https://..."],
  "app": "steemit/0.1"
}
```

**Extraction rules**:
- Maximum 5 tags per post
- Tags converted to lowercase
- Empty tags ignored
- Duplicate tags removed
- Universal tag ("") added for posts with non-negative rewards

### Automatic Tags

**Universal tag** (empty string ""):
- Applied to all posts with `net_rshares >= 0`
- Enables "all content" queries
- Excluded from negative-reward content

## Ranking Algorithms

### Hot Score

**Reddit-style hot algorithm** for recent popular content:

```
hot_score = sign(score) * log10(max(abs(score/10000000), 1)) + created_time/10000
```

**Characteristics**:
- Favors recent content
- Logarithmic scaling (early votes matter more)
- Time component: `created_time / 10000`
- Updates on vote changes

**Use case**: Front page, recent popular content

### Trending Score

**Reddit-style trending algorithm** for long-term popular content:

```
trending_score = sign(score) * log10(max(abs(score/10000000), 1)) + created_time/480000
```

**Characteristics**:
- Similar to hot but slower time decay
- Time component: `created_time / 480000` (48x slower than hot)
- Content trends longer
- Better for discovering established popular content

**Use case**: Trending page, popular content over days

## Database Objects

### tag_object

**Indexes individual posts by tag**:

```cpp
struct tag_object {
  tag_name_type tag;           // Tag string (max 32 chars)
  time_point_sec created;      // Post creation time
  time_point_sec active;       // Last activity (comment/vote)
  time_point_sec cashout;      // Cashout time
  int64_t net_rshares;        // Net reward shares
  int32_t net_votes;          // Net votes (up - down)
  int32_t children;           // Comment count
  double hot;                 // Hot score
  double trending;            // Trending score
  share_type promoted_balance; // Promoted balance

  account_id_type author;     // Post author
  comment_id_type parent;     // Parent comment (or null for posts)
  comment_id_type comment;    // Comment being tagged
};
```

**Indexes**:
- By tag + creation time (newest first)
- By tag + active time (recently active)
- By tag + hot score (hot content)
- By tag + trending score (trending content)
- By tag + net votes (most voted)
- By tag + children count (most discussed)
- By tag + promoted balance (promoted content)
- By author + comment (author's posts)

### tag_stats_object

**Aggregate statistics per tag**:

```cpp
struct tag_stats_object {
  tag_name_type tag;           // Tag string
  asset total_payout;          // Total payouts (SBD)
  int32_t net_votes;          // Sum of net votes
  uint32_t top_posts;         // Count of top-level posts
  uint32_t comments;          // Count of comments
  fc::uint128 total_trending; // Sum of trending scores
};
```

### author_tag_stats_object

**Author statistics per tag**:

```cpp
struct author_tag_stats_object {
  account_id_type author;     // Author account
  tag_name_type tag;          // Tag
  asset total_rewards;        // Total rewards earned (SBD)
  uint32_t total_posts;       // Post count in this tag
};
```

## Query Patterns

### Content Discovery Queries

**Get posts by tag, sorted by created**:
```
Index: by_parent_created
Query: (tag="photography", parent=null, created DESC)
Use: Browse recent posts in category
```

**Get trending posts**:
```
Index: by_parent_trending
Query: (tag="", parent=null, trending DESC)
Use: Trending page (all content)
```

**Get hot posts**:
```
Index: by_parent_hot
Query: (tag="", parent=null, hot DESC)
Use: Hot/front page
```

**Get posts by votes**:
```
Index: by_parent_net_votes
Query: (tag="nature", parent=null, net_votes DESC)
Use: Most voted content
```

**Get most discussed**:
```
Index: by_parent_children
Query: (tag="", parent=null, children DESC)
Use: Posts with most comments
```

**Get promoted posts**:
```
Index: by_parent_promoted
Query: (tag="", parent=null, promoted_balance DESC)
Use: Promoted/advertised content
```

### Author Queries

**Get author's posts in tag**:
```
Index: by_author_comment
Query: (author=alice, tag="photography")
Use: Author's content in category
```

**Get author's top tags**:
```
Index: by_author_posts_tag
Query: (author=alice, total_posts DESC)
Use: Author's most-used tags
```

## Use Cases

### Content Platform

**Purpose**: Enable Steem-based content platform

```ini
# Required plugins
plugin = chain p2p webserver
plugin = tags
plugin = database_api condenser_api

# Tags plugin enables:
# - Tag browsing
# - Trending/hot pages
# - Category discovery
# - Author profiles
```

### API Node for dApps

**Purpose**: Provide content discovery APIs

```ini
plugin = chain p2p tags
plugin = database_api condenser_api follow

# Enables queries:
# - get_discussions_by_trending
# - get_discussions_by_hot
# - get_discussions_by_created
# - get_discussions_by_blog (with follow plugin)
```

### Analytics Platform

**Purpose**: Analyze tag usage and trends

```ini
plugin = chain tags

# Query tag_stats_object for:
# - Popular tags (by post count)
# - Trending tags (by total_trending)
# - Active tags (by comment count)
# - Tag evolution over time
```

### Low-Memory Node

**Purpose**: Consensus node without tags

```ini
# Disable tags plugin to save memory
plugin = chain p2p witness

# Saves 2-5GB RAM
# Cannot answer tag-based queries
# Still validates all content
```

## Memory Requirements

### Memory Usage

**Estimates** (mainnet, 2023):
- tag_object: ~2-3GB (millions of active posts)
- tag_stats_object: ~10MB (thousands of tags)
- author_tag_stats_object: ~500MB (millions of author-tag combinations)
- **Total: 2.5-4GB**

### Low-Memory Mode

**Reduce memory with `IS_LOW_MEM`**:

```bash
# Build with low-memory flag
cmake -DLOW_MEMORY_NODE=ON ..
make

# tags plugin behavior:
# - Still indexes tags
# - Reduced index coverage
# - Some queries unavailable
```

## Performance Considerations

### Indexing Overhead

**Per post/comment**:
- Parse JSON metadata (1-10ms)
- Extract and normalize tags (1ms)
- Create/update tag_objects (1-5 per post)
- Update tag_stats_object
- Update author_tag_stats_object

**Replay performance**:
- Slower than no-tags replay
- Parses all historical JSON metadata
- Rebuilds all tag indexes

### Query Performance

**Fast queries** (uses indexes):
- Get posts by tag + sort order
- Get trending/hot posts
- Get author posts in tag

**Slower queries** (scans):
- Cross-tag queries
- Complex filtering
- Large result sets

## Monitoring

### Plugin Status

**Check initialization**:
```bash
grep "Initializing tags plugin" logs/stderr.txt
```

### Tag Statistics

**Via API**:
```bash
# Get tag stats (if exposed by API plugin)
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"database_api.find_tags",
  "params":{"tags":["photography","nature"]}
}'
```

### Database Size

**Check shared memory usage**:
```bash
# Tag-related database objects
ls -lh witness_node_data_dir/blockchain/shared_memory.bin

# Approximate tag plugin usage
# Compare with/without plugin enabled
```

## Troubleshooting

### Tags Not Updating

**Problem**: New posts not appearing in tag queries

**Check**:
```bash
# Verify plugin enabled
grep "plugin.*tags" config.ini

# Check for errors
grep -i "tags" logs/stderr.txt | grep -i error
```

**Solutions**:
1. Verify `plugin = tags` in config
2. Replay chain if plugin recently enabled
3. Check JSON metadata is valid

### Invalid JSON Metadata

**Problem**: Post tags not extracted

**Cause**: Malformed JSON in post metadata

**Behavior**:
- Plugin catches parse errors
- Post is not tagged
- No error logged (by design)
- Universal tag ("") still applied if rewards >= 0

### Memory Errors

**Problem**: Out of memory with tags plugin

**Symptoms**:
```
bad_alloc exception
shared memory exhausted
```

**Solutions**:
1. Increase shared file size: `--shared-file-size=8G`
2. Use `LOW_MEMORY_NODE=ON` build
3. Disable tags plugin if not needed
4. Add more RAM

### Wrong Trending Order

**Problem**: Trending order seems incorrect

**Check**:
- Trending algorithm favors cumulative score over time
- Recent posts with few votes rank lower
- Old posts with many votes rank higher
- Working as designed (Reddit algorithm)

**Alternative**:
- Use hot algorithm for recent popular content
- Use created for chronological
- Use net_votes for pure vote count

## Advanced Topics

### Tag Limits

**Hard limits**:
- Maximum 5 tags per post
- Tag length: 32 characters (tag_name_type)
- Tags are case-insensitive (converted to lowercase)

**Soft limits** (application layer):
- Many apps enforce additional rules
- Character restrictions
- Banned tags
- Tag formatting

### Score Calculation

**When scores update**:
- On post creation
- On every vote
- On comment (updates parent active time)
- On cashout (may remove from indexes)

**Score persistence**:
- Stored in tag_object
- Recomputed during replay
- Consistent across nodes

### Cashout Behavior

**After cashout**:
- `cashout_time` becomes `fc::time_point_sec::maximum()`
- tag_object removed from indexes
- No longer appears in queries
- Statistics updated

**Exception**: Posts can be queried by author even after cashout

## Migration Notes

### Enabling Tags Plugin

**Requires chain replay**:

```bash
# 1. Stop node
systemctl stop steemd

# 2. Edit config
echo "plugin = tags" >> config.ini

# 3. Replay chain
steemd --replay-blockchain

# 4. Wait for replay to complete (hours to days)
# 5. Node operational with tags enabled
```

### Disabling Tags Plugin

**Also requires chain replay**:

```bash
# 1. Stop node
# 2. Remove "plugin = tags" from config
# 3. Replay chain
# 4. Saves 2-5GB RAM
```

## Related Documentation

- [condenser_api](condenser_api.md) - Tag query API
- [database_api](database_api.md) - Database queries
- [follow Plugin](follow.md) - Social following
- [Plugin Development](../development/plugin.md) - Creating plugins

## Source Code

- **Implementation**: [libraries/plugins/tags/tags_plugin.cpp](../../libraries/plugins/tags/tags_plugin.cpp)
- **Header**: [libraries/plugins/tags/include/steem/plugins/tags/tags_plugin.hpp](../../libraries/plugins/tags/include/steem/plugins/tags/tags_plugin.hpp)

## License

See [LICENSE.md](../../LICENSE.md)
