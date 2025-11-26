# json_rpc Plugin

JSON-RPC 2.0 request handling and method routing for Steem API plugins.

## Overview

The `json_rpc` plugin provides a central registry for all API methods and handles JSON-RPC 2.0 protocol compliance. It routes incoming RPC requests to appropriate API plugins, manages request/response formatting, and provides error handling according to JSON-RPC 2.0 specification.

**Plugin Type**: Infrastructure (Required for APIs)
**Dependencies**: None (base plugin)
**Memory**: Minimal (~5MB)
**Default**: Enabled automatically when API plugins are used

## Purpose

- **API Registration**: Central registry for all API methods from all plugins
- **Request Routing**: Route JSON-RPC calls to appropriate API handlers
- **Protocol Compliance**: Enforce JSON-RPC 2.0 specification
- **Error Handling**: Standardized error codes and messages
- **Batch Requests**: Support for batch RPC calls
- **Method Discovery**: List available methods and signatures
- **Request Logging**: Optional logging for testing and debugging

## Configuration

### Config File Options

```ini
# json_rpc plugin enabled automatically with API plugins
# No explicit enabling needed

# Optional: Enable JSON-RPC request/response logging
# log-json-rpc = /path/to/log/directory
```

### Command Line Options

```bash
steemd \
  --log-json-rpc=/var/log/steemd/json-rpc
```

### Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `log-json-rpc` | (disabled) | Directory path for JSON-RPC logging (creates YAML test files) |

## JSON-RPC 2.0 Protocol

### Request Format

**Single Request**:
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "database_api.get_dynamic_global_properties",
  "params": {}
}
```

**Batch Request**:
```json
[
  {
    "jsonrpc": "2.0",
    "id": 1,
    "method": "block_api.get_block",
    "params": {"block_num": 12345}
  },
  {
    "jsonrpc": "2.0",
    "id": 2,
    "method": "database_api.find_accounts",
    "params": {"accounts": ["alice", "bob"]}
  }
]
```

### Response Format

**Success Response**:
```json
{
  "jsonrpc": "2.0",
  "result": {
    "head_block_number": 12345678,
    "head_block_id": "00bc614e...",
    "time": "2023-01-15T10:30:00"
  },
  "id": 1
}
```

**Error Response**:
```json
{
  "jsonrpc": "2.0",
  "error": {
    "code": -32601,
    "message": "Method not found",
    "data": {
      "api": "invalid_api",
      "method": "invalid_method"
    }
  },
  "id": 1
}
```

**Batch Response**:
```json
[
  {
    "jsonrpc": "2.0",
    "result": { "previous": "...", "timestamp": "..." },
    "id": 1
  },
  {
    "jsonrpc": "2.0",
    "result": [
      { "name": "alice", "balance": "100.000 STEEM" },
      { "name": "bob", "balance": "200.000 STEEM" }
    ],
    "id": 2
  }
]
```

## Error Codes

### Standard JSON-RPC Errors

| Code | Message | Description |
|------|---------|-------------|
| -32700 | Parse error | Invalid JSON received |
| -32600 | Invalid Request | JSON-RPC format violation |
| -32601 | Method not found | Method doesn't exist |
| -32602 | Invalid params | Invalid method parameters |
| -32603 | Internal error | Internal JSON-RPC error |

### Steem-Specific Errors

| Code | Message | Description |
|------|---------|-------------|
| -32000 | Server error | General server error |
| -32001 | No params | Required params missing |
| -32002 | Parse params error | Can't parse parameters |
| -32003 | Error during call | Exception during method execution |

### Error Examples

**Method Not Found**:
```json
{
  "jsonrpc": "2.0",
  "error": {
    "code": -32601,
    "message": "Could not find API database_api",
    "data": {
      "api": "database_api"
    }
  },
  "id": 1
}
```

**Invalid Parameters**:
```json
{
  "jsonrpc": "2.0",
  "error": {
    "code": -32002,
    "message": "Parse params error: Expected object",
    "data": "fc::parse_error_exception: ..."
  },
  "id": 1
}
```

**Execution Error**:
```json
{
  "jsonrpc": "2.0",
  "error": {
    "code": -32003,
    "message": "Error during call: Account not found",
    "data": {
      "code": 10,
      "name": "assert_exception",
      "message": "Account 'nonexistent' not found"
    }
  },
  "id": 1
}
```

## Method Naming

### Convention

Methods follow the pattern: `api_name.method_name`

**Examples**:
```
database_api.find_accounts
account_history_api.get_account_history
block_api.get_block_header
```

### Legacy "call" Method

Backwards compatibility with old format:

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "call",
  "params": ["database_api", "get_dynamic_global_properties", {}]
}
```

