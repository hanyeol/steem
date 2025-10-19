# stats_export Plugin

Export blockchain statistics and transaction data for external analysis and monitoring.

## Overview

The `stats_export` plugin collects blockchain statistics on a per-block basis and exports them through the block data export framework. It tracks global properties, transaction metadata, and memory usage for analysis and monitoring.

**Plugin Type**: Monitoring/Export
**Dependencies**: [chain](chain.md), [block_data_export](block_data_export.md)
**Memory**: Minimal (~50MB)
**Default**: Disabled

## Purpose

- **Transaction Statistics**: Track transaction sizes and authors per block
- **Global Properties**: Export dynamic global properties (supply, rewards, etc.)
- **Memory Monitoring**: Track database free memory
- **Data Export**: Provide statistics to external systems
- **Performance Analysis**: Enable blockchain performance monitoring

## Configuration

### Config File Options

```ini
# Enable stats export
plugin = block_data_export stats_export

# Block data export is required for stats_export
# Configure export destination
block-data-export-file = /path/to/export/stats.json
```

### Command Line Options

```bash
steemd \
  --plugin=block_data_export \
  --plugin=stats_export \
  --block-data-export-file=/var/log/steem/stats.json
```

### Configuration Parameters

The stats_export plugin itself has no direct configuration options. It relies on the `block_data_export` plugin for export configuration.

| Parameter | Plugin | Default | Description |
|-----------|--------|---------|-------------|
| `block-data-export-file` | block_data_export | (none) | File path for exported statistics |
| `block-data-export-endpoint` | block_data_export | (none) | HTTP endpoint for stats push |

## Exported Data

### Data Structure

Per block, the plugin exports:

```json
{
  "global_properties": {
    "head_block_number": 12345678,
    "head_block_id": "00bc614e...",
    "time": "2023-01-15T12:34:56",
    "current_witness": "witness-name",
    "total_pow": 0,
    "num_pow_witnesses": 0,
    "virtual_supply": "...",
    "current_supply": "...",
    "confidential_supply": "...",
    "current_sbd_supply": "...",
    "confidential_sbd_supply": "...",
    "total_vesting_fund_steem": "...",
    "total_vesting_shares": "...",
    "total_reward_fund_steem": "...",
    "total_reward_shares2": "...",
    "pending_rewarded_vesting_shares": "...",
    "pending_rewarded_vesting_steem": "...",
    "sbd_interest_rate": 0,
    "sbd_print_rate": 10000,
    "maximum_block_size": 65536,
    "current_aslot": 12345678,
    "recent_slots_filled": "340282366920938463463374607431768211455",
    "participation_count": 128,
    "last_irreversible_block_num": 12345658,
    "vote_power_reserve_rate": 10,
    "delegation_return_period": 432000,
    "reverse_auction_seconds": 300,
    "available_account_subsidies": 9876543,
    "sbd_stop_percent": 1000,
    "sbd_start_percent": 900,
    "next_maintenance_time": "2023-01-15T13:00:00",
    "last_budget_time": "2023-01-15T12:00:00",
    "content_reward_percent": 7500,
    "vesting_reward_percent": 1500,
    "sps_fund_percent": 1000,
    "sps_interval_ledger": "..."
  },
  "transaction_stats": [
    {
      "user": "alice",
      "size": 256
    },
    {
      "user": "bob",
      "size": 384
    }
  ],
  "free_memory": 85899345920
}
```

### Transaction Statistics

For each transaction in the block:
- **user**: Account that signed the transaction (posting > active > owner priority)
- **size**: Transaction size in bytes (serialized)

### Global Properties

Complete snapshot of `dynamic_global_property_object` including:
- Block metadata (number, ID, timestamp, witness)
- Supply information (STEEM, SBD, vesting)
- Reward pools and distribution
- Network parameters (block size, participation)
- Hardfork and consensus parameters

### Memory Statistics

- **free_memory**: Available memory in shared memory file (bytes)

## Use Cases

### Blockchain Analytics

**Purpose**: Analyze blockchain growth and usage

```ini
plugin = chain p2p block_data_export stats_export

# Export to file for analysis
block-data-export-file = /data/analytics/steem-stats.jsonl

# Process with external tools
# - Transaction volume analysis
# - User activity tracking
# - Supply/inflation monitoring
```

### Performance Monitoring

**Purpose**: Monitor node performance over time

```ini
plugin = chain block_data_export stats_export

# Export to monitoring system
block-data-export-endpoint = http://monitoring.local:8080/ingest

# Track metrics:
# - Free memory trends
# - Transaction sizes
# - Block sizes
# - Network participation
```

### Transaction Analysis

**Purpose**: Track transaction authors and sizes

```bash
# Export transaction statistics
cat /data/steem-stats.jsonl | jq '.transaction_stats[]'

# Find largest transactions
cat /data/steem-stats.jsonl | jq '[.transaction_stats[] | select(.size > 1000)]'

# Count transactions by user
cat /data/steem-stats.jsonl | jq '.transaction_stats[].user' | sort | uniq -c
```

### Supply Tracking

**Purpose**: Monitor STEEM/SBD supply changes

