# Follow Plugin

Social graph and content feed management.

## Overview

The follow plugin tracks:
- Follow/unfollow relationships
- Mute/ignore lists
- Blog feeds
- Reblog (resteem) activity
- Reputation calculations

## Configuration

```ini
plugin = follow

# Maximum feed size per account
follow-max-feed-size = 500

# Start calculating feeds at specific block
# follow-start-feeds = 0
```

## Features

- **Social Graph**: Follow/mute relationships
- **Blog Feeds**: Personalized content streams
- **Reblogs**: Track content sharing
- **Reputation**: User reputation scores
- **Feed Caching**: Efficient feed generation

## Reputation System

Reputation is calculated based on:
- Votes received
- Voter reputation
- Historical voting patterns

## Memory Considerations

Feed size impacts memory usage. Limit `follow-max-feed-size` for lower memory nodes.

## Dependencies

- `chain` - Account and content data

## Used By

- `follow_api` - Social graph queries
- `reputation_api` - Reputation data (partially)