**Modern equivalent**:
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "database_api.get_dynamic_global_properties",
  "params": {}
}
```

**Recommendation**: Use modern `api.method` format for new applications.

## Built-In Methods

### jsonrpc.get_methods

List all available API methods:

**Request**:
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "jsonrpc.get_methods",
  "params": {}
}
```

**Response**:
```json
{
  "jsonrpc": "2.0",
  "result": [
    "database_api.find_accounts",
    "database_api.get_dynamic_global_properties",
    "block_api.get_block_header",
    "jsonrpc.get_methods",
    "jsonrpc.get_signature"
  ],
  "id": 1
}
```

### jsonrpc.get_signature

Get method signature (argument and return types):

**Request**:
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "jsonrpc.get_signature",
  "params": {
    "method": "block_api.get_block"
  }
}
```

**Response**:
```json
{
  "jsonrpc": "2.0",
  "result": {
    "args": {
      "block_num": 0
    },
    "ret": {
      "previous": "",
      "timestamp": "1970-01-01T00:00:00",
      "witness": "",
      "transactions": []
    }
  },
  "id": 1
}
```

## API Registration

### Plugin Registration

API plugins register their methods during initialization:

```cpp
// In your API plugin
void my_api_plugin::plugin_initialize(const variables_map& options)
{
    // Register methods with json_rpc plugin
    JSON_RPC_REGISTER_API(name());
}
```

### Method Registration

Individual methods registered via macro:

```cpp
DECLARE_API(
    (get_account)
    (get_accounts)
    (get_block)
    (get_block_header)
)
```

### Registration Flow

```
API Plugin Initialize
        ↓
JSON_RPC_REGISTER_API macro
        ↓
For each method:
    ↓
    Register with json_rpc_plugin
    ↓
    Add to method registry
    ↓
    Add to signature registry
        ↓
Methods available for RPC calls
```

## Request Processing

### Processing Pipeline

```
HTTP/WebSocket receives JSON
        ↓
Parse JSON to variant
        ↓
Validate JSON-RPC 2.0 format
        ↓
Extract method name
        ↓
Look up method in registry
        ↓
Parse parameters
        ↓
Execute method
        ↓
Format response
        ↓
Return JSON-RPC response
```

### Batch Processing

```
Receive batch array
        ↓
Validate array not empty
        ↓
For each request:
    ↓
    Process individually
    ↓
    Collect response
        ↓
Return response array
```

### Error Handling

```
Try to parse JSON
├─ Success → Process request
└─ Failure → Return parse error (-32700)

Try to validate JSON-RPC
├─ Success → Continue
└─ Failure → Return invalid request (-32600)

Try to find method
├─ Success → Continue
└─ Failure → Return method not found (-32601)

Try to parse params
├─ Success → Continue
└─ Failure → Return invalid params (-32602)

Try to execute method
├─ Success → Return result
└─ Failure → Return error (-32003)
```

## Request Logging

### Enabling Logging

```ini
log-json-rpc = /var/log/steemd/jsonrpc
```

Creates directory structure:
```
/var/log/steemd/jsonrpc/
├── 1.json          # Request
├── 1.json.pat      # Response
├── 2.json
├── 2.json.pat
├── 1_error.json    # Failed request
├── 1_error.json.pat  # Error response
└── tests.yaml      # PyRestTest format
```

### Log Format

**Request file** (`1.json`):
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "block_api.get_block",
  "params": {"block_num": 12345}
}
```

