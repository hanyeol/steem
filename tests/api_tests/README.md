# API Tests (C++ Tests)

Python-based API comparison and validation test scripts.

## Overview

This directory contains Python scripts for testing Steem APIs by comparing responses between different nodes or implementations. Tests use concurrent execution for performance and generate detailed comparison reports.

**Note**: This is different from [`python_scripts/tests/api_tests/`](../../python_scripts/tests/api_tests/) which uses pyresttest framework.

## Test Scripts

| Script | Purpose |
|--------|---------|
| **[jsonsocket.py](jsonsocket.py)** | JSON-RPC socket communication utility |
| **[list_account.py](list_account.py)** | List all blockchain accounts |
| **[list_comment.py](list_comment.py)** | List all comments/posts |
| **[test_ah_get_account_history.py](test_ah_get_account_history.py)** | Compare account history API between nodes |
| **[test_ah_get_ops_in_block.py](test_ah_get_ops_in_block.py)** | Compare block operations API between nodes |
| **[test_list_votes.py](test_list_votes.py)** | Compare vote data (version 1) |
| **[test_list_votes2.py](test_list_votes2.py)** | Compare vote data (version 2) |

## Quick Start

### Prerequisites

```bash
# Python 3.6+
pip install requests  # If using HTTP transport
```

### Basic Usage

```bash
# Compare account history between two nodes
./test_ah_get_account_history.py 4 \
    http://127.0.0.1:8090 \
    http://127.0.0.1:8091

# Compare block operations
./test_ah_get_ops_in_block.py 4 \
    http://127.0.0.1:8090 \
    http://127.0.0.1:8091
```

## JSONSocket Utility

**File**: [jsonsocket.py](jsonsocket.py)

Low-level JSON-RPC socket communication.

### Usage

```python
from jsonsocket import JSONSocket, steemd_call

# Create socket connection
sock = JSONSocket("127.0.0.1", 8090, "/rpc")

# Make RPC call
response = steemd_call(
    sock,
    "database_api",
    "get_account_history",
    {"account": "alice", "start": -1, "limit": 100}
)

print(response)
```

### Functions

```python
# Initialize socket
JSONSocket(host, port, path, timeout=None)

# Make steemd RPC call
steemd_call(socket, api, method, params)

# Generic JSON-RPC call
json_rpc_call(socket, method, params, id=1)
```

## List Account Utility

**File**: [list_account.py](list_account.py)

Retrieve all accounts from blockchain.

### Usage

```python
from list_account import list_accounts

# Get all accounts from node
accounts = list_accounts("http://127.0.0.1:8090")

# Returns list of account names
for account in accounts:
    print(account)
```

### Standalone

```bash
python list_account.py http://127.0.0.1:8090 > accounts.txt
```

## List Comment Utility

**File**: [list_comment.py](list_comment.py)

Retrieve all comments/posts from blockchain.

### Usage

```python
from list_comment import list_comments

# Get all comments
comments = list_comments("http://127.0.0.1:8090")

# Returns list of comment identifiers
for comment in comments:
    print(f"{comment['author']}/{comment['permlink']}")
```

## Account History Comparison

**File**: [test_ah_get_account_history.py](test_ah_get_account_history.py)

Compare `get_account_history` API responses between two nodes.

### Usage

```bash
./test_ah_get_account_history.py <jobs> <url1> <url2> [working_dir] [accounts_file]

# Examples:
# Auto-detect CPU count
./test_ah_get_account_history.py 0 \
    http://node1:8090 \
    http://node2:8090

# 4 parallel jobs
./test_ah_get_account_history.py 4 \
    http://node1:8090 \
    http://node2:8090 \
    ./history_results

# Use specific account list
./test_ah_get_account_history.py 4 \
    http://node1:8090 \
    http://node2:8090 \
    ./history_results \
    accounts.txt
```

### Parameters

| Parameter | Required | Description |
|-----------|----------|-------------|
| `jobs` | Yes | Number of parallel workers (0 = auto-detect CPUs) |
| `url1` | Yes | Reference node URL (for account list) |
| `url2` | Yes | Comparison node URL |
| `working_dir` | No | Output directory (default: current dir) |
| `accounts_file` | No | Custom account list file (default: fetch from url1) |

