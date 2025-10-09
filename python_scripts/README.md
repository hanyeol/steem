# Python Scripts

Python utilities and testing frameworks for Steem blockchain development.

## Overview

This directory contains Python packages and test scripts for:
- Running debug nodes with controlled blockchain environments
- Automated API testing and validation
- Protocol testing (hardforks, payouts, consensus)
- Integration testing with real blockchain data
- Development utilities for Steem node interaction

## Components

| Component | Purpose | Documentation |
|-----------|---------|---------------|
| **[steemdebugnode/](steemdebugnode/)** | Debug node wrapper package | [README](steemdebugnode/README.md) |
| **[tests/](tests/)** | Integration test scripts | [README](tests/README.md) |
| **[setup.py](setup.py)** | Package installation script | See below |

## Quick Start

### Installation

```bash
# Install steemdebugnode package
cd python_scripts
pip install -e .

# Or install with dependencies
pip install -e . steem-python
```

### Running Tests

```bash
# Hardfork testing
python tests/debug_hardforks.py \
    --steemd /path/to/steemd \
    --data-dir /path/to/data_dir

# Payout validation
python tests/test_payouts.py \
    --steemd /path/to/steemd \
    --data-dir /path/to/data_dir

# API testing
cd tests/api_tests
./run_api_tests.sh equal
```

## steemdebugnode Package

Python wrapper for launching steemd with the debug_node plugin.

### Features

- **Context Manager**: Automatic cleanup with `with` statement
- **Block Replay**: Replay blocks from existing data directories
- **Block Generation**: Programmatically generate new blocks
- **Hardfork Control**: Activate specific hardfork versions
- **Isolated Testing**: Uses temporary directories (source never modified)
- **RPC Interface**: Built-in JSON-RPC client

### Installation

```bash
cd python_scripts
pip install -e .
```

This installs the `steemdebugnode` package in development mode.

### Basic Usage

```python
from steemdebugnode import DebugNode

# Create debug node
debug_node = DebugNode(
    steemd='./build/programs/steemd/steemd',
    data_dir='witness_node_data_dir'
)

# Run with auto-cleanup
with debug_node:
    # Replay blocks
    debug_node.debug_push_blocks(1000)

    # Get blockchain state
    props = debug_node.rpc.call(
        'database_api',
        'get_dynamic_global_properties',
        {}
    )
    print(f"Head block: {props['head_block_number']}")
```

See [steemdebugnode/README.md](steemdebugnode/README.md) for complete documentation.

## Test Scripts

### Integration Tests

Located in [`tests/`](tests/), these scripts test blockchain protocol behavior.

#### debug_hardforks.py

Tests hardfork activation and consensus rules.

```bash
python tests/debug_hardforks.py \
    --steemd build/programs/steemd/steemd \
    --data-dir witness_node_data_dir \
    --pause-node false
```

**Tests**:
- Block replay from data directory
- Hardfork version activation
- Post-hardfork block production
- Database invariant validation

#### test_payouts.py

Validates reward distribution calculations.

```bash
python tests/test_payouts.py \
    --steemd build/programs/steemd/steemd \
    --data-dir witness_node_data_dir \
    --pause-node true
```

**Tests**:
- Author/curator reward split
- Curation window mechanics
- Beneficiary routing
- Reward fund distribution
- SBD debt calculations

### API Tests

Automated API validation framework using `pyresttest`.

```bash
# Install dependencies
pip install pyresttest

# Run tests
cd tests/api_tests
./run_api_tests.sh equal  # Exact match
./run_api_tests.sh contain  # Containment check
```

**Features**:
- Declarative YAML test definitions
- Pattern-based response validation
- Support for multiple APIs
- Detailed failure reporting
- Output file generation for debugging

See [tests/api_tests/README.md](tests/api_tests/README.md) for details.

## Package Structure

