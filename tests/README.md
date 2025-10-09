# Steem Tests

Comprehensive test suite for Steem blockchain implementation.

## Overview

The tests directory contains:
- **C++ Unit Tests**: Core blockchain and plugin functionality
- **Python Integration Tests**: API validation and comparison
- **Smoketest Framework**: Node upgrade validation
- **Test Fixtures**: Shared test utilities and helpers

## Test Components

| Component | Purpose | Documentation |
|-----------|---------|---------------|
| **[tests/](tests/)** | Core blockchain tests (chain_test) | [README](tests/README.md) |
| **[plugin_tests/](plugin_tests/)** | Plugin functionality tests (plugin_test) | [README](plugin_tests/README.md) |
| **[db_fixture/](db_fixture/)** | Test fixture library | [README](db_fixture/README.md) |
| **[api_tests/](api_tests/)** | Python API comparison tests | [README](api_tests/README.md) |
| **[smoketest/](smoketest/)** | Integration test framework | [README](smoketest/README.md) |
| **[bmic_objects/](bmic_objects/)** | BMIC test objects | [README](bmic_objects/README.md) |
| **[undo_data/](undo_data/)** | Undo system utilities | [README](undo_data/README.md) |
| **[scripts/](scripts/)** | Test helper scripts | [README](scripts/README.md) |
| **[generate_empty_blocks/](generate_empty_blocks/)** | Empty block generation | [README](generate_empty_blocks/README.md) |

## Quick Start

### Building Tests

```bash
# From build directory
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_STEEM_TESTNET=ON \
      -DLOW_MEMORY_NODE=OFF \
      -DCLEAR_VOTES=ON \
      ..

# Build all tests
make -j$(nproc) chain_test plugin_test

# Or build specific test
make chain_test
```

### Running Tests

```bash
# Core blockchain tests
./tests/chain_test

# Plugin tests
./tests/plugin_test

# Specific test suite
./tests/chain_test -t basic_tests

# Verbose output
./tests/chain_test --log_level=all
```

## Docker Testing

### Create Test Environment

From repository root:

```bash
docker build --rm=false \
    -t steemitinc/ci-test-environment:latest \
    -f tests/scripts/Dockerfile.testenv .
```

### Run Tests

```bash
docker build --rm=false \
    -t steemitinc/steem-test \
    -f Dockerfile.test .
```

### Troubleshoot Failing Tests

```bash
docker run -ti \
    steemitinc/ci-test-environment:latest \
    /bin/bash
```

Inside container:

```bash
git clone https://github.com/steemit/steem.git \
    /usr/local/src/steem
cd /usr/local/src/steem
git checkout <branch>
git submodule update --init --recursive
mkdir build && cd build

cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_STEEM_TESTNET=ON \
    -DLOW_MEMORY_NODE=OFF \
    -DCLEAR_VOTES=ON \
    ..

make -j$(nproc) chain_test
./tests/chain_test

cd /usr/local/src/steem
doxygen
programs/build_helpers/check_reflect.py
```

## Test Categories

### C++ Unit Tests

**chain_test** - Core blockchain functionality:
- Basic operations and validation
- Block production and validation
- Serialization/deserialization
- BMIC system
- Undo/redo mechanisms
- SMT operations

**plugin_test** - Plugin functionality:
- JSON-RPC protocol validation
- Market history tracking
- Plugin operation handling

See [tests/README.md](tests/README.md) and [plugin_tests/README.md](plugin_tests/README.md).

### Python Integration Tests

**API Tests** - Compare API responses between nodes:
- Account history comparison
- Block operations comparison
- Vote tracking validation

**Smoketest** - Automated upgrade validation:
- Run test vs reference nodes
- Compare API responses
- Generate diff reports

See [api_tests/README.md](api_tests/README.md) and [smoketest/README.md](smoketest/README.md).

## Test Fixtures

### clean_database_fixture

Fresh database for each test:

```cpp
BOOST_FIXTURE_TEST_SUITE( my_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( test_case )
{
    ACTORS( (alice)(bob) )
    fund( "alice", 10000 );
    // Test code...
}

BOOST_AUTO_TEST_SUITE_END()
```

### Helper Macros

```cpp
// Create test accounts
ACTORS( (alice)(bob)(charlie) )

// Fund accounts
fund( "alice", 10000 );
vest( "bob", 1000000 );

// Generate blocks
generate_blocks( 10 );

// Validate operations
REQUIRE_OP_VALIDATION_SUCCESS( op, field, value );
REQUIRE_OP_VALIDATION_FAILURE( op, field, value );
```