### Output

Creates comparison files in `working_dir`:
- `account_name.json` - History data for each account
- Error logs for mismatches
- Summary statistics

### What It Tests

- Account operation history completeness
- Operation ordering consistency
- Virtual operation generation
- History pagination

## Block Operations Comparison

**File**: [test_ah_get_ops_in_block.py](test_ah_get_ops_in_block.py)

Compare `get_ops_in_block` API responses between two nodes.

### Usage

```bash
./test_ah_get_ops_in_block.py <jobs> <url1> <url2> [start_block] [end_block] [working_dir]

# Examples:
# Compare blocks 1-10000
./test_ah_get_ops_in_block.py 4 \
    http://node1:8090 \
    http://node2:8090 \
    1 \
    10000

# Auto-detect head block
./test_ah_get_ops_in_block.py 4 \
    http://node1:8090 \
    http://node2:8090
```

### Parameters

| Parameter | Required | Description |
|-----------|----------|-------------|
| `jobs` | Yes | Number of parallel workers |
| `url1` | Yes | Reference node URL |
| `url2` | Yes | Comparison node URL |
| `start_block` | No | Starting block number (default: 1) |
| `end_block` | No | Ending block number (default: head block) |
| `working_dir` | No | Output directory |

### What It Tests

- Block operation completeness
- Operation type consistency
- Virtual operation generation in blocks
- Block-level API accuracy

## Vote Comparison Tests

### test_list_votes.py

Compare vote data between nodes.

```bash
./test_list_votes.py <jobs> <url1> <url2> [working_dir] [comments_file]
```

### test_list_votes2.py

Enhanced vote comparison with additional validation.

```bash
./test_list_votes2.py <jobs> <url1> <url2> [working_dir] [comments_file]
```

## Parallel Execution

All test scripts support parallel execution for performance:

### ThreadPoolExecutor

For I/O-bound tasks (network requests):
```python
from concurrent.futures import ThreadPoolExecutor

with ThreadPoolExecutor(max_workers=jobs) as executor:
    futures = [executor.submit(test_account, acc) for acc in accounts]
    wait(futures)
```

### ProcessPoolExecutor

For CPU-bound tasks (data processing):
```python
from concurrent.futures import ProcessPoolExecutor

with ProcessPoolExecutor(max_workers=jobs) as executor:
    futures = [executor.submit(process_data, data) for data in dataset]
    wait(futures)
```

### Auto-detect CPUs

```python
import multiprocessing

if jobs <= 0:
    jobs = multiprocessing.cpu_count()
```

## Output and Reporting

### Working Directory Structure

```
working_dir/
├── account1.json          # Account history for account1
├── account2.json          # Account history for account2
├── block_1000.json        # Operations in block 1000
├── errors.log             # Error summary
└── summary.json           # Test statistics
```

### Error Tracking

```python
errors = 0

def future_end_cb(future):
    global errors
    if future.result() == False:
        errors += 1

# Register callback
future.add_done_callback(future_end_cb)
```

### Summary Statistics

- Total accounts/blocks tested
- Comparison errors found
- Processing time
- Throughput (items/sec)

## Use Cases

### 1. Node Upgrade Validation

Compare old vs new node implementation:
```bash
# Test account history API compatibility
./test_ah_get_account_history.py 8 \
    http://old-node:8090 \
    http://new-node:8090 \
    ./upgrade_validation
```

### 2. Plugin Testing

Validate account_history plugin:
```bash
# Compare account_history vs account_history_rocksdb
./test_ah_get_account_history.py 4 \
    http://node-with-ah:8090 \
    http://node-with-rocksdb:8090
```

### 3. Replay Verification

Verify replay produces identical results:
```bash
# Original node vs replayed node
./test_ah_get_ops_in_block.py 8 \
    http://original:8090 \
    http://replayed:8090 \
    1 \
    1000000
```

### 4. Load Testing

Stress test API endpoints:
```bash
# Maximum parallelism
./test_ah_get_account_history.py 0 \
    http://node:8090 \
    http://node:8090  # Compare to itself
```

## Writing New Comparison Tests

### Template

