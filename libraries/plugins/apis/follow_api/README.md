# Follow API Plugin

Social graph and feed queries.

## Overview

Exposes follow plugin data for social features.

## Configuration

```ini
plugin = follow_api

# Requires state plugin
plugin = follow
```

## Methods

- `get_followers` - Account followers
- `get_following` - Accounts followed
- `get_follow_count` - Follow statistics
- `get_feed` - Personalized feed
- `get_blog` - User blog
- `get_account_reputations` - Reputation data

## Usage

```bash
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"follow_api.get_followers",
  "params":{"account":"alice", "start":"", "type":"blog", "limit":100},
  "id":1
}' http://localhost:8090
```

## Dependencies

- `follow`
- `json_rpc`
- `webserver`
