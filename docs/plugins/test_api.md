# test_api Plugin

Testing and development utilities for the Steem node API infrastructure.

## Overview

The `test_api` plugin provides minimal test endpoints for verifying API functionality and testing the JSON-RPC infrastructure. This plugin is primarily used for development and testing purposes, not production deployments.

**Plugin Type**: API Plugin (Test/Development)
**Dependencies**: [json_rpc](json_rpc.md)
**Memory**: Minimal
**Default**: Disabled

## Purpose

- **API Testing**: Verify JSON-RPC infrastructure is working
- **Development**: Test plugin system and API registration
- **Integration Testing**: Validate client-server communication
- **Example Plugin**: Reference implementation for plugin developers

## Configuration

### Enable Plugin

```ini
# In config.ini
plugin = test_api
```

Or via command line:

```bash
steemd --plugin=test_api
```

### Prerequisites

Only requires the JSON-RPC plugin:

```ini
plugin = json_rpc
plugin = webserver
plugin = test_api
```

### No Configuration Options

This plugin has no configurable parameters. It provides fixed test endpoints.

## API Methods

All methods are called via JSON-RPC at the endpoint configured by the `webserver` plugin (default: `http://localhost:8090`).

### test_api_a

Simple test endpoint that returns a fixed response.

**Request Parameters**: None (empty object)

**Returns**: Object with `value` field

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"test_api.test_api_a",
  "params":{}
}'
```

**Response**:
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "value": "A"
  }
}
```

### test_api_b

Another simple test endpoint with different response.

**Request Parameters**: None (empty object)

**Returns**: Object with `value` field

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"test_api.test_api_b",
  "params":{}
}'
```

**Response**:
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "value": "B"
  }
}
```

## Use Cases

### API Infrastructure Testing

Verify that the API server is running and responding:

```bash
# Quick health check
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"test_api.test_api_a",
  "params":{}
}' | jq -r '.result.value'
# Expected: "A"
```

### Integration Testing

Test client libraries and API communication:

```javascript
// JavaScript/Node.js example
const axios = require('axios');

async function testAPIConnection() {
  try {
    const response = await axios.post('http://localhost:8090', {
      jsonrpc: '2.0',
      id: 1,
      method: 'test_api.test_api_a',
      params: {}
    });

    if (response.data.result.value === 'A') {
      console.log('API connection successful');
      return true;
    }
  } catch (error) {
    console.error('API connection failed:', error.message);
    return false;
  }
}
```

### Plugin Development Reference

The `test_api` plugin serves as a minimal example for developers creating new API plugins:

**Key Implementation Patterns**:
1. Plugin registration with appbase
2. API method declaration using `DECLARE_API` macro
3. JSON-RPC integration
4. Request/response structure definition
5. FC_REFLECT macros for serialization

**Source Reference**:
```cpp
// From test_api_plugin.hpp
class test_api_plugin : public appbase::plugin< test_api_plugin >
{
   APPBASE_PLUGIN_REQUIRES( (plugins::json_rpc::json_rpc_plugin) );

   DECLARE_API(
      (test_api_a)
      (test_api_b)
   )
};

// Argument and return structures
struct test_api_a_args {};
struct test_api_a_return { std::string value; };

FC_REFLECT( test_api_a_args, )
FC_REFLECT( test_api_a_return, (value) )
```

### Automated Health Checks

Monitor node availability:

```bash
#!/bin/bash
# Simple node health check script

check_api() {
  response=$(curl -s http://localhost:8090 -d '{
    "jsonrpc":"2.0",
    "id":1,
    "method":"test_api.test_api_a",
    "params":{}
  }')

  if echo "$response" | grep -q '"value":"A"'; then
    echo "Node API: OK"
    return 0
  else
    echo "Node API: FAILED"
    return 1
  fi
}

check_api
```

### Performance Baseline

Establish baseline for API response times:

```javascript
async function measureAPILatency(iterations = 100) {
  const latencies = [];

  for (let i = 0; i < iterations; i++) {
    const start = Date.now();
    await fetch('http://localhost:8090', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({
        jsonrpc: '2.0',
        id: 1,
        method: 'test_api.test_api_a',
        params: {}
      })
    });
    latencies.push(Date.now() - start);
  }

  const avg = latencies.reduce((a, b) => a + b) / latencies.length;
  const min = Math.min(...latencies);
  const max = Math.max(...latencies);

  console.log(`API Latency - Avg: ${avg}ms, Min: ${min}ms, Max: ${max}ms`);
}
```

