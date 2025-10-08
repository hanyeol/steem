# Tags API Plugin

Content discovery and tag-based queries.

## Overview

Exposes tags plugin data for content discovery.

## Configuration

```ini
plugin = tags_api

# Requires state plugin
plugin = tags
```

## Methods

- `get_trending_tags` - Trending tags
- `get_discussions_by_trending` - Trending posts
- `get_discussions_by_hot` - Hot posts
- `get_discussions_by_created` - New posts
- `get_discussions_by_blog` - Author blog
- `get_discussions_by_feed` - User feed

## Usage

```bash
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"tags_api.get_discussions_by_trending",
  "params":{"tag":"steem", "limit":10},
  "id":1
}' http://localhost:8090
```

## Dependencies

- `tags`
- `json_rpc`
- `webserver`
