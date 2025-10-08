# Condenser API Plugin

Legacy compatibility API for Steem applications.

## Overview

The condenser_api provides:
- Backward compatibility with old API format
- Combined functionality from multiple APIs
- Legacy method names
- Original call signatures

## Configuration

```ini
plugin = condenser_api

# Usually made public
public-api = database_api condenser_api
```

## Features

- **Legacy Support**: Compatible with old Steem apps
- **Combined API**: Aggregates multiple plugin APIs
- **Convenience**: Single API for common operations
- **Backward Compatible**: Maintains old method signatures

## Common Methods

- `get_trending` - Trending posts
- `get_blog` - User blog
- `get_discussions_by_*` - Content queries
- `get_account_history` - Transaction history
- `get_state` - Page state (deprecated)
- `broadcast_transaction` - Submit transactions

## Migration Note

New applications should use specific APIs:
- `database_api` - Blockchain data
- `account_history_api` - Account history
- `follow_api` - Social features
- `tags_api` - Content discovery

## Dependencies

- Multiple state plugins (optional dependencies)
- `json_rpc` - Request handling
- `webserver` - HTTP/WebSocket

## Usage

```bash
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"condenser_api.get_trending",
  "params":["", 10],
  "id":1
}' http://localhost:8090
```

## Notes

Widely used for compatibility. Consider migrating to specific APIs for new development.
