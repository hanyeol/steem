# Steem Test Suite

## Directory Structure

```
tests/
├── chain/              # Core blockchain tests → chain_test executable
│   ├── basic/          # Basic functionality tests
│   ├── block/          # Block validation tests
│   ├── operations/     # Operation tests (account, comment, vesting, etc.)
│   ├── serialization/  # Serialization tests
│   ├── database/       # Database and state management tests
│   ├── bmic/           # BMIC tests
│   └── live/           # Live chain tests (hardforks, mainnet data)
│
├── plugin/             # Plugin tests → plugin_test executable
│   ├── json_rpc/       # JSON-RPC plugin tests
│   └── market_history/ # Market history plugin tests
│
├── fixtures/           # Test fixtures (database_fixture, etc.)
├── helpers/            # Test helper utilities
│   ├── bmic/           # BMIC test helpers
│   └── undo/           # Undo test utilities
│
├── integration/        # Integration tests
│   ├── smoke/          # Node upgrade regression tests
│   └── api/            # API response validation tests
│
└── scripts/            # Test scripts
```

## Running Tests Locally

### Build Tests

```bash
cd build
cmake -DBUILD_STEEM_TESTNET=ON -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc) chain_test plugin_test
```

### Run All Tests

```bash
# Run all chain tests
./tests/chain_test

# Run all plugin tests
./tests/plugin_test
```

### Run Specific Test Suites

```bash
# Run specific test suite (either form works)
./tests/chain_test -t operation_tests
./tests/chain_test -t block_tests

# Run specific test case (either form works)
./tests/chain_test -t operation_tests/account_create_validate

# Verbose output
./tests/chain_test --log_level=all

# List all tests
./tests/chain_test --list_content
```

## Running Tests with Docker

### To Create Test Environment Container

From the root of the repository:

    docker build --rm=false \
        -t steemitinc/ci-test-environment:latest \
        -f tests/scripts/Dockerfile.testenv .

## To Run The Tests

(Also in the root of the repository.)

    docker build --rm=false \
        -t steemitinc/steem-test \
        -f Dockerfile.test .

## To Troubleshoot Failing Tests

    docker run -ti \
        steemitinc/ci-test-environment:latest \
        /bin/bash

Then, inside the container:

(These steps are taken from `/Dockerfile.test` in the
repository root.)

    git clone https://github.com/hanyeol/steem.git \
        /usr/local/src/steem
    cd /usr/local/src/steem
    git checkout <branch> # e.g. 123-feature
    git submodule update --init --recursive
    mkdir build
    cd build
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
