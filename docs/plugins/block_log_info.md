# block_log_info Plugin

Calculates and reports running hash and size statistics of the block log file for verification and integrity monitoring.

## Overview

The `block_log_info` plugin computes a running cryptographic hash (SHA-256) of the block log file along with size metrics. It periodically outputs this information for verification, integrity checking, and debugging purposes. This is particularly useful for ensuring block log consistency across nodes and detecting corruption.

**Plugin Type**: Utility (Monitoring)
**Dependencies**: [chain](chain.md)
**Memory**: Minimal (~1-5MB)
**CPU**: Low (incremental hashing)
**Default**: Disabled (enable explicitly)

## Purpose

- **Block Log Verification**: Verify block log integrity across nodes
- **Corruption Detection**: Detect block log file corruption
- **Synchronization Verification**: Confirm identical block logs between nodes
- **Debugging**: Troubleshoot block log issues
- **Audit Trail**: Create verifiable checkpoints of blockchain state
- **Forensics**: Investigate blockchain history discrepancies

## How It Works

### Running Hash Calculation

**Incremental SHA-256**:
- Hash is updated block-by-block
- Includes block data + offset information
- Deterministic across all nodes
- Computationally efficient

**Hash Input** (per block):
```
Block data (serialized block)
+ Block offset (8 bytes, little-endian)
```

**Algorithm**:
```cpp
for each block:
    serialize(block) → data
    data.append(block_offset as 8 bytes)
    running_hash.update(data)
```

### Size Tracking

**Cumulative size**: Total bytes written to block log
**Includes**:
- Block data (transactions, signatures)
- Offset markers
- All block log overhead

### Reporting Intervals

**Time-based reporting**:
- Configurable interval (default: 86400 seconds = 1 day)
- Reports at interval boundaries
- Can defer until block is irreversible

**Report Content**:
- Block number
- Total block log size
- Running SHA-256 hash
- Timestamp (implicit from block)

## Configuration

### Config File Options

```ini
# Enable the plugin
plugin = block_log_info

# Print interval in seconds (default: 86400 = 1 day)
block-log-info-print-interval-seconds = 86400

# Wait for irreversibility before printing (default: true)
block-log-info-print-irreversible = true

# Output destination (default: ILOG)
# Options: ILOG, STDOUT, STDERR, filename
block-log-info-print-file = ILOG
```

### Command Line Options

```bash
# Basic usage (default settings)
steemd --plugin=block_log_info

# Custom interval (hourly)
steemd --plugin=block_log_info \
  --block-log-info-print-interval-seconds=3600

# Print immediately (not wait for irreversible)
steemd --plugin=block_log_info \
  --block-log-info-print-irreversible=false

# Output to file
steemd --plugin=block_log_info \
  --block-log-info-print-file=/var/log/block-log-info.log

# Output to console
steemd --plugin=block_log_info \
  --block-log-info-print-file=STDOUT
```

### Configuration Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `block-log-info-print-interval-seconds` | int32 | 86400 | Report interval in seconds (1 day) |
| `block-log-info-print-irreversible` | bool | true | Wait for irreversibility before printing |
| `block-log-info-print-file` | string | ILOG | Output destination |

## Output Destinations

### ILOG (Default)

**Standard logging system**:
```ini
block-log-info-print-file = ILOG
```

**Output**:
```
block_num=12345678   size=27453892100   hash=a1b2c3d4e5f6...
```

**Location**: Standard steemd logs

### STDOUT

**Console output**:
```ini
block-log-info-print-file = STDOUT
```

**Use Case**: Script processing, monitoring systems

### STDERR

**Error stream output**:
```ini
block-log-info-print-file = STDERR
```

**Use Case**: Separating info from standard output

### File

**Dedicated log file**:
```ini
block-log-info-print-file = /var/log/steem/block-log-info.log
```

**Features**:
- Dedicated file for block log info
- Append mode (preserves history)
- Easy to process separately

### Disable