```
python_scripts/
├── setup.py                      # Package metadata and installation
├── steemdebugnode/              # Debug node wrapper package
│   ├── __init__.py              # Package exports
│   ├── debugnode.py             # Main DebugNode class
│   └── private_testnet.py       # Example usage script
└── tests/                        # Integration tests
    ├── debug_hardforks.py       # Hardfork testing
    ├── test_payouts.py          # Payout validation
    └── api_tests/               # API test framework
        ├── run_api_tests.sh     # Test runner
        ├── validator_ex.py      # Custom validators
        ├── database_api/        # Database API tests
        └── account_history_api/ # Account history tests
```

## setup.py

Package installation configuration.

```python
from setuptools import setup

setup(
    name='steemdebugnode',
    version='0.1',
    description='Wrapper for Steem Debug Node',
    url='http://github.com/steemit/steem',
    author='Steemit, Inc.',
    author_email='vandeberg@steemit.com',
    license='See LICENSE.md',
    packages=['steemdebugnode'],
    zip_safe=False
)
```

### Development Installation

```bash
# Install in editable mode (changes reflected immediately)
pip install -e .

# Uninstall
pip uninstall steemdebugnode
```

### Distribution

```bash
# Build package
python setup.py sdist bdist_wheel

# Install from wheel
pip install dist/steemdebugnode-0.1-*.whl
```

## Requirements

### System Requirements

- **Platform**: POSIX (Linux, macOS) - Windows not supported
- **Python**: 3.6 or later

### Python Dependencies

```bash
# Core dependencies
pip install steem-python  # RPC client
pip install pyresttest    # API testing (optional)
pip install past six      # Python 2/3 compatibility
```

### Steem Dependencies

- **steemd**: Built with debug_node plugin enabled
- **Data Directory**: Existing blockchain data (for replay tests)

### Building steemd

```bash
# Standard build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) steemd

# Testnet build (for testing)
cmake -DBUILD_STEEM_TESTNET=ON ..
make -j$(nproc) steemd
```

## Use Cases

### 1. Hardfork Testing

Test protocol upgrades before deployment:

```python
with DebugNode('./steemd', 'data_dir') as node:
    # Replay to current state
    node.debug_push_blocks(1000000)

    # Activate next hardfork
    node.debug_set_hardfork(21)

    # Generate blocks in new hardfork
    node.debug_generate_blocks(100)

    # Verify no violations
    assert node.debug_push_blocks(0) == 0
```

### 2. Plugin Development

Test custom plugins with real data:

```python
node = DebugNode(
    './steemd',
    'data_dir',
    plugins=['my_custom_plugin'],
    apis=['my_custom_api']
)

with node:
    # Test plugin functionality
    result = node.rpc.call('my_custom_api', 'my_method', {})
```

### 3. API Validation

Automated API regression testing:

```bash
# Generate API response patterns
cd tests/api_tests
./run_api_tests.sh equal

# Verify responses match after changes
./run_api_tests.sh contain
```

### 4. Performance Analysis

Benchmark block processing:

```python
import time

with DebugNode('./steemd', 'data_dir') as node:
    start = time.time()
    blocks = node.debug_push_blocks(10000)
    elapsed = time.time() - start

    print(f"Processed {blocks} blocks in {elapsed:.2f}s")
    print(f"Rate: {blocks/elapsed:.2f} blocks/sec")
```

### 5. Blockchain Analysis

Analyze historical blockchain data:

```python
with DebugNode('./steemd', 'data_dir') as node:
    # Replay to specific point
    node.debug_push_blocks(5000000)

    # Analyze state at that block
    accounts = node.rpc.call(
        'database_api',
        'list_accounts',
        {'start': '', 'limit': 1000, 'order': 'by_name'}
    )

    # Process results
    for account in accounts:
        print(f"{account['name']}: {account['balance']}")
```

## Development

### Running Tests Locally

