# follow Plugin

Track follow relationships and user feeds for social networking features on the Steem blockchain.

## Overview

The `follow` plugin maintains follow/unfollow relationships between accounts and generates personalized content feeds. It processes custom follow operations, tracks blogs, manages reblogs, and calculates user reputation based on voting activity.

**Plugin Type**: State Tracking
**Dependencies**: [chain](chain.md)
**Memory**: Medium (200-500MB depending on network size)
**Default**: Disabled (enable for social features)

## Purpose

- **Follow Tracking**: Maintain follower/following relationships between accounts
- **Feed Generation**: Create personalized content feeds based on follows
- **Blog Management**: Track user blogs and reblogged content
- **Reputation System**: Calculate and maintain user reputation scores
- **Mute Lists**: Support for account muting/ignoring
- **Reblog Support**: Track content reblogs and reblog statistics

## Configuration

### Config File Options

```ini
# Enable follow plugin
plugin = follow

# Maximum feed size per account (default: 500)
follow-max-feed-size = 500

# Block time to start calculating feeds (epoch seconds)
# Use 0 to process all historical data
follow-start-feeds = 0
```

### Command Line Options

```bash
steemd \
  --plugin=follow \
  --follow-max-feed-size=1000 \
  --follow-start-feeds=1451606400
```

### Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `follow-max-feed-size` | 500 | Maximum number of posts cached in each account's feed |
| `follow-start-feeds` | 0 | Unix timestamp when to start building feeds (0 = from genesis) |

## Database Objects

### Follow Relationships

Tracks who follows whom:

```cpp
struct follow_object {
    account_name_type follower;   // Account doing the following
    account_name_type following;  // Account being followed
    uint16_t what;                // Bitmask: 1=blog, 2=ignore/mute
};
```

**Follow Types**:
- `blog` (1): Follow account's posts in feed
- `ignore` (2): Mute/ignore account (hide from feed)

### Feed Objects

Personalized content feeds:

```cpp
struct feed_object {
    account_name_type account;              // Feed owner
    comment_id_type comment;                // Post in feed
    vector<account_name_type> reblogged_by; // Who reblogged this
    account_name_type first_reblogged_by;   // First reblogger
    time_point_sec first_reblogged_on;      // First reblog time
    uint32_t account_feed_id;               // Sequential feed ID
};
```

### Blog Objects

User-authored and reblogged content:

```cpp
struct blog_object {
    account_name_type account;    // Blog owner
    comment_id_type comment;      // Post in blog
    time_point_sec reblogged_on;  // When reblogged (if applicable)
    uint32_t blog_feed_id;        // Sequential blog ID
};
```

### Reputation Objects

User reputation scores:

```cpp
struct reputation_object {
    account_name_type account;  // Account name
    share_type reputation;      // Reputation score (can be negative)
};
```

### Follow Count Objects

Follower/following statistics:

```cpp
struct follow_count_object {
    account_name_type account;  // Account name
    uint32_t follower_count;    // Number of followers
    uint32_t following_count;   // Number of accounts followed
};
```

## Custom Operations

### Follow Operation

Follow or unfollow an account:

```json
{
  "id": "follow",
  "required_posting_auths": ["alice"],
  "json": {
    "follower": "alice",
    "following": "bob",
    "what": ["blog"]
  }
}
```

**Fields**:
- `follower`: Account performing the follow action
- `following`: Account being followed/unfollowed
- `what`: Array of actions - `["blog"]` to follow, `[""]` or `[]` to unfollow, `["ignore"]` to mute

**Examples**:

Follow an account:
```json
{
  "follower": "alice",
  "following": "bob",
  "what": ["blog"]
}
```

Unfollow an account:
```json
{
  "follower": "alice",
  "following": "bob",
  "what": []
}
```

Mute/ignore an account:
```json
{
  "follower": "alice",
  "following": "spam-account",
  "what": ["ignore"]
}
```

### Reblog Operation

