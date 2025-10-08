# Plugin Tests

Plugin-specific functionality test suite (plugin_test).

## Overview

C++ unit tests for Steem plugin functionality using Boost.Test framework. Tests validate plugin behavior, API responses, and plugin integration with the core blockchain.

**Test Executable**: `plugin_test`

## Test Files

| Test File | Purpose |
|-----------|---------|
| **[main.cpp](main.cpp)** | Test suite initialization with genesis timestamp support |
| **[json_rpc.cpp](json_rpc.cpp)** | JSON-RPC protocol validation |
| **[market_history.cpp](market_history.cpp)** | Market history plugin functionality |
| **[plugin_ops.cpp](plugin_ops.cpp)** | Plugin operation handling |
| **[smt_market_history.cpp](smt_market_history.cpp)** | SMT market history functionality |

## Building

```bash
# From build directory
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_STEEM_TESTNET=ON \
      ..
make -j$(nproc) plugin_test
```

## Running Tests

### All Tests

```bash
# Run all plugin tests
./tests/plugin_test
```

### Specific Test Suite

```bash
# JSON-RPC tests
./tests/plugin_test -t json_rpc

# Market history tests
./tests/plugin_test -t market_history

# List available tests
./tests/plugin_test --list_content
```

### With Environment Variables

```bash
# Set genesis timestamp
export STEEM_TESTING_GENESIS_TIMESTAMP=1234567890
./tests/plugin_test
```

## Test Suites

### JSON-RPC Tests

**File**: [json_rpc.cpp](json_rpc.cpp)

**Purpose**: Validate JSON-RPC 2.0 protocol compliance

**Coverage**:
- Request validation
- Error code compliance
- Parameter validation
- Method routing
- Response formatting

**Standard Error Codes** (from JSON-RPC 2.0 spec):
- `-32700` - Parse error (Invalid JSON)
- `-32600` - Invalid Request
- `-32601` - Method not found
- `-32602` - Invalid params
- `-32603` - Internal error
- `-32000` to `-32099` - Server error

**Example Test**:
```cpp
BOOST_AUTO_TEST_CASE( basic_validation )
{
    std::string request;

    // Missing jsonrpc field
    request = "{}";
    make_request( request, JSON_RPC_INVALID_REQUEST );

    // Wrong version
    request = "{\"jsonrpc\": \"1.0\", \"method\": \"call\", \"params\": [], \"id\": 1}";
    make_request( request, JSON_RPC_INVALID_REQUEST );

    // Invalid JSON
    request = "\"jsonrpc\" \"2.0\"";
    make_request( request, JSON_RPC_PARSE_ERROR );
}
```

**Test Categories**:
1. **jsonrpc field validation**: Version string, type checking
2. **method field validation**: Required, string type, existence
3. **params field validation**: Object/array types, optional
4. **id field validation**: String/number/null types
5. **Batch requests**: Array handling, error aggregation

**Fixture**: `json_rpc_database_fixture`

### Market History Tests

**File**: [market_history.cpp](market_history.cpp)

**Purpose**: Validate market history plugin functionality

**Coverage**:
- Trade recording
- Order book tracking
- Volume calculations
- Price feed history
- Bucket aggregation (1min, 5min, 15min, 1hr, 4hr, 1day)

**Example Scenario**:
```cpp
BOOST_AUTO_TEST_CASE( market_history_tracking )
{
    // Create market orders
    limit_order_create_operation op;
    op.owner = "alice";
    op.amount_to_sell = asset( 1000, STEEM_SYMBOL );
    op.min_to_receive = asset( 1000, SBD_SYMBOL );

    // Execute trade
    push_transaction( op );

    // Verify history recorded
    auto history = get_market_history( STEEM_SYMBOL, SBD_SYMBOL );
    BOOST_REQUIRE( history.size() > 0 );
}
```

### Plugin Operations Tests

**File**: [plugin_ops.cpp](plugin_ops.cpp)

**Purpose**: Test plugin-specific operations

**Coverage**:
- Custom operation handling
- Plugin state updates
- Operation validation
- Plugin hooks and callbacks

### SMT Market History Tests

