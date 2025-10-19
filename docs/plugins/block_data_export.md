# block_data_export Plugin

Exports structured block data to JSON files for external processing, analytics, and auditing.

## Overview

The `block_data_export` plugin provides a framework for extracting and exporting blockchain data to external files. It allows other plugins to register custom data exporters and outputs the collected data as JSON records, one per block. This is useful for data warehousing, analytics pipelines, compliance auditing, and blockchain research.

**Plugin Type**: Utility
**Dependencies**: [chain](chain.md)
**Memory**: Minimal (~50-100MB)
**CPU**: Moderate (JSON conversion overhead)
**Default**: Disabled (enable explicitly)

## Purpose

- **Data Export**: Extract blockchain data to external systems
- **Analytics Pipeline**: Feed data to analytics/BI tools
- **Audit Trail**: Create permanent records for compliance
- **Research**: Enable blockchain analysis and research
- **Backup**: Create structured backups of blockchain data
- **Integration**: Feed data to external databases or applications

## How It Works

### Export Flow

```
Block applied
    ↓
Pre-apply hook: Create export data objects
    ↓
Other plugins populate export data
    ↓
Post-apply hook: Collect all export data
    ↓
Convert to JSON (parallel threads)
    ↓
Write to output file (sequential)
    ↓
Next block
```

### Threading Model

**Multi-threaded architecture**:

1. **Main thread**: Block processing
2. **Conversion threads**: JSON serialization (N+1 threads, where N = CPU cores)
3. **Output thread**: File writing (single thread)

**Queues**:
- **Data queue**: Blocks waiting for JSON conversion
- **Output queue**: JSON waiting to be written

**Benefits**:
- Non-blocking block processing
- Parallel JSON conversion
- Sequential file writes (data consistency)

### Extensibility

**Plugin Registration System**:

Other plugins can register "export data factories":

```cpp
// In another plugin
plugin.register_export_data_factory("my_data", []() {
    return std::make_shared<my_export_data>();
});
```

**Export Data Lifecycle**:

1. **Pre-apply**: Factory creates empty export data object
2. **Processing**: Other plugins populate the object
3. **Post-apply**: Object collected and queued for export
4. **Conversion**: Serialized to JSON
5. **Output**: Written to file

## Configuration

### Config File Options

```ini
# Enable the plugin
plugin = block_data_export

# Output file path (NONE to disable, effectively disabling the plugin)
block-data-export-file = /path/to/export/blockchain-data.json

# Disable export (default)
# block-data-export-file = NONE
```

### Command Line Options

```bash
# Enable with file output
steemd --plugin=block_data_export \
  --block-data-export-file=/data/blockchain-export.json

# Disable export
steemd --plugin=block_data_export \
  --block-data-export-file=NONE
```

### Configuration Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `block-data-export-file` | string | NONE | Output file path (NONE disables export) |

## Output Format

### JSON Structure

Each line in the output file is a JSON object representing one block:

```json
{
  "block_id": "00bc614a7b89a44da0a6d0faa43b801e8edaa823",
  "previous": "00bc61499f1e0c8f7d25e95f7e8a8efbdf7c7d1a",
  "export_data": {
    "my_exporter": {
      "custom_field_1": "value",
      "custom_field_2": 123,
      "nested_data": {
        "field": "value"
      }
    },
    "another_exporter": {
      "data": [1, 2, 3, 4, 5]
    }
  }
}
```

### Standard Fields

**Always present**:
- `block_id`: Hash of current block
- `previous`: Hash of previous block
- `export_data`: Object containing all registered export data

**Export data fields**: Depend on which plugins registered exporters

### File Format

**Line-delimited JSON (JSONL)**:
- One JSON object per line
- No comma between objects
- Easy to stream process
- Compatible with most data tools

**Example file**:
```json
{"block_id":"00000001...", "previous":"00000000...", "export_data":{}}
{"block_id":"00000002...", "previous":"00000001...", "export_data":{}}
{"block_id":"00000003...", "previous":"00000002...", "export_data":{}}
```

## Use Cases

### Analytics Pipeline

**Purpose**: Feed blockchain data to analytics systems

**Configuration**:
```ini
plugin = block_data_export
block-data-export-file = /data/blockchain-export.jsonl
```

**Processing**:
```bash
# Stream to analytics system
tail -f /data/blockchain-export.jsonl | process-stream.sh

# Load into database
cat blockchain-export.jsonl | mongoimport --collection blocks

# Process with jq
cat blockchain-export.jsonl | jq '.export_data.my_field'
```

### Compliance Auditing

**Purpose**: Create permanent audit trail

**Configuration**:
```ini
plugin = block_data_export
block-data-export-file = /audit/blockchain-audit-trail.jsonl
```