```bash
# Build steemd with debug plugin
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_STEEM_TESTNET=ON ..
make -j$(nproc) steemd

# Create test data directory
mkdir -p test_data_dir/blockchain

# Install Python package
cd ../python_scripts
pip install -e .

# Run integration tests
python tests/debug_hardforks.py \
    -s ../build/programs/steemd/steemd \
    -d test_data_dir

# Run API tests (requires running node)
../build/programs/steemd/steemd --data-dir=test_data_dir &
sleep 30
cd tests/api_tests
./run_api_tests.sh contain
```

### Writing New Tests

1. **Create test script** in `tests/`:
   ```python
   from steemdebugnode import DebugNode

   def main():
       with DebugNode('./steemd', 'data_dir') as node:
           # Test logic here
           pass
   ```

2. **Add to test suite**:
   ```bash
   # Add to CI configuration
   python tests/my_new_test.py -s ./steemd -d data_dir
   ```

3. **Document test** in `tests/README.md`

See [tests/README.md](tests/README.md) for test writing guide.

## Continuous Integration

### Example CI Configuration

```yaml
# .github/workflows/python_tests.yml
name: Python Tests
on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2

      - name: Set up Python
        uses: actions/setup-python@v2
        with:
          python-version: '3.8'

      - name: Install dependencies
        run: |
          pip install -e python_scripts
          pip install pyresttest

      - name: Build steemd
        run: |
          mkdir build && cd build
          cmake -DBUILD_STEEM_TESTNET=ON ..
          make -j$(nproc) steemd

      - name: Run integration tests
        run: |
          python python_scripts/tests/debug_hardforks.py \
            -s build/programs/steemd/steemd \
            -d test_data_dir

      - name: Run API tests
        run: |
          build/programs/steemd/steemd --data-dir=test_data_dir &
          sleep 30
          cd python_scripts/tests/api_tests
          ./run_api_tests.sh contain
```

## Troubleshooting

### Import Errors

```
ModuleNotFoundError: No module named 'steemdebugnode'
```

**Solution**: Install package in development mode:
```bash
cd python_scripts
pip install -e .
```

### steemd Won't Start

**Solution**: Enable stderr to see errors:
```python
debug_node = DebugNode(
    steemd='./steemd',
    data_dir='data_dir',
    steemd_err=sys.stderr  # See error output
)
```

### Platform Not Supported

```
This script only works on POSIX systems
```

**Solution**: These scripts require Linux or macOS. Windows not supported.

### Connection Timeout

**Solution**: Increase startup wait time in `steemdebugnode/debugnode.py`:
```python
# In __enter__ method (line ~100)
sleep(10)  # Wait longer for steemd startup
```

### Replay Failures

**Solution**: Use `--replay` flag:
```python
debug_node = DebugNode(
    steemd='./steemd',
    data_dir='data_dir',
    args='--replay'  # Force clean replay
)
```

## Best Practices

1. **Always use context manager**: Ensures cleanup
   ```python
   with DebugNode(...) as node:
       # Tests here
   ```

2. **Skip validation during replay**: Faster development
   ```python
   node.debug_push_blocks(100000, skip_validate_invariants=True)
   ```

3. **Validate at end**: Catch invariant violations
   ```python
   node.debug_push_blocks(0, skip_validate_invariants=False)
   ```

4. **Enable stderr for debugging**: See error messages
   ```python
   DebugNode(..., steemd_err=sys.stderr)
   ```

5. **Use testnet builds**: Separate from production data
   ```bash
   cmake -DBUILD_STEEM_TESTNET=ON ..
   ```

## Additional Resources

- [steemdebugnode Package](steemdebugnode/README.md)
- [Integration Tests](tests/README.md)
- [API Tests](tests/api_tests/README.md)
- [Debug Node Plugin](../libraries/plugins/debug_node/README.md)
- [Building Steem](../docs/building.md)
- [Testing Guide](../docs/testing.md)

## Contributing

When adding new Python utilities or tests:

1. Follow existing code structure
2. Add type hints where possible
3. Include docstrings for functions/classes
4. Update relevant README files
5. Add tests to CI configuration

## License

See [LICENSE](../LICENSE.md)