**No output** (calculate but don't print):
```ini
block-log-info-print-file =
```

**Use Case**: Testing, when only internal state needed

## Output Format

### Report Line Format

```
block_num=<number>   size=<bytes>   hash=<hex_hash>
```

**Example**:
```
block_num=12345678   size=27453892100   hash=a1b2c3d4e5f6789012345678901234567890123456789012345678901234
```

### Fields

| Field | Description | Example |
|-------|-------------|---------|
| `block_num` | Block number at report time | 12345678 |
| `size` | Cumulative block log size in bytes | 27453892100 |
| `hash` | SHA-256 hash (hex) | a1b2c3d4... |

## Reporting Intervals

### Time-Based Intervals

**How intervals work**:

```
Interval = 86400 seconds (1 day)

Block timestamp: 2024-01-15 12:30:45
Current interval: floor(timestamp / 86400)

When interval changes → generate report
```

**Example** (1 hour intervals):
```
block-log-info-print-interval-seconds = 3600

Reports at:
- 2024-01-15 01:00:00 (interval 12345)
- 2024-01-15 02:00:00 (interval 12346)
- 2024-01-15 03:00:00 (interval 12347)
...
```

### Irreversibility Delay

**With irreversibility check** (default):
```ini
block-log-info-print-irreversible = true
```

**Behavior**:
- Report generated when interval changes
- Held in memory until block is irreversible
- Printed once block becomes irreversible
- Ensures reported data is final

**Without irreversibility check**:
```ini
block-log-info-print-irreversible = false
```

**Behavior**:
- Report printed immediately when interval changes
- Faster reporting
- Risk: Could report reversible blocks

## Use Cases

### Block Log Verification

**Purpose**: Verify block logs match across nodes

**Setup**:
```ini
plugin = block_log_info
block-log-info-print-interval-seconds = 3600
block-log-info-print-file = /var/log/block-log-verification.log
```

**Verification**:
```bash
# Compare logs from two nodes
diff node1-block-log-info.log node2-block-log-info.log

# Should be identical if block logs match
```

### Corruption Detection

**Purpose**: Detect block log corruption

**Process**:
1. Record hash at known good state
2. Periodically compare current hash
3. Mismatch indicates corruption

**Example**:
```bash
# Known good state
block_num=10000000   size=15GB   hash=abc123...

# Later check shows different hash at same block
# → Corruption detected
```

### Synchronization Monitoring

**Purpose**: Ensure multiple nodes stay synchronized

**Setup**:
```ini
# Same configuration on all nodes
block-log-info-print-interval-seconds = 3600
block-log-info-print-file = STDOUT
```

**Monitoring**:
```bash
# Collect from all nodes
ssh node1 "tail -n 1 block-log-info.log"
ssh node2 "tail -n 1 block-log-info.log"
ssh node3 "tail -n 1 block-log-info.log"

# Compare - should be identical
```

### Debugging Block Log Issues

**Purpose**: Troubleshoot block log problems

**Use Cases**:
- Identify when corruption occurred
- Compare before/after replay
- Verify backup integrity
- Investigate sync issues

### Audit Trail

**Purpose**: Create verifiable checkpoints

**Configuration**:
```ini
block-log-info-print-interval-seconds = 86400  # Daily
block-log-info-print-file = /audit/block-log-checkpoints.log
```

**Benefits**:
- Permanent record
- Cryptographic verification
- Compliance documentation
- Historical reference

## Performance Impact

### Resource Usage

| Resource | Impact |
|----------|--------|
| **CPU** | Very Low (incremental hash update) |
| **Memory** | Minimal (1-5 MB for state) |
| **Disk I/O** | None (except report writes) |
| **Network** | None |

### Performance Characteristics

**Per block overhead**:
- Hash update: ~1-10 µs
- Size tracking: Negligible
- Total impact: < 0.01% of block processing

**Report generation**:
- Hash finalization: < 1 ms
- File write: < 1 ms
- Minimal impact on block processing

## Monitoring

### Log Messages

**Initialization**:
```
Initializing block_log_info plugin
```

**Genesis block**:
```
# First block initializes state
```

**Interval reports**:
```
block_num=86400   size=123456789   hash=a1b2c3d4...
block_num=172800  size=246813578   hash=e5f6a7b8...
```

**Warnings**:
```
print_interval_seconds set to value <= 0, if you don't need printing,
consider disabling block_log_info_plugin entirely to improve performance
```

### Health Checks

**Good indicators**:
- Regular reports at expected intervals
- Hash values changing (indicates blocks processing)
- Size increasing monotonically
- No error messages

**Warning signs**:
- No reports generated
- Hash not changing
- Size not increasing
- Error messages in logs

## Troubleshooting

### No Reports Generated

**Problem**: Plugin enabled but no output

**Symptoms**:
- Plugin initializes successfully
- No report lines in output

**Causes**:
1. Interval too long (no interval passed yet)
2. Output disabled (`block-log-info-print-file` empty)
3. Blocks not becoming irreversible (with irreversibility check)
4. Interval set to 0 or negative

**Solutions**:

1. **Reduce interval**:
   ```ini
   block-log-info-print-interval-seconds = 60  # 1 minute for testing
   ```

2. **Check output destination**:
   ```ini
   block-log-info-print-file = STDOUT  # Ensure output enabled
   ```

3. **Disable irreversibility check** (for testing):
   ```ini
   block-log-info-print-irreversible = false
   ```

4. **Verify interval > 0**:
   ```ini
   block-log-info-print-interval-seconds = 3600  # Positive value
   ```

### Hash Mismatch Between Nodes

**Problem**: Different hashes for same block number

**Symptoms**:
```
Node 1: block_num=10000   hash=abc123...
Node 2: block_num=10000   hash=def456...  # Different!
```

**Causes**:
- Block log corruption on one node
- Different block history (fork)
- Database corruption
- Plugin started at different points

**Investigation**:

1. **Compare block logs directly**:
   ```bash
   diff block_log_1 block_log_2
   ```

2. **Check for forks**: Verify same blockchain

3. **Replay from clean state**: Both nodes from genesis

4. **Verify block log integrity**:
   ```bash
   # Use block_log utility
   block_log --verify
   ```

### Unexpected Size Changes

**Problem**: Block log size doesn't match expectations

**Causes**:
- Block log rebuilt/truncated
- Node replayed
- Different starting point

**Normal behavior**:
- Size always increases
- Increases by variable amounts (block size varies)

**Abnormal behavior**:
- Size decreases (indicates replay/rebuild)
- Size not increasing (node stuck)

## Best Practices

### For Node Operators

1. **Regular monitoring**: Check reports periodically
2. **Archive reports**: Save historical data
3. **Compare nodes**: Verify consistency across infrastructure
4. **Reasonable intervals**: Don't set too frequent (adds overhead)
5. **Irreversibility**: Use default (true) for production

### For Developers

1. **Testing**: Use short intervals for development
2. **Debugging**: Output to STDOUT for immediate feedback
3. **Automation**: Parse output for automated checks
4. **Documentation**: Document expected hash values

### For Auditors

1. **Checkpoint recording**: Maintain hash checkpoints
2. **Periodic verification**: Regular consistency checks
3. **Multiple sources**: Compare against multiple nodes
4. **Documentation**: Keep audit trail of verifications

## Report Processing

### Parse Reports

**Bash**:
```bash
# Extract latest hash
tail -n 1 block-log-info.log | grep -oP 'hash=\K[a-f0-9]+'

# Extract block number
tail -n 1 block-log-info.log | grep -oP 'block_num=\K[0-9]+'

# Extract size
tail -n 1 block-log-info.log | grep -oP 'size=\K[0-9]+'
```

**Python**:
```python
import re

def parse_report(line):
    block_num = int(re.search(r'block_num=(\d+)', line).group(1))
    size = int(re.search(r'size=(\d+)', line).group(1))
    hash_val = re.search(r'hash=([a-f0-9]+)', line).group(1)
    return {'block_num': block_num, 'size': size, 'hash': hash_val}

with open('block-log-info.log') as f:
    for line in f:
        report = parse_report(line)
        print(report)
```

### Automated Monitoring

**Monitoring script**:
```bash
#!/bin/bash
# monitor-block-log.sh

LOG_FILE="/var/log/steem/block-log-info.log"
ALERT_EMAIL="admin@example.com"

# Get latest report
LATEST=$(tail -n 1 "$LOG_FILE")

# Extract hash
HASH=$(echo "$LATEST" | grep -oP 'hash=\K[a-f0-9]+')

# Compare with known good state
if [ "$HASH" != "$EXPECTED_HASH" ]; then
    echo "Hash mismatch detected!" | mail -s "Block Log Alert" "$ALERT_EMAIL"
fi
```

### Comparison Tools

**Compare two nodes**:
```bash
#!/bin/bash
# compare-nodes.sh

NODE1_LOG="/data/node1/block-log-info.log"
NODE2_LOG="/data/node2/block-log-info.log"

# Compare latest reports
DIFF=$(diff <(tail -n 1 "$NODE1_LOG") <(tail -n 1 "$NODE2_LOG"))

if [ -n "$DIFF" ]; then
    echo "Nodes out of sync!"
    echo "$DIFF"
    exit 1
else
    echo "Nodes in sync"
    exit 0
fi
```

## Database Objects

### block_log_hash_state_object

**Stores running hash state**:

```cpp
struct block_log_hash_state_object {
    uint64_t     total_size;       // Cumulative size
    ripemd_hash  rsha256;          // Running SHA-256 hash
    uint64_t     last_interval;    // Last reported interval
};
```

### block_log_pending_message_object

**Stores pending reports** (when waiting for irreversibility):

```cpp
struct block_log_pending_message_object {
    block_log_message_data data;   // Report data
};
```

**Pending messages**:
- Created when interval changes
- Held until block is irreversible
- Printed and removed when irreversible
- Ensures only final data reported

## Related Documentation

- [chain Plugin](chain.md) - Core blockchain database
- [Node Modes Guide](../operations/node-modes-guide.md) - Configuration by use case

## Source Code

- **Implementation**: [src/plugins/block_log_info/block_log_info_plugin.cpp](../../src/plugins/block_log_info/block_log_info_plugin.cpp)
- **Objects**: [src/plugins/block_log_info/include/steem/plugins/block_log_info/block_log_info_objects.hpp](../../src/plugins/block_log_info/include/steem/plugins/block_log_info/block_log_info_objects.hpp)
- **Plugin Header**: [src/plugins/block_log_info/include/steem/plugins/block_log_info/block_log_info_plugin.hpp](../../src/plugins/block_log_info/include/steem/plugins/block_log_info/block_log_info_plugin.hpp)

## License

See [LICENSE.md](../../LICENSE.md)
