# webserver Plugin

HTTP/WebSocket server for exposing blockchain APIs to external applications.

## Overview

The `webserver` plugin provides network endpoints for accessing Steem blockchain data through JSON-RPC 2.0 APIs. It acts as the gateway between external applications and the blockchain node.

**Plugin Type**: Core Infrastructure
**Dependencies**: [json_rpc](json_rpc.md) (required)
**Memory**: Minimal (~100MB)
**Default**: Enabled by default

## Purpose

- **HTTP Server**: Serve REST API requests
- **WebSocket Server**: Provide real-time bidirectional communication
- **Request Routing**: Dispatch JSON-RPC requests to appropriate API plugins
- **Multi-threading**: Handle concurrent API requests efficiently
- **Protocol**: Implements JSON-RPC 2.0 specification

## Configuration

### Config File Options

```ini
# HTTP endpoint (REST API)
webserver-http-endpoint = 0.0.0.0:8090

# WebSocket endpoint (real-time)
webserver-ws-endpoint = 0.0.0.0:8091

# Thread pool size for request handling
webserver-thread-pool-size = 32

# Legacy option (deprecated, use above endpoints instead)
# rpc-endpoint = 0.0.0.0:8090
```

### Command Line Options

```bash
steemd \
  --webserver-http-endpoint=127.0.0.1:8090 \
  --webserver-ws-endpoint=127.0.0.1:8091 \
  --webserver-thread-pool-size=64
```

### Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `webserver-http-endpoint` | (none) | IP:PORT for HTTP requests |
| `webserver-ws-endpoint` | (none) | IP:PORT for WebSocket connections |
| `webserver-thread-pool-size` | 32 | Number of threads for request handling |
| `rpc-endpoint` | (none) | **Deprecated**: Use specific endpoints above |

**Note**: If no endpoints are configured, the webserver plugin loads but doesn't listen on any ports.

## Endpoint Types

### HTTP Endpoint

**Use case**: Standard REST API calls

**Example**:
```bash
curl -X POST http://localhost:8090 \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "database_api.get_dynamic_global_properties",
    "params": {}
  }'
```

**Response**:
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "head_block_number": 12345678,
    "head_block_id": "00bc614e...",
    "time": "2024-01-15T12:00:00",
    ...
  }
}
```

**Characteristics**:
- ✅ Request/response pattern
- ✅ Easy to use (standard HTTP)
- ✅ Works with any HTTP client
- ❌ No server-push notifications
- ❌ Higher overhead per request

### WebSocket Endpoint

**Use case**: Real-time applications, streaming data

**Example** (JavaScript):
```javascript
const ws = new WebSocket('ws://localhost:8091');

ws.onopen = () => {
  ws.send(JSON.stringify({
    jsonrpc: '2.0',
    id: 1,
    method: 'database_api.get_dynamic_global_properties',
    params: {}
  }));
};

ws.onmessage = (event) => {
  const response = JSON.parse(event.data);
  console.log(response.result);
};
```

**Characteristics**:
- ✅ Persistent connection
- ✅ Lower latency
- ✅ Server can push notifications
- ✅ Efficient for multiple requests
- ⚠️ More complex to implement

### Same Endpoint for Both

Both HTTP and WebSocket can use the same port:

```ini
webserver-http-endpoint = 0.0.0.0:8090
webserver-ws-endpoint = 0.0.0.0:8090
```

The server automatically detects the protocol based on the request headers.

## Architecture

### Request Flow

```
External Application
        ↓
HTTP/WebSocket Request (JSON-RPC 2.0)
        ↓
webserver_plugin (port 8090/8091)
        ↓
json_rpc_plugin (parse & route)
        ↓
API Plugin (database_api, etc.)
        ↓
chain_plugin (blockchain data)
        ↓
Response ← ← ← ← ← ←
```

### Thread Model

- **HTTP Thread**: Dedicated thread for HTTP server (`http_ios`)
- **WebSocket Thread**: Dedicated thread for WebSocket server (`ws_ios`)
- **Thread Pool**: Configurable worker threads for processing requests (`thread_pool_ios`)

**Benefit**: HTTP/WebSocket processing doesn't interfere with blockchain validation.

### Implementation Details

**Based on**: [WebSocket++](https://github.com/zaphoyd/websocketpp) library
**Networking**: Boost.Asio for async I/O
**Endpoints**: Optional (only created if configured)

```cpp
// Internal structure (simplified)
fc::optional<tcp::endpoint> http_endpoint;   // HTTP server (if configured)
fc::optional<tcp::endpoint> ws_endpoint;     // WebSocket server (if configured)
boost::thread_group thread_pool;             // Worker threads
```

## Use Cases

### Public API Node

**Configuration**:
```ini
plugin = webserver p2p json_rpc
plugin = database_api network_broadcast_api

