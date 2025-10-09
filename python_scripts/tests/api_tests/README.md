# API Tests

Automated testing framework for Steem JSON-RPC APIs using `pyresttest`.

## Overview

This directory contains declarative API tests that:
- Validate API response structure and content
- Compare responses against expected patterns
- Test multiple API endpoints systematically
- Support both exact match and containment validation
- Generate output files for debugging failed tests

## Directory Structure

```
api_tests/
├── run_api_tests.sh           # Main test runner script
├── basic_smoketest.yaml        # Initial connectivity test
├── validator_ex.py             # Custom validator extensions
├── jsonsocket.py               # JSON-RPC socket utility
├── comparator_*.py             # Comparison utilities
├── database_api/               # Database API tests
│   ├── database_api_test.yaml
│   ├── request_template.json
│   └── *.json.pat              # Expected response patterns
├── account_history_api/        # Account History API tests
│   ├── account_history_api_test.yaml
│   └── *.json.pat
└── compare_state/              # State comparison utilities
```

## Quick Start

### Prerequisites

```bash
# Install pyresttest
pip install pyresttest

# Install Python dependencies
pip install past six

# Build and run steemd with APIs enabled
./steemd --plugin="database_api account_history_api" \
         --webserver-http-endpoint=127.0.0.1:8090
```

### Running Tests

```bash
# Run with exact match comparison
cd python_scripts/tests/api_tests
./run_api_tests.sh equal

# Run with containment comparison
./run_api_tests.sh contain
```

## Test Framework

### pyresttest