```python
#!/usr/bin/env python3
import sys
from concurrent.futures import ThreadPoolExecutor, wait
from jsonsocket import JSONSocket, steemd_call
from pathlib import Path

errors = 0

def compare_api_call(account, url1, url2, wdir):
    """Compare API responses for given account"""
    try:
        # Fetch from both nodes
        sock1 = JSONSocket.from_url(url1)
        sock2 = JSONSocket.from_url(url2)

        response1 = steemd_call(sock1, "api_name", "method", {"account": account})
        response2 = steemd_call(sock2, "api_name", "method", {"account": account})

        # Compare responses
        if response1 != response2:
            print(f"Mismatch for {account}")
            # Save diff to file
            with open(wdir / f"{account}.diff", "w") as f:
                f.write(f"URL1: {response1}\n")
                f.write(f"URL2: {response2}\n")
            return False

        return True
    except Exception as e:
        print(f"Error for {account}: {e}")
        return False

def main():
    if len(sys.argv) < 4:
        print("Usage: script.py <jobs> <url1> <url2> [working_dir]")
        exit(1)

    jobs = int(sys.argv[1])
    url1 = sys.argv[2]
    url2 = sys.argv[3]
    wdir = Path(sys.argv[4]) if len(sys.argv) > 4 else Path(".")

    # Get test data
    accounts = get_accounts(url1)

    # Run parallel comparison
    with ThreadPoolExecutor(max_workers=jobs) as executor:
        futures = [
            executor.submit(compare_api_call, acc, url1, url2, wdir)
            for acc in accounts
        ]
        wait(futures)

    # Report errors
    print(f"Total errors: {errors}")

if __name__ == "__main__":
    main()
```

## Best Practices

1. **Use Reference Node**: url1 should be known-good node
2. **Parallel Jobs**: Set based on network/CPU capacity
3. **Working Directory**: Use dedicated dir to avoid conflicts
4. **Error Handling**: Always handle network and JSON errors
5. **Progress Reporting**: Print progress for long-running tests
6. **Resource Cleanup**: Close sockets properly

## Performance Tuning

### Optimal Job Count

```python
import multiprocessing

# CPU-bound tasks
jobs = multiprocessing.cpu_count()

# I/O-bound tasks (network)
jobs = multiprocessing.cpu_count() * 4

# Memory-constrained
jobs = min(multiprocessing.cpu_count(), 4)
```

### Batching

For large datasets, process in batches:
```python
batch_size = 1000
for i in range(0, len(accounts), batch_size):
    batch = accounts[i:i+batch_size]
    # Process batch
```

### Connection Pooling

Reuse sockets when possible:
```python
# Create socket pool
sockets = [JSONSocket(host, port, path) for _ in range(jobs)]

# Distribute work
for i, account in enumerate(accounts):
    sock = sockets[i % jobs]
    # Use sock
```

## Troubleshooting

### Connection Errors

```
ConnectionRefusedError: [Errno 111] Connection refused
```

**Solution**: Ensure node is running and accessible:
```bash
curl http://127.0.0.1:8090
```

### Timeout Errors

**Solution**: Increase timeout in JSONSocket:
```python
sock = JSONSocket(host, port, path, timeout=60)
```

### Memory Issues

**Solution**: Reduce parallel jobs:
```bash
./test_script.py 2 url1 url2  # Use fewer workers
```

### Rate Limiting

**Solution**: Add delays between requests:
```python
import time
time.sleep(0.1)  # 100ms delay
```

## Integration with CI

### Example CI Script

```bash
#!/bin/bash
set -e

# Start two nodes
./steemd --data-dir=node1 &
./steemd --data-dir=node2 &
sleep 30

# Run comparison tests
cd tests/api_tests
./test_ah_get_account_history.py 4 \
    http://127.0.0.1:8090 \
    http://127.0.0.1:8091 \
    ./results

# Check for errors
if [ -f results/errors.log ]; then
    cat results/errors.log
    exit 1
fi
```

## Additional Resources

- [Python API Tests (pyresttest)](../../python_scripts/tests/api_tests/README.md)
- [Account History Plugin](../../libraries/plugins/account_history/README.md)
- [Account History API](../../libraries/plugins/account_history_api/README.md)
- [Database API Plugin](../../libraries/plugins/database_api/README.md)

## License

See [LICENSE](../../LICENSE.md)