**Response file** (`1.json.pat`):
```json
{
  "previous": "00003038...",
  "timestamp": "2016-03-24T16:05:00",
  "witness": "alice",
  "transactions": []
}
```

**Tests file** (`tests.yaml`):
```yaml
---
- config:
  - testset: "API Tests"

- test:
  - body: {file: "1.json"}
  - name: "test1"
  - url: "/rpc"
  - method: "POST"
  - validators:
    - extract_test: {jsonpath_mini: "error", test: "not_exists"}
    - json_file_validator: {jsonpath_mini: "result", expected: {template: '1'}}
```

### Use Cases for Logging

**API Testing**: Generate test suites from live requests

**Debugging**: Capture problem requests for analysis

**Performance Analysis**: Identify slow methods

**Regression Testing**: Replay logged requests

## Performance Considerations

### Method Lookup

Method lookup is O(log n) using std::map:
- Fast even with hundreds of methods
- No performance concern

### Parameter Parsing

JSON parsing overhead:
- Small params: ~10μs
- Large params: ~100μs
- Batch requests: Linear in batch size

### Batch Optimization

Batch requests processed sequentially:
- No parallelization
- Use for logical grouping, not performance
- Each request holds read lock independently

### Memory Usage

Minimal memory overhead:
- Method registry: ~1KB per method
- Active requests: ~1KB per request
- Logging: Disk I/O only

## Monitoring

### Method Usage

Track which methods are called:

```bash
# Enable logging
log-json-rpc = /var/log/steemd/jsonrpc

# Count method calls
ls /var/log/steemd/jsonrpc/*.json | wc -l

# Most common methods
cat /var/log/steemd/jsonrpc/*.json | \
  jq -r '.method' | \
  sort | uniq -c | sort -rn
```

### Error Tracking

Monitor error rates:

```bash
# Count errors
ls /var/log/steemd/jsonrpc/*_error.json | wc -l

# Error types
cat /var/log/steemd/jsonrpc/*_error.json.pat | \
  jq -r '.code' | \
  sort | uniq -c
```

### Performance Monitoring

Check method execution time (requires application logging):

```bash
# Watch for slow methods in logs
tail -f witness_node_data_dir/logs/stderr.txt | \
  grep -i "slow\|timeout\|lock"
```

## Troubleshooting

### Method Not Found

**Problem**: API method returns "method not found" error

**Symptoms**:
```json
{
  "error": {
    "code": -32601,
    "message": "Could not find API database_api"
  }
}
```

**Solutions**:
1. Verify plugin enabled:
   ```ini
   plugin = database_api
   ```

2. Check method name format:
   ```
   Correct: database_api.get_dynamic_global_properties
   Wrong: get_dynamic_global_properties
   Wrong: database_api_get_dynamic_global_properties
   ```

3. List available methods:
   ```bash
   curl -s http://localhost:8090 \
     -d '{"jsonrpc":"2.0","id":1,"method":"jsonrpc.get_methods","params":{}}'
   ```

### Invalid Parameters

**Problem**: Method exists but params rejected

**Symptoms**:
```json
{
  "error": {
    "code": -32002,
    "message": "Parse params error"
  }
}
```

**Solutions**:
1. Check parameter format:
   ```json
   # Array params
   {"params": [12345]}

   # Object params
   {"params": {"block_num": 12345}}
   ```

2. Get method signature:
   ```bash
   curl -s http://localhost:8090 \
     -d '{"jsonrpc":"2.0","id":1,"method":"jsonrpc.get_signature","params":{"method":"block_api.get_block"}}'
   ```

3. Verify parameter types:
   ```
   String: "alice"
   Number: 12345
   Boolean: true
   Array: [1,2,3]
   Object: {"key": "value"}
   ```

### Batch Request Failures

**Problem**: Batch request returns error

**Symptoms**:
```json
{
  "error": {
    "code": -32000,
    "message": "Array is invalid"
  }
}
```