**Features**:
- Immutable record
- Complete block information
- Timestamped entries
- Easy to verify against blockchain

### Research and Analysis

**Purpose**: Enable blockchain research

**Data Analysis**:
```python
import json

# Analyze export data
with open('blockchain-export.jsonl') as f:
    for line in f:
        block = json.loads(line)
        # Analyze block data
        analyze(block['export_data'])
```

### Backup and Archival

**Purpose**: Structured blockchain backup

**Advantages over block log**:
- Human-readable format
- Queryable with standard tools
- Easy to import into databases
- Selective data export

### External System Integration

**Purpose**: Feed data to external systems

**Examples**:
- Data warehouses
- Search engines
- Monitoring systems
- Notification services

## Performance Impact

### Resource Usage

| Resource | Impact |
|----------|--------|
| **CPU** | Moderate (JSON serialization) |
| **Memory** | Low (~100MB for queues) |
| **Disk I/O** | Moderate (continuous writes) |
| **Network** | None |

### Performance Characteristics

**Queue Sizes**: Max 100 blocks in each queue
**Thread Count**: CPU cores + 2 (conversion + output threads)
**Write Rate**: ~1-10 blocks/second (depends on data size)

**Bottlenecks**:
- JSON conversion (CPU-bound)
- Disk write speed (I/O-bound)
- Export data complexity

### Optimization Tips

1. **Use SSD**: Faster write speeds
2. **Dedicated disk**: Avoid I/O contention
3. **Minimize export data**: Only export necessary fields
4. **Monitor queue depth**: Shouldn't stay at max (100)

## Monitoring

### Log Messages

**Initialization**:
```
Initializing block_data_export plugin
```

**Normal Operation**:
```
# No routine log messages
# Plugin operates silently when working correctly
```

**Shutdown**:
```
# Queues closed, threads joined
```

### Health Checks

**Good indicators**:
- Export file growing steadily
- Queue sizes low (< 50)
- CPU usage moderate
- No error messages

**Warning signs**:
- Export file not growing
- Queues always full (100/100)
- High CPU usage
- Disk I/O errors

### Monitoring Commands

```bash
# Watch file growth
watch -n 5 'ls -lh /path/to/export.jsonl'

# Count exported blocks
wc -l /path/to/export.jsonl

# Monitor disk space
df -h /mount/point

# Tail recent exports
tail -f /path/to/export.jsonl

# Verify JSON format
tail -n 10 /path/to/export.jsonl | jq .
```

## Troubleshooting

### Export File Not Growing

**Problem**: File created but not being written to

**Symptoms**:
- File exists but empty or not changing
- Blockchain syncing normally

**Causes**:
- Plugin enabled but no exporters registered
- File path invalid/permission denied
- Plugin initialized but not started

**Solutions**:

1. **Check file path**:
   ```bash
   ls -l /path/to/export.jsonl
   # Verify permissions
   ```

2. **Verify plugin configuration**:
   ```ini
   # Ensure not set to NONE
   block-data-export-file = /valid/path.jsonl
   ```

3. **Check logs**: Look for initialization errors

4. **Restart node**: Ensure plugin fully initialized

### Queue Overflow

**Problem**: Data queues consistently full

**Symptoms**:
```
# Queues at max capacity (100)
# Blocks waiting to be processed
# Possible memory growth
```

**Causes**:
- Slow JSON conversion
- Slow disk writes
- Complex export data
- Insufficient CPU

**Solutions**:

1. **Reduce export data complexity**: Remove unnecessary fields

2. **Use faster storage**: Switch to SSD

3. **Monitor CPU**: Ensure adequate processing power

4. **Increase queue size** (requires code modification)

### Disk Space Exhausted

**Problem**: Export file fills disk

**Symptoms**:
```
No space left on device
Write failed
```

**Solutions**:

1. **Rotate export files**:
   ```bash
   # Stop node
   mv export.jsonl export-backup.jsonl
   # Start node (creates new file)
   ```

2. **Compress old exports**:
   ```bash
   gzip export-old.jsonl
   ```

3. **Stream to external system**: Instead of local file

4. **Add disk space**: Increase storage

### Invalid JSON Output

**Problem**: Malformed JSON in export file

**Symptoms**:
```bash
# jq fails to parse
jq . export.jsonl
parse error: ...
```

**Causes**:
- Export data serialization error
- Disk write error
- Process interrupted mid-write

**Solutions**:

1. **Validate export data**: Check registered exporters

2. **Check disk health**: Run disk diagnostics

3. **Verify file integrity**: Look for partial writes

4. **Rebuild export**: Replay from last good block

## Integration with Other Plugins

### Registering Export Data

**Example plugin integration**:

