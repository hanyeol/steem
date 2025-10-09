# Python Tests

Test scripts for Steem blockchain functionality using the debug node framework.

## Overview

This directory contains Python-based integration tests that:
- Use the `steemdebugnode` wrapper to run isolated test environments
- Replay blockchain history and verify protocol behavior
- Test hardfork activation and consensus rules
- Validate payout calculations and economic logic
- Provide automated API testing framework

## Test Scripts

| Script | Purpose | Documentation |
|--------|---------|---------------|
| **[debug_hardforks.py](debug_hardforks.py)** | Test hardfork activation and verification | See below |
| **[test_payouts.py](test_payouts.py)** | Validate reward payout calculations | See below |
| **[api_tests/](api_tests/)** | Automated API testing framework | [README](api_tests/README.md) |

## debug_hardforks.py

Tests hardfork activation on replayed blockchain data.

### Usage

```bash
python debug_hardforks.py \
    --steemd /path/to/steemd \
    --data-dir /path/to/data_dir \
    --pause-node false
```

### Arguments

| Argument | Description | Required | Default |
|----------|-------------|----------|---------|
| `--steemd`, `-s` | Path to steemd binary | Yes | - |
| `--data-dir`, `-d` | Existing data directory to replay | Yes | - |
| `--pause-node`, `-p` | Keep node running after tests | No | `false` |

### What It Tests

1. **Block Replay**: Replays all blocks from data directory in 100k batches
2. **Hardfork Activation**: Triggers next hardfork version
3. **Block Production**: Generates new blocks under new hardfork
4. **Invariant Validation**: Verifies no database invariants violated
5. **Consensus Check**: Ensures protocol rules maintained

### Example

```bash
# Build steemd first
cd build && make steemd

# Run hardfork test
python python_scripts/tests/debug_hardforks.py \
    --steemd build/programs/steemd/steemd \
    --data-dir witness_node_data_dir

# Output:
# Replaying blocks...
# Blocks Replayed: 100000
# Blocks Replayed: 200000
# ...
# Generating blocks after hardfork...
# Validating invariants...
# All tests passed!
```

### Exit Codes

- `0` - All tests passed
- `1` - Error occurred (invalid paths, replay failure, etc.)

## test_payouts.py

Validates reward payout calculations and distribution logic.

### Usage

```bash
python test_payouts.py \
    --steemd /path/to/steemd \
    --data-dir /path/to/data_dir \
    --pause-node true
```

### Arguments

| Argument | Description | Required | Default |
|----------|-------------|----------|---------|
| `--steemd`, `-s` | Path to steemd binary | Yes | - |
| `--data-dir`, `-d` | Existing data directory | Yes | - |
| `--pause-node`, `-p` | Pause after tests for inspection | No | `true` |

### What It Tests

1. **Reward Pool Distribution**: Verifies reward fund calculations
2. **Author Payouts**: Checks author/curator split logic
3. **Curation Rewards**: Validates curation window and weights
4. **Beneficiary Payouts**: Tests beneficiary routing
5. **SBD Conversions**: Verifies debt/equity ratio calculations

### Example

```bash
python python_scripts/tests/test_payouts.py \
    --steemd build/programs/steemd/steemd \
    --data-dir witness_node_data_dir \
    --pause-node false
```

## api_tests/

Automated API response validation framework.

See [api_tests/README.md](api_tests/README.md) for detailed documentation.

### Quick Start

```bash
# Run all API tests
cd python_scripts/tests/api_tests
./run_api_tests.sh
```

## Requirements

### System Requirements

- **Platform**: POSIX (Linux, macOS)
- **Python**: 3.6+

### Python Dependencies

```bash
# Install steemdebugnode package
cd python_scripts
pip install -e .

# Or install dependencies manually
pip install steem-python
```

### Steem Dependencies

- **steemd binary**: Built with debug_node plugin
- **Data directory**: Existing blockchain data for replay tests

## Running Tests

### Individual Test

```bash
# Run specific test
python debug_hardforks.py -s ./steemd -d ./data_dir

# Run with verbose output
python test_payouts.py -s ./steemd -d ./data_dir --pause-node true 2>&1
```

### All Tests

```bash
# Run hardfork and payout tests
python debug_hardforks.py -s ./steemd -d ./data_dir && \
python test_payouts.py -s ./steemd -d ./data_dir

# Run API tests
cd api_tests && ./run_api_tests.sh
```

## Writing New Tests

### Template

