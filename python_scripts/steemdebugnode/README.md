# Steem Debug Node

Python wrapper for launching and interacting with a Steem debug node.

## Overview

The `steemdebugnode` package provides a Python interface to run steemd with the `debug_node` plugin enabled. This allows developers to:

- Replay blockchain history from existing data directories
- Control block production programmatically
- Generate blocks at specific timestamps
- Test hardforks and protocol changes
- Create isolated testing environments

## Installation

```bash
cd python_scripts
pip install -e .
```

## Files

| File | Purpose |
|------|---------|
| **[debugnode.py](debugnode.py)** | Main `DebugNode` class wrapper |
| **[private_testnet.py](private_testnet.py)** | Example script for running private testnet |
| **[__init__.py](__init__.py)** | Package initialization |

## DebugNode Class

### Basic Usage

```python
from steemdebugnode import DebugNode

# Create debug node
debug_node = DebugNode(
    steemd='/path/to/steemd',
    data_dir='/path/to/existing/data_dir',
    plugins=['account_history'],
    apis=['account_history_api'],
    steemd_err=sys.stderr
)

# Run with context manager (auto-cleanup)
with debug_node:
    # Replay blocks
    debug_node.debug_push_blocks(1000)

    # Generate blocks until timestamp
    debug_node.debug_generate_blocks_until(future_timestamp, True)

    # Set hardfork
    debug_node.debug_set_hardfork(20)
```

### Constructor Parameters

```python
DebugNode(
    steemd,          # Path to steemd binary
    data_dir,        # Existing data directory (not modified)
    args='',         # Additional steemd arguments
    plugins=[],      # Extra plugins (debug_node always loaded)
    apis=[],         # Extra APIs (debug_node_api always loaded)
    steemd_out=None, # Stdout stream (default: /dev/null)
    steemd_err=None  # Stderr stream (default: /dev/null)
)
```

### Key Methods

#### Block Production

```python
# Push N blocks from data directory
blocks_pushed = debug_node.debug_push_blocks(
    count=100,
    skip_validate_invariants=False
)

# Generate blocks until timestamp
debug_node.debug_generate_blocks_until(
    timestamp,           # Unix timestamp
    generate_sparsely    # True: only at witness slots
)

# Generate N blocks
debug_node.debug_generate_blocks(count=10)
```

#### Hardfork Testing

```python
# Set hardfork version
debug_node.debug_set_hardfork(hardfork_id=20)

# Get current hardfork
hf_version = debug_node.debug_get_hardfork()
```

#### JSON-RPC Calls

```python
# Make RPC call to any API
result = debug_node.rpc.call(
    'database_api',
    'get_dynamic_global_properties',
    {}
)
```

## Examples

### Replay and Test

```python
#!/usr/bin/env python3
from steemdebugnode import DebugNode
import sys

debug_node = DebugNode(
    steemd='./build/programs/steemd/steemd',
    data_dir='witness_node_data_dir',
    args='--replay',
    steemd_err=sys.stderr
)

with debug_node:
    # Replay all blocks
    total = 0
    while True:
        pushed = debug_node.debug_push_blocks(10000)
        if pushed == 0:
            break
        total += pushed
        print(f"Replayed {total} blocks")

    # Get current state
    props = debug_node.rpc.call('database_api',
                                'get_dynamic_global_properties', {})
    print(f"Head block: {props['head_block_number']}")
```

### Hardfork Testing

```python
from steemdebugnode import DebugNode
from time import time

debug_node = DebugNode(
    steemd='./steemd',
    data_dir='data_dir'
)

with debug_node:
    # Replay to current
    debug_node.debug_push_blocks(1000000, skip_validate_invariants=True)

    # Trigger next hardfork
    current_hf = debug_node.debug_get_hardfork()
    debug_node.debug_set_hardfork(current_hf + 1)

    # Generate blocks in new hardfork
    debug_node.debug_generate_blocks(100)

    # Verify no invariant violations
    debug_node.debug_push_blocks(0, skip_validate_invariants=False)
```

### Custom Plugin Testing