Reblog (share) content to your blog:

```json
{
  "id": "follow",
  "required_posting_auths": ["alice"],
  "json": {
    "account": "alice",
    "author": "bob",
    "permlink": "test-post"
  }
}
```

**Fields**:
- `account`: Account doing the reblog
- `author`: Original post author
- `permlink`: Original post permlink

**Restrictions**:
- Cannot reblog your own posts
- Cannot reblog comments (only top-level posts)
- Can only reblog each post once
- Reblogs added to your blog and followers' feeds

## Feed Generation

### How Feeds Work

When a user creates a post:
1. Post added to author's blog
2. Post added to each follower's feed
3. Feed limited to `follow-max-feed-size` most recent posts
4. Older posts automatically removed from feed

### Feed Population

Feeds only populate after `follow-start-feeds` time:

```ini
# Start feeds from specific date (Jan 1, 2020)
follow-start-feeds = 1577836800

# Process all historical data (warning: slow initial sync)
follow-start-feeds = 0
```

**Performance Tip**: Set `follow-start-feeds` to recent time to avoid processing old historical data during initial sync.

### Feed Size Management

Feeds are automatically trimmed:

```cpp
// When account_feed_id exceeds follow-max-feed-size:
// 1. Oldest posts are removed
// 2. Only most recent N posts retained
// 3. Keeps memory usage bounded
```

Recommended sizes:
- **Mobile/Light Nodes**: 100-200
- **Standard Nodes**: 500 (default)
- **Archive Nodes**: 1000-2000

## Reputation System

### Reputation Calculation

Reputation is calculated from vote rshares:

```cpp
reputation_delta = vote_rshares >> 6;  // Shift away vesting precision
new_reputation = old_reputation + reputation_delta;
```

### Reputation Rules

**Rule #1**: Negative reputation cannot affect others
- Users with reputation < 0 cannot change others' reputation
- Prevents spam accounts from griefing

**Rule #2**: Downvotes require higher reputation
- To downvote and affect reputation, voter must have higher reputation than author
- Prevents reputation attacks from lower-reputation accounts

### Reputation Examples

Initial reputation (new account): `0`

Upvote from high-reputation user: `+1000`
```
New reputation: 1000
```

Downvote (voter rep > author rep): `-500`
```
New reputation: 500
```

Downvote (voter rep < author rep): No change
```
Reputation unchanged (rule #2 blocks it)
```

Upvote from negative-reputation user: No change
```
Reputation unchanged (rule #1 blocks it)
```

### Reputation Display

Raw reputation values are large integers. UIs typically convert to 0-100 scale:

```cpp
// Common conversion (used by Steemit.com)
log10_reputation = log10(abs(reputation));
display_reputation = (log10_reputation - 9) * 9 + 25;

// Negative reputation shown as values below 25
// Positive reputation: 25-100+
// New accounts: 25
// High reputation: 60-80
```

## Data Flow

### New Post Flow

```
Author creates post
        ↓
Post added to author's blog
        ↓
Query author's followers
        ↓
For each follower with "blog" follow:
    ↓
    Add post to follower's feed
    ↓
    Trim feed to max size
```

### Reblog Flow

```
User reblogs post
        ↓
Validation (not own post, is top-level, not already reblogged)
        ↓
Add to user's blog with reblog flag
        ↓
Add to user's followers' feeds
        ↓
Update reblog tracking in feed objects
```

### Vote Reputation Flow

```
User votes on content
        ↓
Calculate reputation delta from rshares
        ↓
Check reputation rules
        ↓
Apply reputation change to author
        ↓
Store in reputation_object
```

## Performance Tuning

### Memory Usage

Estimate memory usage:

```
follow_objects: num_relationships × 32 bytes
feed_objects: num_users × feed_size × 128 bytes
blog_objects: num_users × feed_size × 64 bytes
reputation_objects: num_users × 32 bytes

Example (1M users, 500 feed size, 100K relationships):
follow: 100K × 32 = 3.2 MB
feeds: 1M × 500 × 128 = 64 GB (!)
blogs: 1M × 500 × 64 = 32 GB
reputation: 1M × 32 = 32 MB

Total: ~96 GB (with 1M active users)
```