```bash
# Track supply over time
cat /data/steem-stats.jsonl | jq '{
  block: .global_properties.head_block_number,
  steem: .global_properties.current_supply,
  sbd: .global_properties.current_sbd_supply
}'

# Calculate daily supply changes
# Parse and compare global_properties.current_supply
```

## Monitoring

### Export Verification

**Check exports are working**:
```bash
# For file export
tail -f /data/steem-stats.jsonl

# Verify JSON structure
tail -1 /data/steem-stats.jsonl | jq .

# Count exported blocks
wc -l /data/steem-stats.jsonl
```

### Data Quality

**Verify data completeness**:
```bash
# Check all expected fields present
tail -1 /data/steem-stats.jsonl | jq 'has("global_properties") and has("transaction_stats") and has("free_memory")'

# Check transaction stats populated
tail -1 /data/steem-stats.jsonl | jq '.transaction_stats | length'
```

### Performance Impact

**Minimal overhead**:
- Reads existing database state
- No additional indexes
- Lightweight serialization
- Exports after block applied

## Troubleshooting

### No Stats Exported

**Problem**: Statistics not appearing in export file

**Check**:
```bash
# Verify both plugins enabled
grep "plugin =" config.ini | grep -E "(block_data_export|stats_export)"

# Check export file path writable
ls -la /path/to/export/

# Verify plugin initialization
grep "Initializing stats_export" logs/stderr.txt
```

**Solutions**:
1. Enable `block_data_export` plugin
2. Configure `block-data-export-file` or `block-data-export-endpoint`
3. Ensure write permissions on export path
4. Check disk space available

### Transaction User Empty

**Problem**: Transaction stats show empty user field

**Cause**: Transaction has no required authorities

**Explanation**:
- Some operations may not require explicit authorities
- Multi-sig transactions may have complex authority structure
- Empty user field indicates no posting/active/owner auth required

### Memory Statistics Zero

**Problem**: `free_memory` always shows 0

**Check**:
```bash
# Verify database opened correctly
grep "Opened database" logs/stderr.txt

# Check shared memory file
ls -lh witness_node_data_dir/blockchain/shared_memory.bin
```

**Solutions**:
1. Ensure database initialized properly
2. Check shared memory configuration
3. Verify sufficient RAM available

## Performance Tuning

### Export Frequency

Statistics exported per block (every 3 seconds):

```bash
# For high-volume analysis, consider:
# - Buffering exports (write every N blocks)
# - Compressing export files
# - Rotating export files regularly

# Log rotation example
logrotate -f /etc/logrotate.d/steem-stats
```

### Disk Usage

**Estimate storage requirements**:

```bash
# Average block stats: ~5-10 KB per block
# Blocks per day: 28,800
# Daily storage: 144-288 MB
# Monthly storage: ~4-8 GB

# Use compression for long-term storage
gzip /data/steem-stats-2023-01.jsonl
```

## Integration Examples

### Prometheus Metrics

Export to Prometheus-compatible format:

```bash
# Parse stats and expose metrics
cat /data/steem-stats.jsonl | jq -r '
  "steem_head_block_number " + (.global_properties.head_block_number | tostring),
  "steem_free_memory_bytes " + (.free_memory | tostring),
  "steem_transaction_count " + (.transaction_stats | length | tostring)
'
```

### Time-Series Database

Import into InfluxDB/TimescaleDB:

```python
# Example Python script
import json
from influxdb import InfluxDBClient

client = InfluxDBClient(host='localhost', port=8086)
client.switch_database('steem_stats')

with open('/data/steem-stats.jsonl') as f:
    for line in f:
        data = json.loads(line)
        point = {
            "measurement": "blockchain_stats",
            "time": data['global_properties']['time'],
            "fields": {
                "block_num": data['global_properties']['head_block_number'],
                "free_memory": data['free_memory'],
                "tx_count": len(data['transaction_stats'])
            }
        }
        client.write_points([point])
```

## Security Considerations

### Data Exposure

Statistics contain public blockchain data:
- No private keys or sensitive information
- All data is publicly verifiable on-chain
- Transaction sizes may indicate activity patterns

### Access Control

**Protect export files**:
```bash
# Restrict file permissions
chmod 600 /data/steem-stats.jsonl

# Use dedicated export user
chown steem-export:steem-export /data/steem-stats.jsonl
```

### Disk Space

**Monitor disk usage**:
```bash
# Alert on low disk space
df -h /data | awk 'NR==2 {if ($5+0 > 80) print "WARNING: Disk usage at " $5}'

# Implement automatic cleanup
find /data -name "steem-stats-*.jsonl" -mtime +30 -delete
```

## Related Documentation

- [block_data_export Plugin](block_data_export.md) - Export framework
- [chain Plugin](chain.md) - Core blockchain database
- [System Architecture](../technical-reference/system-architecture.md) - Architecture overview

## Source Code

- **Implementation**: [libraries/plugins/stats_export/stats_export_plugin.cpp](../../libraries/plugins/stats_export/stats_export_plugin.cpp)
- **Header**: [libraries/plugins/stats_export/include/steem/plugins/stats_export/stats_export_plugin.hpp](../../libraries/plugins/stats_export/include/steem/plugins/stats_export/stats_export_plugin.hpp)

## License

See [LICENSE.md](../../LICENSE.md)
