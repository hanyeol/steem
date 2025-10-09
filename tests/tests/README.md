# Chain Tests

Core blockchain functionality test suite (chain_test).

## Overview

Comprehensive C++ unit tests for Steem blockchain core functionality using Boost.Test framework. Tests include:
- Basic blockchain operations
- Block production and validation
- Operation serialization and deserialization
- BMIC (Block Modifying Irreversible Constraint) system
- Undo/redo mechanisms
- SMT (Smart Media Token) operations
- Live chain testing scenarios

## Test Files

| Test File | Purpose |
|-----------|---------|
| **[main.cpp](main.cpp)** | Test suite initialization |
| **[basic_tests.cpp](basic_tests.cpp)** | Basic blockchain functionality |
| **[block_tests.cpp](block_tests.cpp)** | Block production and validation |
| **[bmic_tests.cpp](bmic_tests.cpp)** | BMIC (Block Modifying Irreversible Constraint) |
| **[live_tests.cpp](live_tests.cpp)** | Live chain testing scenarios |
| **[operation_tests.cpp](operation_tests.cpp)** | Steem operation validation |
| **[operation_time_tests.cpp](operation_time_tests.cpp)** | Operation timing constraints |
| **[serialization_tests.cpp](serialization_tests.cpp)** | Protocol serialization |
| **[undo_tests.cpp](undo_tests.cpp)** | Undo/redo functionality |
| **[smt_operation_tests.cpp](smt_operation_tests.cpp)** | SMT operation validation |
| **[smt_operation_time_tests.cpp](smt_operation_time_tests.cpp)** | SMT operation timing |
| **[smt_tests.cpp](smt_tests.cpp)** | SMT protocol tests |

## Building

```bash
# From build directory
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_STEEM_TESTNET=ON \
      -DLOW_MEMORY_NODE=OFF \
      -DCLEAR_VOTES=ON \
      ..
make -j$(nproc) chain_test
```

## Running Tests

### All Tests

```bash
# Run all chain tests
./tests/chain_test
```

### Specific Test Suite

```bash
# Run specific test suite
./tests/chain_test -t basic_tests

# Run specific test case
./tests/chain_test -t basic_tests/parse_size_test

# List available tests
./tests/chain_test --list_content
```

### With Options

```bash
# Verbose output
./tests/chain_test --log_level=all

# Show test names
./tests/chain_test --show_progress

# Run specific pattern
./tests/chain_test -t "*/vote_*"
```

## Test Categories

### Basic Tests

**File**: [basic_tests.cpp](basic_tests.cpp)

**Coverage**:
- String/size parsing utilities
- Basic data structure validation
- Core protocol primitives
- Utility function tests

**Example**:
```cpp
BOOST_AUTO_TEST_CASE( parse_size_test )
{
    BOOST_CHECK_EQUAL( fc::parse_size( "0" ), 0 );
    BOOST_CHECK_EQUAL( fc::parse_size( "1k" ), 1024 );
    BOOST_CHECK_EQUAL( fc::parse_size( "1M" ), 1024*1024 );
}
```

### Block Tests

**File**: [block_tests.cpp](block_tests.cpp)

**Coverage**:
- Block production workflow
- Block header validation
- Block signing and verification
- Witness scheduling
- Block timestamp validation

### BMIC Tests

**File**: [bmic_tests.cpp](bmic_tests.cpp)

**Coverage**:
- Block Modifying Irreversible Constraint system
- Irreversible block handling
- State modification constraints
- Checkpoint validation

### Operation Tests

**File**: [operation_tests.cpp](operation_tests.cpp)

**Coverage**:
- All Steem operations validation
- Transfer, vote, comment operations
- Account creation and updates
- Witness operations
- Escrow and savings operations
- Custom operations

**Example Test**:
```cpp
BOOST_AUTO_TEST_CASE( transfer_operation_test )
{
    transfer_operation op;
    op.from = "alice";
    op.to = "bob";
    op.amount = asset( 1000, STEEM_SYMBOL );
    op.memo = "test transfer";

    op.validate();  // Should not throw
}
```

### Serialization Tests

**File**: [serialization_tests.cpp](serialization_tests.cpp)

**Coverage**:
- Operation serialization to binary
- Deserialization from binary
- JSON serialization
- Backwards compatibility
- Protocol version handling

**Note**: Compiled with `/bigobj` flag on MSVC due to large template instantiations.

### Undo Tests

**File**: [undo_tests.cpp](undo_tests.cpp)

**Coverage**:
- Database undo/redo mechanisms
- Session management
- State rollback
- Transaction atomicity
- Fork handling

### SMT Tests

**Files**:
- [smt_operation_tests.cpp](smt_operation_tests.cpp)
- [smt_operation_time_tests.cpp](smt_operation_time_tests.cpp)
- [smt_tests.cpp](smt_tests.cpp)

**Coverage**:
- SMT creation operations
- Token emission schedules
- SMT transfer operations
- Contribution tracking
- Revenue splits

