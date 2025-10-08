# Database Fixture

Test fixture library providing database setup and helper utilities for Steem blockchain tests.

## Overview

The db_fixture library provides:
- **Test Database Setup**: Automated blockchain database initialization
- **Test Helpers**: Macros and functions for common test operations
- **Account Management**: Easy creation and funding of test accounts
- **Transaction Utilities**: Simplified transaction building and execution
- **Assertion Macros**: Enhanced error reporting for test failures

Used by both [chain_test](../tests/) and [plugin_test](../plugin_tests/).

## Files

| File | Purpose |
|------|---------|
| **[database_fixture.hpp](database_fixture.hpp)** | Main fixture class and helper macros |
| **[database_fixture.cpp](database_fixture.cpp)** | Fixture implementation |
| **[CMakeLists.txt](CMakeLists.txt)** | Build configuration |

## Main Fixtures

### clean_database_fixture

Provides fresh database for each test case.

```cpp
BOOST_FIXTURE_TEST_SUITE( my_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( test_case )
{
    // db is available and clean
    BOOST_REQUIRE( db.head_block_num() == 0 );
}

BOOST_AUTO_TEST_SUITE_END()
```

**Features**:
- Clean state for each test
- Pre-initialized appbase application
- Loaded plugins (debug_node, database_api, etc.)
- Genesis block created

### database_fixture

Base fixture with shared database state.

```cpp
struct my_fixture : public database_fixture
{
    my_fixture() {
        // Custom setup
    }
};

BOOST_FIXTURE_TEST_SUITE( my_tests, my_fixture )
// Tests...
BOOST_AUTO_TEST_SUITE_END()
```

### json_rpc_database_fixture

Specialized fixture for JSON-RPC testing.

```cpp
BOOST_FIXTURE_TEST_SUITE( json_rpc_tests, json_rpc_database_fixture )

BOOST_AUTO_TEST_CASE( rpc_validation )
{
    std::string request = "{...}";
    make_request( request, expected_error_code );
}

BOOST_AUTO_TEST_SUITE_END()
```

**Additional Features**:
- `make_request()` - Test single JSON-RPC request
- `make_array_request()` - Test batch requests
- Automatic error code validation

## Helper Macros

### Account Creation

```cpp
// Create test accounts with private keys
ACTORS( (alice)(bob)(charlie) )

// Now you have:
// - Accounts: alice, bob, charlie
// - Private keys: alice_private_key, bob_private_key, charlie_private_key
// - Public keys: alice_public_key, bob_public_key, charlie_public_key
```

### Account Funding

```cpp
// Fund accounts with STEEM
fund( "alice", 10000 );
fund( "bob", 5000 );

// Fund with VESTS (Steem Power)
vest( "alice", 1000000 );
```

### Transaction Helpers

```cpp
// Push transaction with signature
PUSH_TX( db, tx, alice_private_key );

// Push block
PUSH_BLOCK( db );

// Generate blocks
generate_blocks( 10 );

// Generate blocks until time
generate_blocks( db.head_block_time() + fc::seconds(60) );
```

### Operation Validation

#### Success Cases

```cpp
transfer_operation op;
// ...

// Test that field value passes validation
REQUIRE_OP_VALIDATION_SUCCESS( op, amount, asset(1000, STEEM_SYMBOL) );

// Test that operation evaluates successfully
REQUIRE_OP_EVALUATION_SUCCESS( op, amount, asset(1000, STEEM_SYMBOL) );
```

#### Failure Cases

```cpp
// Test validation failure
REQUIRE_OP_VALIDATION_FAILURE( op, amount, asset(-1000, STEEM_SYMBOL) );

// Test validation failure with specific exception
REQUIRE_OP_VALIDATION_FAILURE_2( op, amount, asset(-1000, STEEM_SYMBOL), fc::assert_exception );

// Test evaluation failure
REQUIRE_THROW_WITH_VALUE( op, amount, asset(1000000, STEEM_SYMBOL), fc::exception );
```

### Exception Testing

```cpp
// Require specific exception
STEEM_REQUIRE_THROW(
    db.push_transaction( invalid_tx, ~0 ),
    fc::exception
);

// Check exception (non-fatal)
STEEM_CHECK_THROW(
    db.push_transaction( invalid_tx, ~0 ),
    fc::exception
);
```

## Fixture Members

### Database Access

```cpp
// Access database
database& db;

// Access appbase application
appbase::application& app;

// Generate blocks
generate_block();
generate_blocks( count );
generate_blocks( timestamp );
```