webserver-http-endpoint = 0.0.0.0:8090
webserver-ws-endpoint = 0.0.0.0:8091
webserver-thread-pool-size = 256  # High concurrency
```

**Purpose**: Serve many external applications

### Private Witness Node

**Configuration**:
```ini
plugin = webserver witness

webserver-http-endpoint = 127.0.0.1:8090  # Localhost only
webserver-thread-pool-size = 4            # Minimal
```

**Purpose**: Local CLI wallet access only

### Exchange Integration

**Configuration**:
```ini
plugin = webserver account_history_rocksdb
plugin = database_api account_history_api network_broadcast_api

webserver-http-endpoint = 10.0.1.100:8090  # Internal network only
webserver-thread-pool-size = 64
```

**Purpose**: Backend API for exchange platform

## Security Considerations

### Network Exposure

**Public Internet** (0.0.0.0):
```ini
webserver-http-endpoint = 0.0.0.0:8090  # ⚠️ Accessible from anywhere
```
- ✅ Allows public access
- ❌ Vulnerable to abuse/DDoS
- ⚠️ **Requires rate limiting** (use reverse proxy)

**Localhost Only** (127.0.0.1):
```ini
webserver-http-endpoint = 127.0.0.1:8090  # ✅ Localhost only
```
- ✅ Secure by default
- ✅ For local tools (CLI wallet)
- ❌ Not accessible remotely

**Private Network** (10.x.x.x, 192.168.x.x):
```ini
webserver-http-endpoint = 10.0.1.100:8090  # Internal network
```
- ✅ Accessible from internal network
- ✅ Protected by firewall
- ⚠️ Still requires authentication

### Best Practices

1. **Use Reverse Proxy**: NGINX or HAProxy for production
   - Rate limiting
   - TLS/SSL termination
   - Request filtering
   - Load balancing

2. **Firewall Rules**: Restrict access by IP
   ```bash
   # Only allow specific IPs
   ufw allow from 203.0.113.0/24 to any port 8090
   ```

3. **Monitoring**: Track request patterns
   - Unusual request volumes
   - Failed requests
   - Response times

4. **Resource Limits**: Prevent resource exhaustion
   ```ini
   webserver-thread-pool-size = 32  # Limit concurrent requests
   ```

## Performance Tuning

### Thread Pool Sizing

**Low traffic** (< 10 req/s):
```ini
webserver-thread-pool-size = 8
```

**Medium traffic** (10-100 req/s):
```ini
webserver-thread-pool-size = 32  # Default
```

**High traffic** (100+ req/s):
```ini
webserver-thread-pool-size = 128
```

**Formula**: `2 × CPU cores × expected concurrent requests`

**Warning**: Too many threads can cause memory overhead and context switching.

### Operating System Limits

**Linux**: Increase file descriptor limits for WebSocket connections

```bash
# /etc/security/limits.conf
steem soft nofile 65536
steem hard nofile 65536

# Or in systemd service file
LimitNOFILE=65536
```

### Reverse Proxy Configuration

**NGINX Example**:
```nginx
upstream steem_backend {
    server localhost:8090;
    keepalive 32;
}

server {
    listen 80;
    server_name api.example.com;

    location / {
        proxy_pass http://steem_backend;
        proxy_http_version 1.1;
        proxy_set_header Connection "";

        # WebSocket support
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";

        # Rate limiting
        limit_req zone=api_limit burst=20 nodelay;
    }
}

# Define rate limit zone
limit_req_zone $binary_remote_addr zone=api_limit:10m rate=10r/s;
```

See [Reverse Proxy Guide](../operations/reverse-proxy-guide.md) for details.

## Monitoring

### Health Check Endpoint

```bash
curl -s http://localhost:8090 \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "database_api.get_dynamic_global_properties"
  }' | jq -r '.result.head_block_number'
