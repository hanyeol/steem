# Block Data Export Plugin

Exports block data to external storage.

## Overview

The block_data_export plugin:
- Exports block data to files
- Supports custom export formats
- Enables external data processing
- Facilitates blockchain analysis

## Configuration

```ini
plugin = block_data_export

# Export directory
# block-data-export-dir = "export"
```

## Features

- **Block Export**: Write blocks to disk
- **Custom Formats**: Configurable output format
- **Continuous Export**: Real-time as blocks arrive
- **Historical Export**: Replay and export

## Use Cases

- Data warehousing
- External analytics
- Blockchain backups
- Custom indexing

## Dependencies

- `chain` - Block data access