**File**: [smt_market_history.cpp](smt_market_history.cpp)

**Purpose**: SMT-specific market history functionality

**Coverage**:
- SMT trading pairs
- SMT order books
- SMT volume tracking
- Multi-token market data

**Build Requirement**: Requires `ENABLE_SMT_SUPPORT=ON`

## Test Fixtures

### json_rpc_database_fixture

Specialized fixture for JSON-RPC testing:
- Provides `make_request()` helper
- Provides `make_array_request()` for batch requests
- Validates JSON-RPC response structure
- Checks error codes and messages

From [../db_fixture/database_fixture.hpp](../db_fixture/database_fixture.hpp).

### Standard Fixtures

- `clean_database_fixture` - Fresh database for each test
- `database_fixture` - Shared database with test helpers

## CMake Configuration

From [../CMakeLists.txt](../CMakeLists.txt):

```cmake
file(GLOB PLUGIN_TESTS "plugin_tests/*.cpp")
add_executable( plugin_test ${PLUGIN_TESTS} )
target_link_libraries( plugin_test
    db_fixture
    steem_chain
    steem_protocol
    account_history_plugin
    market_history_plugin
    witness_plugin
    debug_node_plugin
    fc
    ${PLATFORM_SPECIFIC_LIBS}
)
```

## Test Initialization

From [main.cpp](main.cpp):

```cpp
boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
    // Seed random number generator
    std::srand(time(NULL));

    // Read genesis timestamp from environment
    const char* genesis_timestamp_str = getenv("STEEM_TESTING_GENESIS_TIMESTAMP");
    if( genesis_timestamp_str != nullptr ) {
        STEEM_TESTING_GENESIS_TIMESTAMP = std::stoul( genesis_timestamp_str );
    }

    return nullptr;
}
```

## Environment Variables

### STEEM_TESTING_GENESIS_TIMESTAMP

Controls blockchain genesis time for deterministic testing:

```bash
# Use specific timestamp
export STEEM_TESTING_GENESIS_TIMESTAMP=1514764800  # 2018-01-01 00:00:00 UTC

# Use current time
export STEEM_TESTING_GENESIS_TIMESTAMP=$(date +%s)

# Run tests
./tests/plugin_test
```

## Writing New Plugin Tests

### Template

```cpp
#include <boost/test/unit_test.hpp>
#include <steem/plugins/my_plugin/my_plugin.hpp>
#include "../db_fixture/database_fixture.hpp"

using namespace steem::plugins::my_plugin;

BOOST_FIXTURE_TEST_SUITE( my_plugin_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( basic_functionality )
{
    try {
        // Setup plugin
        auto& plugin = app.register_plugin< my_plugin >();
        plugin.plugin_initialize( options );
        plugin.plugin_startup();

        // Test functionality
        auto result = plugin.some_method();

        // Verify
        BOOST_REQUIRE_EQUAL( result.status, "success" );
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
```

### Best Practices

1. **Use Appropriate Fixtures**: Choose fixture based on test needs
2. **Test Error Paths**: Validate plugin error handling
3. **Test Plugin Lifecycle**: Initialize, startup, shutdown
4. **Mock Dependencies**: Isolate plugin functionality
5. **Test API Responses**: Validate JSON structure and content

## JSON-RPC Test Helpers

### make_request()

Validate single JSON-RPC request:

```cpp
void make_request(
    const std::string& request,
    int expected_error_code
);

// Usage
std::string request = "{\"jsonrpc\": \"2.0\", \"method\": \"unknown\", \"id\": 1}";
make_request( request, JSON_RPC_METHOD_NOT_FOUND );
```

### make_array_request()

Validate batch JSON-RPC requests:

```cpp
void make_array_request(
    const std::string& request,
    int expected_error_code
);

// Usage
std::string batch = "[{...}, {...}]";
make_array_request( batch, JSON_RPC_PARSE_ERROR );
```

## Debugging Tests

### Verbose Output

```bash
# Show detailed test execution
./tests/plugin_test --log_level=all

# Show test progress
./tests/plugin_test --show_progress
```

### GDB Debugging