See [db_fixture/README.md](db_fixture/README.md).

## CMake Configuration

From [CMakeLists.txt](CMakeLists.txt):

```cmake
# Chain tests
file(GLOB UNIT_TESTS "tests/*.cpp")
add_executable( chain_test ${UNIT_TESTS} )
target_link_libraries( chain_test
    db_fixture chainbase steem_chain steem_protocol
    account_history_plugin market_history_plugin
    witness_plugin debug_node_plugin fc
)

# Plugin tests
file(GLOB PLUGIN_TESTS "plugin_tests/*.cpp")
add_executable( plugin_test ${PLUGIN_TESTS} )
target_link_libraries( plugin_test
    db_fixture steem_chain steem_protocol
    account_history_plugin market_history_plugin
    witness_plugin debug_node_plugin fc
)
```

## Environment Variables

### STEEM_TESTING_GENESIS_TIMESTAMP

Control genesis time for deterministic testing:

```bash
export STEEM_TESTING_GENESIS_TIMESTAMP=1514764800
./tests/chain_test
```

## CI Integration

### Example GitHub Actions

```yaml
name: Tests
on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
        with:
          submodules: recursive

      - name: Build tests
        run: |
          mkdir build && cd build
          cmake -DBUILD_STEEM_TESTNET=ON ..
          make -j$(nproc) chain_test plugin_test

      - name: Run tests
        run: |
          cd build
          ./tests/chain_test
          ./tests/plugin_test
```

## Writing New Tests

### C++ Unit Test

```cpp
#include <boost/test/unit_test.hpp>
#include "../db_fixture/database_fixture.hpp"

using namespace steem::chain;
using namespace steem::protocol;

BOOST_FIXTURE_TEST_SUITE( my_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( my_test )
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
        tx.set_expiration( db.head_block_time() + fc::seconds(60) );
        tx.sign( alice_private_key, db.get_chain_id() );

        db.push_transaction( tx, 0 );

        // Verify
        BOOST_REQUIRE_EQUAL(
            db.get_balance( "alice", STEEM_SYMBOL ).amount.value,
            9000
        );
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
```

### Python API Test

```python
#!/usr/bin/env python3
from jsonsocket import JSONSocket, steemd_call

def compare_api(account, url1, url2):
    sock1 = JSONSocket.from_url(url1)
    sock2 = JSONSocket.from_url(url2)

    response1 = steemd_call(sock1, "database_api", "get_account", {"account": account})
    response2 = steemd_call(sock2, "database_api", "get_account", {"account": account})

    assert response1 == response2

if __name__ == "__main__":
    compare_api("alice", "http://node1:8090", "http://node2:8090")
```

## Debugging Tests

### GDB

```bash
gdb --args ./tests/chain_test -t basic_tests/parse_size_test

(gdb) break basic_tests.cpp:50
(gdb) run
```

### Verbose Output

```bash
# Show all log messages
./tests/chain_test --log_level=all

# Show test progress
./tests/chain_test --show_progress

# List all tests
./tests/chain_test --list_content
```

### Memory Leak Detection

```bash
valgrind --leak-check=full ./tests/chain_test
```

## Best Practices

1. **Use Fixtures**: Always use appropriate fixtures for isolation
2. **Wrap in try/catch**: Use `FC_LOG_AND_RETHROW()` for better errors
3. **Test Edge Cases**: Include boundary conditions and error cases
4. **Clean State**: Ensure tests don't depend on execution order
5. **Descriptive Names**: Use clear test and suite names
6. **Document Complex Tests**: Add comments explaining test logic

## Troubleshooting

### Build Errors

```bash
# Clean build
rm -rf build && mkdir build && cd build
cmake -DBUILD_STEEM_TESTNET=ON ..
make -j$(nproc) chain_test
```

### Test Failures

```bash
# Run specific failing test
./tests/chain_test -t failing_suite/failing_test --log_level=all

# Check for timing issues
export STEEM_TESTING_GENESIS_TIMESTAMP=$(date +%s)
./tests/chain_test
```

### Missing Dependencies

```bash
# Install Boost.Test
sudo apt-get install libboost-test-dev

# Update submodules
git submodule update --init --recursive
```

## Additional Resources

- [Testing Guide](../docs/testing.md)
- [Building Steem](../docs/building.md)
- [Plugin Development](../docs/plugin.md)
- [Boost.Test Documentation](https://www.boost.org/doc/libs/release/libs/test/)

## License

See [LICENSE](../LICENSE.md)