### Reducing Memory

**Option 1**: Reduce feed size
```ini
follow-max-feed-size = 100  # Reduces by 80%
```

**Option 2**: Start feeds later
```ini
follow-start-feeds = 1609459200  # Only recent feeds
```

**Option 3**: Use external database
- Consider using `account_history_rocksdb` pattern for feeds
- Store feeds in external database instead of memory

### CPU Optimization

Feed population is CPU-intensive:

**Slow**:
```ini
follow-start-feeds = 0  # Process all history
```

**Fast**:
```ini
follow-start-feeds = 1640995200  # Only recent activity
```

### Disk I/O

Chainbase memory-mapped files:
- Feed/blog objects stored in shared_memory.bin
- Large file size with many users
- SSD strongly recommended

## Monitoring

### Follow Statistics

Query via database API (requires database_api plugin):

```bash
# Get follow counts
curl -s http://localhost:8090 \
  -d '{"jsonrpc":"2.0","id":1,"method":"follow_api.get_follow_count","params":["alice"]}'

# Response
{
  "account": "alice",
  "follower_count": 1234,
  "following_count": 567
}
```

### Feed Status

Monitor feed population:

```bash
# Check logs for feed processing
tail -f witness_node_data_dir/logs/stderr.txt | grep -i feed

# Sample output:
# Feed populated for user alice: 500 posts
# Trimmed old feed entries for bob: removed 50
```

### Memory Monitoring

Check shared memory file size:

```bash
ls -lh witness_node_data_dir/blockchain/shared_memory.bin

# Large file indicates high follow plugin usage
# 20GB: Moderate usage
# 50GB+: Heavy usage (many users with full feeds)
```

## Troubleshooting

### Feeds Not Populating

**Problem**: User feeds empty or incomplete

**Symptoms**:
```
get_feed returns empty array
New posts not appearing in feeds
```

**Solutions**:
1. Check `follow-start-feeds` setting:
   ```ini
   # If too far in future, no feeds populate
   follow-start-feeds = 0
   ```

2. Verify follow relationships exist:
   ```
   Check follow_count shows follower_count > 0
   ```

3. Ensure posts are top-level (not comments):
   ```
   Feeds only include root posts, not replies
   ```

### High Memory Usage

**Problem**: Node using excessive memory

**Symptoms**:
```
shared_memory.bin > 50GB
OOM kills or swap thrashing
```

**Solutions**:
1. Reduce feed size:
   ```ini
   follow-max-feed-size = 200
   ```

2. Replay chain to apply new settings:
   ```bash
   # Stop node
   steemd --replay-blockchain
   ```

3. Consider not using follow plugin:
   ```ini
   # Disable if social features not needed
   # plugin = follow  # commented out
   ```

### Reputation Not Updating

**Problem**: User reputation stuck or incorrect

**Symptoms**:
```
Votes not changing reputation
Reputation shows 0 for active accounts
```