The framework uses [pyresttest](https://github.com/svanoort/pyresttest) for declarative HTTP API testing.

**Features**:
- YAML-based test definitions
- JSONPath extraction and validation
- Template-based request generation
- Custom validator extensions
- Test result reporting

### Comparators

Two comparison modes available:

| Comparator | Module | Behavior |
|------------|--------|----------|
| `equal` | `comparator_equal.py` | Exact JSON match |
| `contain` | `comparator_contain.py` | Pattern containment |

## Test Configuration

### YAML Structure

```yaml
---
- config:
  - testset: "API Test Suite Name"
  - api: &api "api_name"
  - variable_binds:
    - api: *api

- base_test: &base_test
  - url: "/rpc"
  - method: "POST"
  - body: {template: {file: "request_template.json"}}
  - validators:
    - extract_test: {jsonpath_mini: "error", test: "not_exists"}
    - extract_test: {jsonpath_mini: "result", test: "exists"}

- test:
  - <<: *base_test
  - name: "test_method_name"
  - validators:
    - ComparatorValidator:
      - extractor: {jsonpath_mini: "result"}
      - expected: "get_method_name"
```

### Request Template

```json
{
  "jsonrpc": "2.0",
  "id": $test_id,
  "method": "$api.$method",
  "params": {}
}
```

### Expected Pattern Files

Pattern files (`.json.pat`) contain expected API responses:

```json
{
  "head_block_number": 12345,
  "head_block_id": "00003039abcd...",
  "time": "2023-01-01T00:00:00"
}
```

**Note**: When using `contain` comparator, only specified fields are checked.

## API Test Suites

### Database API Tests

**Location**: [`database_api/`](database_api/)

**Methods Tested**:
- `get_config` - Node configuration
- `get_dynamic_global_properties` - Current blockchain state
- `get_witness_schedule` - Witness rotation schedule
- `get_hardfork_properties` - Hardfork version info
- `get_current_price_feed` - STEEM/USD price feed
- `get_feed_history` - Historical price feeds
- `get_reward_funds` - Reward pool state
- `list_witnesses` - Active witness list

**Example Test**:
```yaml
- test:
  - name: "get_dynamic_global_properties"
  - validators:
    - ComparatorValidator:
      - extractor: {jsonpath_mini: "result"}
      - expected: "get_dynamic_global_properties"
```

**Pattern File**: `get_dynamic_global_properties.json.pat`
```json
{
  "head_block_number": 50000000,
  "current_witness": "witness-name",
  "total_vesting_shares": "400000000.000000 VESTS"
}
```

### Account History API Tests

**Location**: [`account_history_api/`](account_history_api/)

**Methods Tested**:
- `get_account_history` - Account operation history
- `get_ops_in_block` - Operations in specific block

**Example**:
```yaml
- test:
  - name: "get_account_history"
  - body:
      jsonrpc: "2.0"
      id: 1
      method: "account_history_api.get_account_history"
      params:
        account: "steemit"
        start: -1
        limit: 10
```

### Compare State Tests

**Location**: [`compare_state/`](compare_state/)

Utilities for comparing blockchain state between different node implementations or versions.

## Custom Validators

### JSONFileValidator

Defined in [`validator_ex.py`](validator_ex.py).

**Purpose**: Compare API responses against pattern files.

**Features**:
- Loads expected patterns from `.json.pat` files
- Supports custom comparators (exact, contain, appbase ops)
- Dumps failed responses to `.json.out` files
- Provides detailed error messages

**Usage in YAML**:
```yaml
- validators:
  - ComparatorValidator:
    - extractor: {jsonpath_mini: "result"}
    - expected: "method_name"  # Loads method_name.json.pat
```

### Comparator Functions

**`comparator_equal.py`**:
```python
def equal(x, y):
    """Exact JSON equality"""
    return x == y
```

**`comparator_contain.py`**:
```python
def contain(x, y):
    """Check if x contains all fields from y"""
    # Recursive containment check
    return all_fields_present(x, y)
```

**`comparator_appbase_ops.py`**:
```python
def appbase_ops(x, y):
    """Compare operation structures in AppBase format"""
    # Handle operation array comparison
```

## Writing New Tests

### 1. Create Test YAML

```yaml
# my_api/my_api_test.yaml
---
- config:
  - testset: "My API Tests"
  - api: &api "my_api"

- base_test: &base_test
  - url: "/rpc"
  - method: "POST"
  - validators:
    - extract_test: {jsonpath_mini: "error", test: "not_exists"}

- test:
  - <<: *base_test
  - name: "my_method"
  - body:
      jsonrpc: "2.0"
      id: 1
      method: "my_api.my_method"
      params: {}
  - validators:
    - ComparatorValidator:
      - extractor: {jsonpath_mini: "result"}
      - expected: "my_method"
```

### 2. Create Pattern File

```bash
# my_api/my_method.json.pat
{
  "expected_field": "expected_value",
  "another_field": 12345
}
```

### 3. Update Test Runner

```bash
# Add to run_api_tests.sh
pyresttest $NODE:$RPC_PORT ./my_api/my_api_test.yaml \
    --import_extensions='validator_ex;'$COMPARATOR
[ $? -ne 0 ] && EXIT_CODE=-1
```

### 4. Run Tests

```bash
./run_api_tests.sh equal
```

## Debugging Failed Tests

### Output Files

When a test fails, the actual response is saved:
```
method_name.json.out  # Actual API response
```

Compare with expected:
```bash
diff method_name.json.pat method_name.json.out
```

### Verbose Mode

Enable pyresttest verbose output:
```bash
# Edit run_api_tests.sh
pyresttest $NODE:$RPC_PORT ./database_api/database_api_test.yaml \
    --import_extensions='validator_ex;'$COMPARATOR \
    --log=debug
```

### Manual Testing

Test individual API calls:
```bash
curl -X POST http://127.0.0.1:8090/rpc \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "database_api.get_config",
    "params": {}
  }' | jq .
```

### Using JSONSocket

```python
from jsonsocket import JSONSocket

sock = JSONSocket("127.0.0.1", 8090, "/rpc")
response = sock.send_request({
    "jsonrpc": "2.0",
    "id": 1,
    "method": "database_api.get_config",
    "params": {}
})
print(response)
```

## Test Scripts

### run_api_tests.sh

Main test orchestration script.

**Arguments**:
- `equal` - Use exact match comparator
- `contain` - Use containment comparator

**Environment Variables**:
```bash
NODE='http://127.0.0.1'  # Node address
RPC_PORT=8090             # RPC port
```

**Exit Codes**:
- `0` - All tests passed
- `-1` - One or more tests failed

### Individual Test Scripts

**`test_ah_get_account_history.py`**:
- Tests `get_account_history` with various parameters
- Validates pagination and filtering

**`test_ah_get_ops_in_block.py`**:
- Tests `get_ops_in_block` for specific blocks
- Verifies operation extraction

**`compare_state_history.sh`**:
- Compares state between nodes
- Useful for verifying synchronization

## Pattern File Generation

### Generate from Live Node

```bash
# Fetch and save response as pattern
curl -X POST http://127.0.0.1:8090/rpc \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "database_api.get_config",
    "params": {}
  }' | jq '.result' > get_config.json.pat
```

### Update Existing Patterns

```python
#!/usr/bin/env python3
import json
from jsonsocket import JSONSocket

sock = JSONSocket("127.0.0.1", 8090, "/rpc")

methods = [
    "get_config",
    "get_dynamic_global_properties",
    "get_witness_schedule"
]

for method in methods:
    response = sock.send_request({
        "jsonrpc": "2.0",
        "id": 1,
        "method": f"database_api.{method}",
        "params": {}
    })

    with open(f"{method}.json.pat", "w") as f:
        json.dump(response["result"], f, indent=2)
```

## Best Practices

1. **Use Containment for Dynamic Data**:
   ```yaml
   # Use 'contain' comparator for fields that change
   ./run_api_tests.sh contain
   ```

2. **Exact Match for Static Config**:
   ```yaml
   # Use 'equal' for stable config values
   ./run_api_tests.sh equal
   ```

3. **Minimal Patterns**:
   Only include fields you want to validate. Remove timestamps, block numbers, etc.

4. **Test Against Testnet**:
   Use testnet for consistent, reproducible results.

5. **Version Pattern Files**:
   Update patterns when API changes are intentional.

## Continuous Integration

### Example CI Configuration

```yaml
# .github/workflows/api_tests.yml
name: API Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2

      - name: Build steemd
        run: |
          cmake -DCMAKE_BUILD_TYPE=Release ..
          make -j$(nproc) steemd

      - name: Start steemd
        run: |
          ./steemd --data-dir=test_data &
          sleep 30

      - name: Install dependencies
        run: pip install pyresttest past six

      - name: Run API tests
        run: |
          cd python_scripts/tests/api_tests
          ./run_api_tests.sh contain
```

## Utilities

### list_account.py

List accounts from blockchain:
```bash
python list_account.py --node=127.0.0.1:8090 --limit=100
```

### jsonsocket.py

Low-level JSON-RPC socket communication:
```python
from jsonsocket import JSONSocket

sock = JSONSocket("127.0.0.1", 8090, "/rpc")
result = sock.send_request({...})
```

## Troubleshooting

### steemd Not Running

```
FATAL: steemd not running?
```

**Solution**: Start steemd with API plugins:
```bash
./steemd --plugin="database_api account_history_api" \
         --webserver-http-endpoint=127.0.0.1:8090
```

### Import Error

```
ImportError: No module named 'pyresttest'
```

**Solution**: Install pyresttest:
```bash
pip install pyresttest
```

### Validator Extension Not Found

```
Error: Cannot import validator_ex
```

**Solution**: Run from `api_tests/` directory:
```bash
cd python_scripts/tests/api_tests
./run_api_tests.sh equal
```

### Pattern File Not Found

```
FileNotFoundError: method_name.json.pat
```

**Solution**: Create pattern file or generate from live node.

### All Tests Failing

Check node connectivity:
```bash
curl http://127.0.0.1:8090
```

Verify RPC endpoint:
```bash
curl -X POST http://127.0.0.1:8090/rpc \
  -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_config","params":{}}'
```

## Additional Resources

- [pyresttest Documentation](https://github.com/svanoort/pyresttest)
- [JSONPath Documentation](https://goessner.net/articles/JsonPath/)
- [Steem API Documentation](../../docs/apis.md)
- [Database API Plugin](../../libraries/plugins/database_api/README.md)
- [Account History API Plugin](../../libraries/plugins/account_history_api/README.md)

## License

See [LICENSE](../../../LICENSE.md)