```python
debug_node = DebugNode(
    steemd='./steemd',
    data_dir='data_dir',
    plugins=['account_history', 'market_history'],
    apis=['account_history_api', 'market_history_api']
)

with debug_node:
    # Test account history
    history = debug_node.rpc.call(
        'account_history_api',
        'get_account_history',
        {'account': 'alice', 'start': -1, 'limit': 10}
    )

    # Test market history
    trades = debug_node.rpc.call(
        'market_history_api',
        'get_recent_trades',
        {'limit': 10}
    )
```

## Architecture

### Temporary Data Directory

The debug node:
1. Creates temporary directory
2. Copies blockchain data from source `data_dir`
3. Generates custom `config.ini` with debug plugins
4. Runs steemd with temporary directory
5. Cleans up on exit (context manager)

Source `data_dir` is **never modified**.

### Plugin Configuration

Default plugins always loaded:
- `chain`
- `p2p` (in no-listen mode)
- `debug_node`
- `witness`

Default APIs always available:
- `database_api` (ID: 0)
- `login_api` (ID: 1)
- `debug_node_api` (ID: 2)

Additional plugins/APIs are appended.

### RPC Connection

The wrapper:
- Waits for steemd to start
- Connects via JSON-RPC on port 8090
- Provides `debug_node.rpc` interface
- Automatically retries failed connections

## Dependencies

```python
# Required packages
steemapi         # Steem RPC client
json, logging    # Standard library
pathlib, shutil  # File operations
subprocess       # Process management
threading        # Thread-safe locking
```

Install `steemapi`:
```bash
pip install steem-python
```

## Thread Safety

The `DebugNode` class uses a lock (`_steemd_lock`) to ensure:
- Only one debug node instance runs at a time
- Proper cleanup on exit
- Safe concurrent RPC calls

## Troubleshooting

### steemd Won't Start

```python
# Enable stderr output to see errors
debug_node = DebugNode(
    steemd='./steemd',
    data_dir='data_dir',
    steemd_err=sys.stderr  # See error messages
)
```

### Connection Timeouts

Increase wait time by modifying `debugnode.py`:
```python
# In __enter__ method
sleep(10)  # Wait longer for steemd startup
```

### Replay Failures

```python
# Use --replay flag
debug_node = DebugNode(
    steemd='./steemd',
    data_dir='data_dir',
    args='--replay'  # Force full replay
)
```

### Invariant Violations

```python
# Skip validation during replay
debug_node.debug_push_blocks(
    count=100000,
    skip_validate_invariants=True  # Faster, less strict
)

# Then validate at end
debug_node.debug_push_blocks(0, skip_validate_invariants=False)
```

## Platform Support

**POSIX only** (Linux, macOS)

Windows support may be added in the future.

## Use Cases

1. **Hardfork Testing**: Verify new hardforks don't break consensus
2. **Plugin Development**: Test custom plugins in isolation
3. **Blockchain Analysis**: Replay and analyze historical data
4. **Performance Testing**: Benchmark block processing
5. **Integration Tests**: Automated testing with real blockchain data

## Example Scripts

See [`tests/`](../tests/) for complete examples:
- [`private_testnet.py`](private_testnet.py) - Run private testnet
- [`debug_hardforks.py`](../tests/debug_hardforks.py) - Test hardfork activation
- [`test_payouts.py`](../tests/test_payouts.py) - Verify payout calculations

## API Reference

### RPC Methods

All `debug_node_api` methods available via:
```python
debug_node.rpc.call('debug_node_api', 'method_name', params)
```

Common methods:
- `debug_push_blocks` - Replay blocks from data dir
- `debug_generate_blocks` - Generate N blocks
- `debug_generate_blocks_until` - Generate until timestamp
- `debug_set_hardfork` - Activate hardfork
- `debug_get_hardfork` - Get current hardfork version
- `debug_has_hardfork` - Check if hardfork active

See [debug_node plugin](../../libraries/plugins/debug_node/README.md) for full API.

## License

See [LICENSE](../../LICENSE.md)