**Build Requirement**: Requires `ENABLE_SMT_SUPPORT=ON`

### Live Tests

**File**: [live_tests.cpp](live_tests.cpp)

**Coverage**:
- Real blockchain scenario tests
- Multi-operation workflows
- Complex state transitions
- Edge cases and corner cases

## Test Fixtures

Tests use the `clean_database_fixture` from [db_fixture](../db_fixture/):

```cpp
BOOST_FIXTURE_TEST_SUITE( basic_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( my_test )
{
    // db is available from fixture
    BOOST_REQUIRE( db.head_block_num() == 0 );
}

BOOST_AUTO_TEST_SUITE_END()
```

**Available Fixtures**:
- `clean_database_fixture` - Fresh database for each test
- `database_fixture` - Shared database state
- Custom fixtures in specific test files

## CMake Configuration

From [../CMakeLists.txt](../CMakeLists.txt):

```cmake
file(GLOB UNIT_TESTS "tests/*.cpp")
add_executable( chain_test ${UNIT_TESTS} )
target_link_libraries( chain_test
    db_fixture
    chainbase
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

## Test Environment

### Environment Variables

```bash
# Set genesis timestamp for testing
export STEEM_TESTING_GENESIS_TIMESTAMP=1234567890

# Run tests
./tests/chain_test
```

### Random Seed

The test suite initializes random number generator:
```cpp
std::srand(time(NULL));
std::cout << "Random number generator seeded to " << time(NULL) << std::endl;
```

For reproducible tests, set seed manually in code.

## Writing New Tests

### Template

```cpp
#include <boost/test/unit_test.hpp>
#include <steem/chain/database.hpp>
#include "../db_fixture/database_fixture.hpp"

using namespace steem::chain;
using namespace steem::protocol;

BOOST_FIXTURE_TEST_SUITE( my_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( my_test_case )
{
    try {
        // Setup
        ACTORS( (alice)(bob) )
        fund( "alice", 10000 );

        // Execute
        transfer_operation op;
        op.from = "alice";
        op.to = "bob";
        op.amount = asset( 1000, STEEM_SYMBOL );

        signed_transaction tx;
        tx.operations.push_back( op );
        tx.set_expiration( db.head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
        tx.sign( alice_private_key, db.get_chain_id() );

        db.push_transaction( tx, 0 );

        // Verify
        BOOST_REQUIRE_EQUAL( db.get_balance( "alice", STEEM_SYMBOL ).amount.value, 9000 );
        BOOST_REQUIRE_EQUAL( db.get_balance( "bob", STEEM_SYMBOL ).amount.value, 1000 );
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
```

### Best Practices

1. **Use Fixtures**: Always use `clean_database_fixture` for isolation
2. **Wrap in try/catch**: Use `FC_LOG_AND_RETHROW()` for better error messages
3. **Use ACTORS Macro**: Create test accounts easily
4. **Check State**: Verify expected state after operations
5. **Test Edge Cases**: Include boundary conditions and error cases

## Debugging Tests

### Failed Test Output

```bash
# Show detailed failure info
./tests/chain_test --log_level=test_suite

# Break on first failure
./tests/chain_test --break_on_error
```

### GDB Debugging

```bash
# Debug with GDB
gdb --args ./tests/chain_test -t basic_tests/parse_size_test

# Inside GDB
(gdb) break basic_tests.cpp:50
(gdb) run
```

### Memory Leak Detection

```bash
# Use valgrind
valgrind --leak-check=full ./tests/chain_test
```

## Performance

### TCMalloc Support

If gperftools is available, tests link with TCMalloc for better performance:

```cmake
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling tests with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()
```

### Build Time

**serialization_tests.cpp** requires `/bigobj` on MSVC due to large template instantiations. May take several minutes to compile.

## Continuous Integration

Tests run automatically in CI:

```dockerfile
# From Dockerfile.test
RUN make -j$(nproc) chain_test && \
    ./tests/chain_test
```

### Docker Testing

See [../README.md](../README.md) for Docker-based testing instructions.

## Common Issues

### Compilation Errors

**Large object file on MSVC**:
```cmake
set_source_files_properties( tests/serialization_tests.cpp
    PROPERTIES COMPILE_FLAGS "/bigobj" )
```

### Runtime Errors

**Missing genesis timestamp**:
```bash
export STEEM_TESTING_GENESIS_TIMESTAMP=$(date +%s)
./tests/chain_test
```

**Database initialization failure**:
- Ensure `BUILD_STEEM_TESTNET=ON` during build
- Clean build directory and rebuild

## Additional Resources

- [Database Fixture](../db_fixture/README.md)
- [Plugin Tests](../plugin_tests/README.md)
- [Boost.Test Documentation](https://www.boost.org/doc/libs/release/libs/test/)
- [Testing Guide](../../docs/testing.md)

## License

See [LICENSE](../../LICENSE.md)