**Solutions**:
1. Ensure array not empty:
   ```json
   # Wrong
   []

   # Correct
   [{"jsonrpc":"2.0","id":1,"method":"...","params":{}}]
   ```

2. Validate each request:
   ```json
   # Each element must be valid JSON-RPC request
   [
     {"jsonrpc":"2.0","id":1,"method":"...","params":{}},
     {"jsonrpc":"2.0","id":2,"method":"...","params":{}}
   ]
   ```

## Use Cases

### API Node

Full RPC API access:

```ini
plugin = chain p2p webserver json_rpc
plugin = database_api block_api

webserver-http-endpoint = 0.0.0.0:8090
webserver-ws-endpoint = 0.0.0.0:8091
```

### Development Node

With request logging:

```ini
plugin = chain p2p webserver json_rpc
plugin = database_api

log-json-rpc = /tmp/jsonrpc-tests

# Generates test files for CI/CD
```

### Production Node

Minimal configuration:

```ini
plugin = chain p2p webserver json_rpc
plugin = database_api

# No logging (performance)
```

## Security Considerations

### Input Validation

All inputs validated:
- JSON syntax checked
- JSON-RPC format enforced
- Parameters type-checked
- No SQL injection risk (no SQL)

### DoS Protection

Rate limiting recommended:

```nginx
# NGINX rate limiting
limit_req_zone $binary_remote_addr zone=rpc:10m rate=10r/s;

location /rpc {
    limit_req zone=rpc burst=20;
    proxy_pass http://localhost:8090;
}
```

### Error Information Disclosure

Error messages can reveal information:
- Method names exposed via get_methods
- Parameter structures via get_signature
- Internal errors via exception messages

**Mitigation**: Use reverse proxy to filter errors in production.

## Best Practices

### Client Implementation

**1. Use modern method format**:
```javascript
// Good
{
  "method": "block_api.get_block",
  "params": {"block_num": 12345}
}

// Avoid (legacy)
{
  "method": "call",
  "params": ["block_api", "get_block", [{"block_num": 12345}]]
}
```

**2. Always include ID**:
```javascript
{
  "id": generateUniqueId(),  // Required for matching responses
  "method": "...",
  "params": {}
}
```

**3. Handle errors**:
```javascript
if (response.error) {
  console.error(`RPC Error ${response.error.code}: ${response.error.message}`);
  // Handle error
} else {
  // Process response.result
}
```

**4. Use batch for multiple calls**:
```javascript
// Batch multiple independent requests
[
  {"jsonrpc":"2.0","id":1,"method":"get_block","params":[100]},
  {"jsonrpc":"2.0","id":2,"method":"get_block","params":[101]},
  {"jsonrpc":"2.0","id":3,"method":"get_block","params":[102]}
]
```

### Server Configuration

**1. Enable only needed APIs**:
```ini
# Only enable required plugins
plugin =  # Not all APIs
```

**2. Use reverse proxy**:
```
Client → NGINX → Steemd
         ↑
    Rate limiting
    Error filtering
    SSL termination
```

**3. Monitor usage**:
```ini
# Enable logging in development
log-json-rpc = /var/log/jsonrpc

# Disable in production (performance)
```

## Related Documentation

- [webserver Plugin](webserver.md) - HTTP/WebSocket transport
- [database_api Plugin](database_api.md) - Blockchain query API
- [Reverse Proxy Guide](../operations/reverse-proxy-guide.md) - Production deployment

## Source Code

- **Implementation**: [libraries/plugins/json_rpc/json_rpc_plugin.cpp](../../libraries/plugins/json_rpc/json_rpc_plugin.cpp)
- **Header**: [libraries/plugins/json_rpc/include/steem/plugins/json_rpc/json_rpc_plugin.hpp](../../libraries/plugins/json_rpc/include/steem/plugins/json_rpc/json_rpc_plugin.hpp)
- **Utilities**: [libraries/plugins/json_rpc/include/steem/plugins/json_rpc/utility.hpp](../../libraries/plugins/json_rpc/include/steem/plugins/json_rpc/utility.hpp)

## License

See [LICENSE.md](../../LICENSE.md)
