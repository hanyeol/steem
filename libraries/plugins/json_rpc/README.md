# JSON-RPC Plugin

Handles JSON-RPC 2.0 protocol for API requests.

## Overview

The json_rpc plugin:
- Routes JSON-RPC requests to appropriate API plugins
- Handles request/response formatting
- Manages API registration
- Supports both HTTP and WebSocket protocols

## Configuration

```ini
plugin = json_rpc
```

No additional configuration required.

## Features

- **JSON-RPC 2.0**: Standard compliant protocol
- **API Routing**: Dispatches calls to registered APIs
- **Async Handling**: Non-blocking request processing
- **Error Handling**: Standardized error responses

## Usage

Requests follow JSON-RPC 2.0 format:

```json
{
  "jsonrpc": "2.0",
  "method": "api_name.method_name",
  "params": [...],
  "id": 1
}
```

## Dependencies

- `webserver` - HTTP/WebSocket transport

## Required By

All API plugins require json_rpc for request handling.