```bash
# Debug specific test
gdb --args ./tests/plugin_test -t json_rpc/basic_validation

# Inside GDB
(gdb) break json_rpc.cpp:50
(gdb) run
```

### Print JSON Responses

Add debug output in tests:

```cpp
std::string response = make_request( request );
std::cout << "Response: " << response << std::endl;
```

## Common Test Patterns

### Testing JSON-RPC API

```cpp
BOOST_AUTO_TEST_CASE( api_call_test )
{
    // Valid request
    std::string request = R"({
        "jsonrpc": "2.0",
        "method": "database_api.get_config",
        "params": {},
        "id": 1
    })";

    auto response = make_request( request );
    BOOST_REQUIRE( response.contains("result") );
}
```

### Testing Market Operations

```cpp
BOOST_AUTO_TEST_CASE( trade_execution )
{
    ACTORS( (alice)(bob) )
    fund( "alice", 10000 );
    fund( "bob", 10000 );

    // Create buy order
    limit_order_create_operation buy_op;
    buy_op.owner = "alice";
    buy_op.amount_to_sell = asset( 1000, STEEM_SYMBOL );
    buy_op.min_to_receive = asset( 1000, SBD_SYMBOL );
    push_transaction( buy_op );

    // Create sell order (matches)
    limit_order_create_operation sell_op;
    sell_op.owner = "bob";
    sell_op.amount_to_sell = asset( 1000, SBD_SYMBOL );
    sell_op.min_to_receive = asset( 1000, STEEM_SYMBOL );
    push_transaction( sell_op );

    // Verify trade in history
    auto trades = get_recent_trades( STEEM_SYMBOL, SBD_SYMBOL, 10 );
    BOOST_REQUIRE_EQUAL( trades.size(), 1 );
}
```

### Testing Plugin State

```cpp
BOOST_AUTO_TEST_CASE( plugin_state_tracking )
{
    // Generate blocks with activity
    generate_blocks( 100 );

    // Query plugin state
    auto state = plugin.get_state();

    // Verify state updated correctly
    BOOST_REQUIRE( state.last_processed_block == 100 );
}
```

## Continuous Integration

Plugin tests run in CI after chain tests:

```dockerfile
# From Dockerfile.test
RUN make -j$(nproc) plugin_test && \
    ./tests/plugin_test
```

## Platform Support

Tests run on:
- Linux (GCC, Clang)
- macOS (Clang)
- Windows (MSVC) - Limited support

## Performance

### TCMalloc

If gperftools available, tests link with TCMalloc:

```cmake
if( GPERFTOOLS_FOUND )
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()
```

### Test Execution Time

- JSON-RPC tests: ~1-2 seconds
- Market history tests: ~5-10 seconds
- Full suite: ~30-60 seconds

## Testnet Requirement

Plugin tests require testnet build:

```bash
cmake -DBUILD_STEEM_TESTNET=ON ..
```

The `IS_TEST_NET` preprocessor flag enables test-specific code:

```cpp
#ifdef IS_TEST_NET
BOOST_AUTO_TEST_CASE( testnet_only_test )
{
    // This test only runs in testnet builds
}
#endif
```

## Troubleshooting

### Tests Skipped

If tests are wrapped in `#ifdef IS_TEST_NET` but don't run:

```bash
# Rebuild with testnet flag
cmake -DBUILD_STEEM_TESTNET=ON ..
make -j$(nproc) plugin_test
```

### Genesis Timestamp Issues

```bash
# Set explicit timestamp
export STEEM_TESTING_GENESIS_TIMESTAMP=1514764800
./tests/plugin_test
```

### Plugin Not Found

Ensure plugin is linked in CMakeLists.txt:

```cmake
target_link_libraries( plugin_test
    ...
    my_plugin  # Add your plugin here
    ...
)
```

## Additional Resources

- [Chain Tests](../tests/README.md)
- [Database Fixture](../db_fixture/README.md)
- [JSON-RPC Plugin](../../libraries/plugins/json_rpc/README.md)
- [Market History Plugin](../../libraries/plugins/market_history/README.md)
- [Boost.Test Documentation](https://www.boost.org/doc/libs/release/libs/test/)

## License

See [LICENSE](../../LICENSE.md)
