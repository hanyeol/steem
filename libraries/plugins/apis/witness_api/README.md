# Witness API Plugin

Witness management and operations.

## Overview

Provides witness-specific operations and queries.

## Configuration

```ini
plugin = witness_api

# Requires witness plugin for block production
plugin = witness
```

## Methods

- `get_active_witnesses` - Current witness schedule
- `get_witness_schedule` - Full witness schedule
- `get_witness_by_account` - Witness data lookup

## Usage

Primarily used by witness nodes for:
- Schedule monitoring
- Witness updates
- Feed publishing

## Dependencies

- `witness` (optional, for witness operations)
- `json_rpc`
- `webserver`