**Causes**:
- Voter has negative reputation (rule #1)
- Downvoter has lower reputation than target (rule #2)
- Post payout already occurred (votes don't count)

**Solutions**:
1. Verify voter reputation:
   ```
   Check voter has reputation >= 0
   ```

2. For downvotes, check reputation comparison:
   ```
   Voter reputation must be > target reputation
   ```

3. Confirm vote is on active post:
   ```
   Post must be within payout window (7 days)
   ```

### Slow Sync with Follow Plugin

**Problem**: Initial sync extremely slow

**Symptoms**:
```
Sync stuck processing old blocks
High CPU usage on old blocks
```

**Solutions**:
1. Set future start time:
   ```ini
   follow-start-feeds = 1672531200  # Jan 1, 2023
   ```

2. Sync without plugin first:
   ```bash
   # Sync without follow
   steemd
   # Stop when synced
   # Enable follow plugin
   # Restart (only processes new blocks)
   ```

## Use Cases

### Social Media Application

Full social features with feeds and follows:

```ini
plugin = chain p2p webserver follow

# Moderate feed sizes for responsive app
follow-max-feed-size = 500

# Only recent content
follow-start-feeds = 1640995200

# Enable follow API
plugin = follow_api
```

### Reputation-Only Node

Track reputation without feeds:

```ini
plugin = chain follow

# Disable feed generation (save memory)
follow-max-feed-size = 0

# Or use separate reputation plugin
plugin = reputation
```

### Archive Node

Complete historical data:

```ini
plugin = chain follow

# Large feed size for historical queries
follow-max-feed-size = 2000

# Process all history
follow-start-feeds = 0
```

### Light Node

Minimal resource usage:

```ini
# Don't use follow plugin
# Use for basic queries
# Rely on external services for social features
```

## API Integration

The follow plugin requires the `follow_api` plugin for RPC access:

```ini
plugin = follow follow_api
```

**Common APIs**:
- `follow_api.get_followers` - List account's followers
- `follow_api.get_following` - List accounts followed
- `follow_api.get_follow_count` - Get follower/following counts
- `follow_api.get_feed` - Get personalized feed
- `follow_api.get_blog` - Get user's blog
- `follow_api.get_account_reputations` - Batch reputation query

See API documentation for detailed usage.

## Migration Notes

### From No Follow to Follow

Enabling follow plugin:

1. **Add to config**:
   ```ini
   plugin = follow
   follow-start-feeds = 1672531200  # Recent date
   ```

2. **Replay blockchain**:
   ```bash
   steemd --replay-blockchain
   ```

3. **Wait for sync**:
   - Only processes blocks after `follow-start-feeds`
   - Much faster than full history

### Changing Feed Size

To change `follow-max-feed-size`:

1. **Stop node**
2. **Update config**:
   ```ini
   follow-max-feed-size = 1000  # New size
   ```
3. **Replay blockchain**:
   ```bash
   steemd --replay-blockchain
   ```

**Note**: No way to change feed size without replay.

## Security Considerations

### Spam Prevention

Follow plugin has built-in spam protection:

1. **Feed size limits**: Prevents memory exhaustion
2. **Reblog restrictions**: Can't reblog same post twice
3. **Reputation system**: Negative rep users can't griefing

### Resource Limits

Monitor resource usage:

```bash
# Set ulimits
ulimit -n 65535      # File descriptors
ulimit -v 134217728  # Virtual memory (128GB)
```

### Data Validation

All follow operations validated:

- Account existence checked
- Posting authority required
- Invalid operations rejected
- No consensus impact (plugin only)

## Related Documentation

- [follow_api Plugin](follow_api.md) - RPC API for follow data
- [reputation Plugin](reputation.md) - Standalone reputation tracking
- [chain Plugin](chain.md) - Blockchain database

## Source Code

- **Implementation**: [src/plugins/follow/follow_plugin.cpp](../../src/plugins/follow/follow_plugin.cpp)
- **Header**: [src/plugins/follow/include/steem/plugins/follow/follow_plugin.hpp](../../src/plugins/follow/include/steem/plugins/follow/follow_plugin.hpp)
- **Operations**: [src/plugins/follow/include/steem/plugins/follow/follow_operations.hpp](../../src/plugins/follow/include/steem/plugins/follow/follow_operations.hpp)
- **Objects**: [src/plugins/follow/include/steem/plugins/follow/follow_objects.hpp](../../src/plugins/follow/include/steem/plugins/follow/follow_objects.hpp)
- **Evaluators**: [src/plugins/follow/follow_evaluators.cpp](../../src/plugins/follow/follow_evaluators.cpp)

## License

See [LICENSE.md](../../LICENSE.md)