```python
#!/usr/bin/env python3
"""
Test description
"""
import sys, signal
from argparse import ArgumentParser
from pathlib import Path
from steemdebugnode import DebugNode

WAITING = True

def main():
    global WAITING

    # Parse arguments
    parser = ArgumentParser(description='Test description')
    parser.add_argument('--steemd', '-s', type=str, required=True)
    parser.add_argument('--data-dir', '-d', type=str, required=True)
    args = parser.parse_args()

    # Validate paths
    steemd = Path(args.steemd)
    if not steemd.exists():
        print('Error: steemd does not exist')
        return 1

    data_dir = Path(args.data_dir)
    if not data_dir.exists():
        print('Error: data_dir does not exist')
        return 1

    # Setup signal handler
    signal.signal(signal.SIGINT, sigint_handler)

    # Create debug node
    debug_node = DebugNode(
        str(steemd),
        str(data_dir),
        steemd_err=sys.stderr
    )

    # Run tests
    with debug_node:
        run_tests(debug_node)

    return 0

def run_tests(debug_node):
    """Main test logic"""
    # Replay blocks
    print('Replaying blocks...')
    debug_node.debug_push_blocks(1000)

    # Your test logic here
    props = debug_node.rpc.call(
        'database_api',
        'get_dynamic_global_properties',
        {}
    )
    print(f"Head block: {props['head_block_number']}")

    # Assertions
    assert props['head_block_number'] > 0

    print('All tests passed!')

def sigint_handler(signum, frame):
    global WAITING
    WAITING = False

if __name__ == '__main__':
    sys.exit(main())
```

### Best Practices

1. **Use Context Manager**: Always use `with debug_node:` for auto-cleanup
2. **Signal Handling**: Implement SIGINT handler for clean exit
3. **Path Validation**: Verify steemd and data_dir exist
4. **Error Output**: Use `steemd_err=sys.stderr` to see errors
5. **Skip Invariants**: Use `skip_validate_invariants=True` for fast replay
6. **Final Validation**: Run `debug_push_blocks(0, False)` to validate at end

## Debugging Tests

### Enable Verbose Output

```python
debug_node = DebugNode(
    str(steemd),
    str(data_dir),
    steemd_out=sys.stdout,  # See all output
    steemd_err=sys.stderr
)
```

### Pause for Inspection

```python
# Keep node running after test
WAITING = True
while WAITING:
    sleep(1)

# In another terminal, connect with cli_wallet
cli_wallet -s ws://127.0.0.1:8090
```

### Check RPC Connection

```python
try:
    props = debug_node.rpc.call('database_api', 'get_dynamic_global_properties', {})
    print(f"Connected! Head block: {props['head_block_number']}")
except Exception as e:
    print(f"RPC error: {e}")
```

## Continuous Integration

### Example CI Script

```bash
#!/bin/bash
set -e

# Build steemd
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_STEEM_TESTNET=ON ..
make -j$(nproc) steemd

# Setup test data directory
mkdir -p test_data_dir/blockchain

# Install Python dependencies
pip install -e python_scripts

# Run tests
python python_scripts/tests/debug_hardforks.py \
    --steemd build/programs/steemd/steemd \
    --data-dir test_data_dir \
    --pause-node false

python python_scripts/tests/test_payouts.py \
    --steemd build/programs/steemd/steemd \
    --data-dir test_data_dir \
    --pause-node false

# Run API tests
cd python_scripts/tests/api_tests
./run_api_tests.sh
```

## Common Issues

### Import Errors

```bash
# Install package in development mode
cd python_scripts
pip install -e .
```

### steemd Not Found

```bash
# Use absolute path
python debug_hardforks.py \
    --steemd /absolute/path/to/steemd \
    --data-dir /absolute/path/to/data_dir
```

### Platform Not Supported

```
This script only works on POSIX systems
```

These tests require POSIX (Linux/macOS). Windows not supported.

### Connection Timeouts

Increase sleep time in `steemdebugnode/debugnode.py`:
```python
# Line ~100 in __enter__ method
sleep(10)  # Wait longer for steemd startup
```

## Performance

### Replay Speed

- **With validation**: ~5,000 blocks/sec
- **Skip validation**: ~50,000 blocks/sec

Use `skip_validate_invariants=True` for faster replay during development.

### Memory Usage

Debug nodes load full blockchain state into memory. For large replays:

```bash
# Build low memory node
cmake -DLOW_MEMORY_NODE=ON ..
make steemd
```

## Additional Resources

- [steemdebugnode Package](../steemdebugnode/README.md)
- [Debug Node Plugin](../../libraries/plugins/debug_node/README.md)
- [Building Steem](../../docs/building.md)
- [C++ Testing Framework](../../tests/README.md)

## License

See [LICENSE](../../LICENSE.md)