### Account Utilities

```cpp
// Create account
const account_object& create_account( name );
const account_object& create_account( name, creator, key );

// Get account
const account_object& get_account( name );

// Fund account
void fund( account_name, amount );
void vest( account_name, vests );
```

### Transaction Utilities

```cpp
// Create and sign transaction
signed_transaction create_transaction( operations );

// Push transaction
void push_transaction( tx );
void push_transaction( tx, skip_flags );

// Validate operations
void validate_database();
```

### Block Production

```cpp
// Generate single block
signed_block generate_block();

// Generate N blocks
void generate_blocks( uint32_t count );

// Generate until timestamp
void generate_blocks( fc::time_point_sec timestamp );

// Generate blocks until condition
void generate_blocks_until( condition_function );
```

## Configuration

### Initial Supply

```cpp
#define INITIAL_TEST_SUPPLY (10000000000ll)
```

Initial STEEM supply for test blockchain.

### Genesis Timestamp

```cpp
extern uint32_t STEEM_TESTING_GENESIS_TIMESTAMP;
```

Set via environment variable:
```bash
export STEEM_TESTING_GENESIS_TIMESTAMP=1234567890
./chain_test
```

### Plugin Loading

Fixtures automatically load:
- `debug_node_plugin`
- `database_api_plugin`
- `block_api_plugin`

Additional plugins loaded as needed.

## Usage Examples

### Basic Test

```cpp
#include "../db_fixture/database_fixture.hpp"

BOOST_FIXTURE_TEST_SUITE( transfer_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( basic_transfer )
{
    try {
        // Create accounts
        ACTORS( (alice)(bob) )

        // Fund alice
        fund( "alice", 10000 );

        // Create transfer
        transfer_operation op;
        op.from = "alice";
        op.to = "bob";
        op.amount = asset( 1000, STEEM_SYMBOL );

        signed_transaction tx;
        tx.operations.push_back( op );
        tx.set_expiration( db.head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
        tx.sign( alice_private_key, db.get_chain_id() );

        // Execute
        db.push_transaction( tx, 0 );

        // Verify
        BOOST_REQUIRE_EQUAL( db.get_balance( "alice", STEEM_SYMBOL ).amount.value, 9000 );
        BOOST_REQUIRE_EQUAL( db.get_balance( "bob", STEEM_SYMBOL ).amount.value, 1000 );
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
```

### Multi-Operation Transaction

```cpp
BOOST_AUTO_TEST_CASE( multi_op_transaction )
{
    try {
        ACTORS( (alice)(bob)(charlie) )
        fund( "alice", 10000 );

        // Build transaction with multiple operations
        signed_transaction tx;

        transfer_operation transfer1;
        transfer1.from = "alice";
        transfer1.to = "bob";
        transfer1.amount = asset( 1000, STEEM_SYMBOL );
        tx.operations.push_back( transfer1 );

        transfer_operation transfer2;
        transfer2.from = "alice";
        transfer2.to = "charlie";
        transfer2.amount = asset( 2000, STEEM_SYMBOL );
        tx.operations.push_back( transfer2 );

        tx.set_expiration( db.head_block_time() + fc::seconds(60) );
        tx.sign( alice_private_key, db.get_chain_id() );

        db.push_transaction( tx, 0 );

        // Verify all transfers
        BOOST_REQUIRE_EQUAL( db.get_balance( "alice", STEEM_SYMBOL ).amount.value, 7000 );
        BOOST_REQUIRE_EQUAL( db.get_balance( "bob", STEEM_SYMBOL ).amount.value, 1000 );
        BOOST_REQUIRE_EQUAL( db.get_balance( "charlie", STEEM_SYMBOL ).amount.value, 2000 );
    }
    FC_LOG_AND_RETHROW()
}
```

### Testing Time-Based Logic

```cpp
BOOST_AUTO_TEST_CASE( time_based_logic )
{
    try {
        ACTORS( (alice) )

        auto start_time = db.head_block_time();

        // Generate blocks for 1 hour
        generate_blocks( start_time + fc::hours(1) );

        // Verify time advanced
        BOOST_REQUIRE( db.head_block_time() >= start_time + fc::hours(1) );

        // Generate specific number of blocks
        generate_blocks( 100 );

        BOOST_REQUIRE_EQUAL( db.head_block_num(), 100 );
    }
    FC_LOG_AND_RETHROW()
}
```

### Testing Validation Failures

