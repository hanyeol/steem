# Account History RocksDB Plugin

Scalable account history using RocksDB storage.

## Overview

Alternative to `account_history` plugin using RocksDB for better scalability:
- Stores history on disk instead of memory
- Supports larger history datasets
- Lower memory footprint
- Slightly slower queries than memory-based version

## Configuration

```ini
plugin = account_history_rocksdb

# RocksDB data directory
account-history-rocksdb-path = "account_history_db"

# Track options (same as account_history)
# account-history-track-account-range = ["a","z"]
# account-history-whitelist-ops = transfer_operation
```

## Features

- **Disk-Based Storage**: Uses RocksDB for persistence
- **Scalability**: Handles large history datasets
- **Memory Efficient**: Significantly lower RAM usage
- **Same API**: Compatible with account_history_api

## Trade-offs

**Advantages:**
- Lower memory usage
- Larger history capacity
- Better for resource-constrained nodes

**Disadvantages:**
- Slower query performance
- Requires fast SSD for good performance
- Additional disk space needed

## Dependencies

- `chain` - Operation processing
- RocksDB library

## Used By

- `account_history_api` - Compatible API access
