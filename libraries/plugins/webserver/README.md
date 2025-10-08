# Webserver Plugin

HTTP/WebSocket server for API access.

## Overview

The webserver plugin provides:
- HTTP endpoint for JSON-RPC requests
- WebSocket endpoint for real-time communication
- Connection management
- Request routing to API plugins

## Configuration

```ini
plugin = webserver

# HTTP endpoint
webserver-http-endpoint = 127.0.0.1:8090

# WebSocket endpoint
webserver-ws-endpoint = 127.0.0.1:8090

# Thread pool size for handling requests
webserver-thread-pool-size = 256
```

### Security

For production, bind to localhost and use a reverse proxy:

```ini
# Secure configuration
webserver-http-endpoint = 127.0.0.1:8090
webserver-ws-endpoint = 127.0.0.1:8090
```

Then use nginx or similar for TLS termination and public exposure.

## Features

- **HTTP API**: RESTful JSON-RPC interface
- **WebSocket**: Bidirectional real-time communication
- **Thread Pool**: Concurrent request handling
- **CORS Support**: Cross-origin requests (configurable)

## Usage

APIs are accessed through the webserver:

```bash
# HTTP request
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"condenser_api.get_dynamic_global_properties",
  "params":[],
  "id":1
}' http://localhost:8090

# WebSocket connection
wscat -c ws://localhost:8090
```

## Dependencies

- `chain` - Blockchain access
- `json_rpc` - Request handling

## Required By

All API plugins require the webserver to be accessible.