```

**Healthy response**: Returns current block number
**Unhealthy**: Timeout or error

### Metrics to Monitor

- **Active connections**: WebSocket connections count
- **Request rate**: Requests per second
- **Response time**: API call latency (P50, P95, P99)
- **Error rate**: Failed requests percentage
- **Thread pool usage**: Active vs idle threads
- **Memory usage**: Ensure no memory leaks

### Logging

Enable detailed logging for debugging:

```ini
log-logger = {"name":"webserver","level":"debug","appender":"stderr"}
```

**Log events**:
- Server startup/shutdown
- Endpoint binding
- Connection events (WebSocket)
- Request errors

## Troubleshooting

### "Address already in use"

**Problem**: Port 8090 already bound

```
Failed to bind to endpoint: 0.0.0.0:8090
Address already in use
```

**Solution**:
```bash
# Find process using port
lsof -i :8090
# or
netstat -tlnp | grep 8090

# Kill process or use different port
webserver-http-endpoint = 0.0.0.0:8091
```

### No Response / Timeout

**Problem**: Endpoint not configured

```ini
# webserver-http-endpoint not set
```

**Solution**: Configure at least one endpoint
```ini
webserver-http-endpoint = 127.0.0.1:8090
```

### High Memory Usage

**Problem**: Too many threads allocated

```ini
webserver-thread-pool-size = 1024  # ⚠️ Too high
```

**Solution**: Reduce thread count
```ini
webserver-thread-pool-size = 32
```

**Each thread**: ~8MB stack + overhead

### WebSocket Connection Drops

**Problem**: Firewall or proxy timeout

**Solution**: Configure keepalive
```nginx
# NGINX
proxy_read_timeout 3600s;
proxy_send_timeout 3600s;
```

## API Plugin Integration

The webserver plugin enables API plugins. To use any API, the webserver must be enabled.

**Example**: Enable database_api
```ini
plugin = webserver json_rpc database_api

webserver-http-endpoint = 127.0.0.1:8090
```

**Available APIs**:
- [database_api](database_api.md) - Core blockchain queries
- [account_history_api](account_history_api.md) - Transaction history
- [network_broadcast_api](network_broadcast_api.md) - Transaction broadcasting
- [tags_api](tags_api.md) - Content and tags
- [follow_api](follow_api.md) - Social graph
- [market_history_api](market_history_api.md) - Market data
- [witness_api](witness_api.md) - Witness info
- [block_api](block_api.md) - Block data

## Examples

### Minimal Localhost Setup

```ini
# config.ini
plugin = webserver json_rpc database_api

webserver-http-endpoint = 127.0.0.1:8090
webserver-thread-pool-size = 4
```

### Production Public API

```ini
# config.ini
plugin = webserver json_rpc
plugin = database_api network_broadcast_api

# Bind to internal IP (reverse proxy handles public)
webserver-http-endpoint = 127.0.0.1:8090
webserver-ws-endpoint = 127.0.0.1:8091
webserver-thread-pool-size = 128

# Detailed logging
log-logger = {"name":"webserver","level":"info","appender":"stderr"}
```

### Docker Deployment

```bash
docker run -d \
  --name steem-api \
  -p 8090:8090 \
  -p 8091:8091 \
  -v $(pwd)/config.ini:/etc/steemdig.ini:ro \
  -v steem-data:/var/lib/steemd \
  steemit/steem:latest
```

## Related Documentation

- [json_rpc Plugin](json_rpc.md) - JSON-RPC request handling
- [Node Types Guide](../operations/node-types-guide.md) - Pre-configured setups
- [Reverse Proxy Guide](../operations/reverse-proxy-guide.md) - NGINX/HAProxy setup
- [DDoS Protection](../operations/ddos-protection.md) - API protection strategies
- [API Notes](../technical-reference/api-notes.md) - API usage guidelines

## Source Code

- **Header**: [webserver_plugin.hpp](../../src/plugins/webserver/include/steem/plugins/webserver/webserver_plugin.hpp)
- **Implementation**: [webserver_plugin.cpp](../../src/plugins/webserver/webserver_plugin.cpp)
- **Library**: WebSocket++ (vendored in `src/base/fc/vendor/websocketpp`)

## License

See [LICENSE.md](../../LICENSE.md)