```cpp
// In another plugin's plugin_initialize()
auto& export_plugin = appbase::app().get_plugin<block_data_export_plugin>();

// Register a factory
export_plugin.register_export_data_factory("my_data", []() {
    return std::make_shared<my_exportable_data>();
});

// In block processing
auto export_data = export_plugin.find_export_data<my_exportable_data>("my_data");
if (export_data) {
    export_data->add_data(my_custom_data);
}
```

### Exportable Data Requirements

**Custom export data must**:

1. **Inherit from** `exportable_block_data`
2. **Be serializable** (FC_REFLECT macros)
3. **Provide default constructor**

**Example**:

```cpp
class my_export_data : public exportable_block_data {
public:
    std::string custom_field;
    uint64_t counter;

    my_export_data() : counter(0) {}
};

FC_REFLECT(my_export_data, (custom_field)(counter))
```

## Best Practices

### For Node Operators

1. **Dedicated storage**: Use separate disk for exports
2. **Monitor space**: Set up disk usage alerts
3. **Regular rotation**: Rotate/archive old export files
4. **Verify exports**: Periodically validate JSON format
5. **Backup strategy**: Include exports in backup plan

### For Plugin Developers

1. **Minimal data**: Only export necessary information
2. **Efficient serialization**: Avoid complex nested structures
3. **Factory pattern**: Use factories for object creation
4. **Thread safety**: Export data objects must be thread-safe
5. **Documentation**: Document export data format

### For Data Consumers

1. **Stream processing**: Process data as it arrives
2. **Error handling**: Handle malformed JSON gracefully
3. **Validate format**: Verify JSON structure
4. **Batch processing**: Process in batches for efficiency
5. **Idempotency**: Handle duplicate blocks

## File Management

### Log Rotation

**Manual rotation**:
```bash
#!/bin/bash
# rotate-export.sh

# Stop steemd
systemctl stop steemd

# Rotate file
mv /data/export.jsonl /data/export-$(date +%Y%m%d).jsonl

# Start steemd (creates new file)
systemctl start steemd

# Compress old file
gzip /data/export-*.jsonl
```

**Logrotate configuration**:
```
/data/blockchain-export.jsonl {
    daily
    rotate 30
    compress
    delaycompress
    notifempty
    create 0644 steem steem
    postrotate
        systemctl restart steemd
    endscript
}
```

### Compression

**Compress old exports**:
```bash
# Compress
gzip export-old.jsonl

# Decompress for analysis
gunzip export-old.jsonl.gz

# Analyze compressed file directly
zcat export-old.jsonl.gz | jq .
```

**Compression Ratio**: Typically 5-10x (JSONL compresses well)

## Data Processing Examples

### Extract Specific Fields

```bash
# Extract block IDs
cat export.jsonl | jq -r '.block_id'

# Extract custom export data
cat export.jsonl | jq '.export_data.my_field'

# Filter by condition
cat export.jsonl | jq 'select(.export_data.counter > 100)'
```

### Import to Database

**MongoDB**:
```bash
mongoimport --db blockchain --collection blocks \
  --file export.jsonl
```

**PostgreSQL** (with jq preprocessing):
```bash
cat export.jsonl | jq -c '{
  block_id: .block_id,
  data: .export_data
}' | psql -c "COPY blocks FROM STDIN"
```

### Stream Processing

**Real-time processing**:
```bash
tail -f export.jsonl | while read line; do
    echo "$line" | jq '.export_data' | process-data.sh
done
```

## Security Considerations

### File Permissions

**Recommended permissions**:
```bash
# Export file should be readable only by necessary users
chmod 640 /path/to/export.jsonl
chown steem:steem /path/to/export.jsonl
```

### Data Privacy

**Considerations**:
- Export data is unencrypted
- Contains blockchain transaction details
- May include sensitive information
- Secure file access appropriately

**Recommendations**:
1. Restrict file access
2. Encrypt at rest (if needed)
3. Secure transfer to external systems
4. Comply with data protection regulations

## Related Documentation

- [chain Plugin](chain.md) - Core blockchain database
- [Node Types Guide](../operations/node-types-guide.md) - Configuration by use case

## Source Code

- **Implementation**: [libraries/plugins/block_data_export/block_data_export_plugin.cpp](../../libraries/plugins/block_data_export/block_data_export_plugin.cpp)
- **Plugin Header**: [libraries/plugins/block_data_export/include/steem/plugins/block_data_export/block_data_export_plugin.hpp](../../libraries/plugins/block_data_export/include/steem/plugins/block_data_export/block_data_export_plugin.hpp)
- **Exportable Data**: [libraries/plugins/block_data_export/include/steem/plugins/block_data_export/exportable_block_data.hpp](../../libraries/plugins/block_data_export/include/steem/plugins/block_data_export/exportable_block_data.hpp)

## License

See [LICENSE.md](../../LICENSE.md)