```cpp
BOOST_AUTO_TEST_CASE( validation_failures )
{
    try {
        transfer_operation op;
        op.from = "alice";
        op.to = "bob";
        op.amount = asset( 1000, STEEM_SYMBOL );

        // Negative amount should fail
        REQUIRE_OP_VALIDATION_FAILURE( op, amount, asset(-1000, STEEM_SYMBOL) );

        // Empty "from" should fail
        REQUIRE_OP_VALIDATION_FAILURE( op, from, "" );

        // Empty "to" should fail
        REQUIRE_OP_VALIDATION_FAILURE( op, to, "" );

        // Valid operation should pass
        REQUIRE_OP_VALIDATION_SUCCESS( op, amount, asset(1000, STEEM_SYMBOL) );
    }
    FC_LOG_AND_RETHROW()
}
```

### JSON-RPC Testing

```cpp
BOOST_FIXTURE_TEST_SUITE( rpc_tests, json_rpc_database_fixture )

BOOST_AUTO_TEST_CASE( rpc_validation )
{
    try {
        std::string request;

        // Invalid JSON
        request = "not json";
        make_request( request, JSON_RPC_PARSE_ERROR );

        // Missing jsonrpc field
        request = "{\"method\": \"call\", \"id\": 1}";
        make_request( request, JSON_RPC_INVALID_REQUEST );

        // Valid request
        request = R"({
            "jsonrpc": "2.0",
            "method": "database_api.get_config",
            "params": {},
            "id": 1
        })";
        make_request( request, 0 );  // Expect success
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
```

## CMake Configuration

From [CMakeLists.txt](CMakeLists.txt):

```cmake
add_library( db_fixture
             database_fixture.cpp
           )

target_link_libraries( db_fixture
                       steem_chain
                       steem_protocol
                       debug_node_plugin
                       database_api_plugin
                       block_api_plugin
                       fc
                     )
```

## Advanced Features

### Custom Fixture

```cpp
struct my_custom_fixture : public database_fixture
{
    my_custom_fixture()
    {
        // Custom initialization
        open_database();
        generate_blocks(10);

        ACTORS( (alice)(bob) )
        fund( "alice", 1000000 );
        fund( "bob", 1000000 );
    }

    ~my_custom_fixture()
    {
        // Custom cleanup
    }

    // Custom helper methods
    void setup_market()
    {
        // Market setup logic
    }
};
```

### Skip Flags

Control transaction validation:

```cpp
// Skip all validation (fast)
db.push_transaction( tx, ~0 );

// Skip specific checks
db.push_transaction( tx, database::skip_transaction_signatures );

// Full validation (default)
db.push_transaction( tx, 0 );
```

## Best Practices

1. **Always use FC_LOG_AND_RETHROW()**: Provides better error messages
   ```cpp
   BOOST_AUTO_TEST_CASE( my_test )
   {
       try {
           // Test code
       }
       FC_LOG_AND_RETHROW()
   }
   ```

2. **Use ACTORS macro**: Simplifies account creation
   ```cpp
   ACTORS( (alice)(bob) )  // Good

   // vs manual creation
   create_account("alice");  // Less convenient
   ```

3. **Fund accounts before use**: Avoid insufficient balance errors
   ```cpp
   ACTORS( (alice) )
   fund( "alice", 10000 );  // Fund before operations
   ```

4. **Set transaction expiration**: Required for valid transactions
   ```cpp
   tx.set_expiration( db.head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
   ```

5. **Sign transactions**: Required unless using skip flags
   ```cpp
   tx.sign( alice_private_key, db.get_chain_id() );
   ```

## Debugging

### Enable Debug Output

```cpp
// In test
fc::enable_record_assert_trip = true;

STEEM_CHECK_THROW( ... );  // Will print debug info
```

### Inspect Database State

```cpp
// Print account state
const auto& alice = db.get_account("alice");
std::cout << fc::json::to_pretty_string( alice ) << std::endl;

// Print balance
auto balance = db.get_balance( "alice", STEEM_SYMBOL );
std::cout << "Balance: " << balance << std::endl;
```

### Validate Invariants

```cpp
// Check database invariants
db.validate_invariants();

// Generate blocks with validation
generate_blocks( 10, false );  // Don't skip validation
```

## Additional Resources

- [Chain Tests](../tests/README.md)
- [Plugin Tests](../plugin_tests/README.md)
- [Boost.Test Documentation](https://www.boost.org/doc/libs/release/libs/test/)
- [Testing Guide](../../doc/testing.md)

## License

See [LICENSE](../../LICENSE.md)