## Development Notes

### Minimal Plugin Design

The `test_api` plugin demonstrates minimal plugin architecture:
- No state tracking
- No database access
- No dependencies on chain data
- Stateless endpoints

### Plugin Lifecycle

Like all appbase plugins, `test_api` follows the standard lifecycle:

1. **initialize** - Plugin registration
2. **startup** - API endpoints registered
3. **running** - Handles requests
4. **shutdown** - Cleanup (none needed for test_api)

### Thread Safety

API methods are called from the webserver thread pool. The test API:
- Maintains no state (thread-safe by design)
- Returns static responses
- Requires no locking

### Adding Custom Test Endpoints

Developers can modify `test_api` to add custom test functionality:

```cpp
// Add new test method
DECLARE_API(
   (test_api_a)
   (test_api_b)
   (test_api_custom)  // New method
)

// Define structures
struct test_api_custom_args {
   std::string input;
};

struct test_api_custom_return {
   std::string output;
};

// Implement method
DEFINE_API_IMPL( test_api_plugin, test_api_custom )
{
   test_api_custom_return result;
   result.output = "Processed: " + args.input;
   return result;
}
```

## Not for Production

### Important Warnings

**Do NOT enable `test_api` in production nodes**:

1. **No utility**: Provides no functional value for production
2. **Attack surface**: Exposes unnecessary endpoints
3. **Resource waste**: Consumes memory and processing time
4. **Security**: May expose internal implementation details

### When to Enable

Enable `test_api` only in:
- **Development environments**: Local testing
- **Staging servers**: Pre-production validation
- **Integration testing**: CI/CD pipelines
- **Plugin development**: Building new API plugins

### Production Checklist

Before deploying to production, verify `test_api` is disabled:

```bash
# Check config file
grep "test_api" witness_node_data_dir/config.ini
# Should return nothing or commented line

# Check running plugins
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"test_api.test_api_a",
  "params":{}
}'
# Should return "Method not found" error
```

## Troubleshooting

### Method Not Found

**Problem**: `test_api` methods return "method not found" error

**Cause**: Plugin not enabled

**Solution**:
```bash
# Add to config.ini
echo "plugin = test_api" >> witness_node_data_dir/config.ini

# Restart node
```

### No Response

**Problem**: API calls hang or timeout

**Cause**: Webserver plugin not configured

**Solution**:
```ini
# Ensure webserver is enabled and configured
plugin = webserver
webserver-http-endpoint = 0.0.0.0:8090
webserver-ws-endpoint = 0.0.0.0:8091

plugin = json_rpc
plugin = test_api
```

## Comparison with Other APIs

### vs. database_api

| Feature | test_api | database_api |
|---------|----------|--------------|
| Purpose | Testing | Blockchain queries |
| State | Stateless | Reads chain state |
| Complexity | Minimal | Complex |
| Production | No | Yes |

### vs. Production APIs

| Feature | test_api | database_api |
|---------|----------|---------------|
| Purpose | Testing | Blockchain queries |
| Dependencies | json_rpc only | Multiple plugins |
| Methods | 2 test methods | 50+ production methods |
| Use case | Development | Production apps |

## Related Documentation

- [json_rpc Plugin](json_rpc.md) - JSON-RPC infrastructure
- [webserver Plugin](webserver.md) - HTTP/WebSocket server
- [Plugin Development](../development/plugin.md) - Creating custom plugins
- [Testing Guide](../development/testing.md) - Testing strategies

## Source Code

- **Plugin Implementation**: [libraries/plugins/apis/test_api/test_api_plugin.cpp](../../libraries/plugins/apis/test_api/test_api_plugin.cpp)
- **Plugin Header**: [libraries/plugins/apis/test_api/include/steem/plugins/test_api/test_api_plugin.hpp](../../libraries/plugins/apis/test_api/include/steem/plugins/test_api/test_api_plugin.hpp)

## License

See [LICENSE.md](../../LICENSE.md)
