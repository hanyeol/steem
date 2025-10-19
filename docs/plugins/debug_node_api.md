# debug_node_api Plugin

Development and testing API for blockchain manipulation and debugging.

## Overview

The `debug_node_api` plugin provides API endpoints for debugging and testing blockchain behavior. It allows developers to manipulate blockchain state, generate blocks, test hardforks, and perform other operations that are useful during development but dangerous in production.

**Plugin Type**: API (Write Operations - Testing Only)
**Dependencies**: [debug_node](debug_node.md), [json_rpc](json_rpc.md)
**State Plugin**: debug_node
**Memory**: Minimal (~10MB)
**Default**: Not enabled

## WARNING

This plugin is **ONLY** for development and testing environments. Never enable it on production nodes, mainnet nodes, or any node connected to the public network.

**Security Risk**: This plugin can:
- Manipulate blockchain state
- Generate blocks at will
- Modify hardfork versions
- Break consensus with the network

## Purpose

- **Testing**: Test blockchain operations in controlled environment
- **Development**: Develop and test new features
- **Debugging**: Debug blockchain behavior and edge cases
- **Hardfork Testing**: Test hardfork activation and behavior
- **Block Generation**: Generate blocks for testing without waiting
- **State Manipulation**: Create specific blockchain states for testing

## Configuration

### Enable Plugin (Testnet Only)

```ini
# In config.ini - TESTNET ONLY
plugin = debug_node debug_node_api

# Required for debug functionality
enable-stale-production = true
required-participation = 0
```

### Command Line (Testnet Only)

```bash
# NEVER use these flags on mainnet
steemd --plugin="debug_node debug_node_api" \
  --enable-stale-production \
  --required-participation=0
```

### Build Requirements

The debug_node plugin requires building with testnet support:

```bash
cmake -DBUILD_STEEM_TESTNET=ON ..
make
```

## API Methods

### debug_generate_blocks

Generate a specific number of blocks.

**Parameters**:
```json
{
  "debug_key": "5JHzFSq3B7u...",
  "count": 10
}
```

**Returns**:
```json
{
  "blocks": 10
}
```

### debug_generate_blocks_until

Generate blocks until a specific head block time.

**Parameters**:
```json
{
  "debug_key": "5JHzFSq3B7u...",
  "head_block_time": "2023-12-31T23:59:59",
  "generate_sparsely": true
}
```

**Returns**:
```json
{
  "blocks": 42
}
```

**Parameters**:
- `debug_key`: Private key for block signing
- `head_block_time`: Target timestamp
- `generate_sparsely`: If true, only generate blocks at witness slots

### debug_push_blocks

Push blocks from an existing block log file.

**Parameters**:
```json
{
  "src_filename": "/path/to/block_log",
  "count": 1000,
  "skip_validate_invariants": false
}
```

**Returns**:
```json
{
  "blocks": 1000
}
```

### debug_pop_block

Remove the most recent block from the blockchain.

**Parameters**: None

**Returns**:
```json
{
  "block": {
    /* popped block object */
  }
}
```

### debug_get_witness_schedule

Get the current witness schedule (debug version).

**Parameters**: None

**Returns**: Witness schedule object.

### debug_get_hardfork_property_object

Get hardfork property information.

**Parameters**: None

**Returns**: Hardfork property object.

### debug_set_hardfork

Force activate a specific hardfork.

**Parameters**:
```json
{
  "hardfork_id": 20
}
```

**Returns**: None

### debug_has_hardfork

Check if a hardfork is active.

**Parameters**:
```json
{
  "hardfork_id": 20
}
```

**Returns**:
```json
{
  "has_hardfork": true
}
```

### debug_get_json_schema

Get JSON schema for all operations.

**Parameters**: None

**Returns**:
```json
{
  "schema": "/* JSON schema */"
}
```

## Usage Examples

### Generate Test Blocks

```bash
# Generate 10 blocks
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "debug_node_api.debug_generate_blocks",
  "params": {
    "debug_key": "5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n",
    "count": 10
  },
  "id": 1
}' | jq
```

### Generate Blocks Until Time

```bash
# Generate blocks until specific timestamp
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "debug_node_api.debug_generate_blocks_until",
  "params": {
    "debug_key": "5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n",
    "head_block_time": "2024-01-01T00:00:00",
    "generate_sparsely": true
  },
  "id": 1
}' | jq
```

### Pop Last Block

```bash
# Remove the most recent block
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "debug_node_api.debug_pop_block",
  "params": {},
  "id": 1
}' | jq
```

### Test Hardfork Activation

```bash
# Activate hardfork 20
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "debug_node_api.debug_set_hardfork",
  "params": {
    "hardfork_id": 20
  },
  "id": 1
}' | jq

# Check if active
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "debug_node_api.debug_has_hardfork",
  "params": {
    "hardfork_id": 20
  },
  "id": 1
}' | jq
```

### Replay Blocks from File

```bash
# Push 1000 blocks from block_log
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "debug_node_api.debug_push_blocks",
  "params": {
    "src_filename": "/path/to/block_log",
    "count": 1000,
    "skip_validate_invariants": false
  },
  "id": 1
}' | jq
```

## Development Workflows

### Test Transaction Processing

```python
import requests
import time

def test_transaction_after_blocks(tx, blocks=1):
    """Submit transaction and generate blocks to include it."""
    # Submit transaction
    payload = {
        "jsonrpc": "2.0",
        "method": "network_broadcast_api.broadcast_transaction",
        "params": {"trx": tx},
        "id": 1
    }
    requests.post("http://localhost:8090", json=payload)

    # Generate blocks to include transaction
    payload = {
        "jsonrpc": "2.0",
        "method": "debug_node_api.debug_generate_blocks",
        "params": {
            "debug_key": "5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n",
            "count": blocks
        },
        "id": 1
    }
    response = requests.post("http://localhost:8090", json=payload)

    return response.json()
```

### Time-Based Testing

```python
from datetime import datetime, timedelta

def advance_time(hours=1):
    """Advance blockchain time by generating blocks."""
    # Get current time
    dgp = get_dynamic_global_properties()
    current_time = datetime.fromisoformat(dgp["time"].replace('Z', ''))

    # Calculate target time
    target_time = current_time + timedelta(hours=hours)

    # Generate blocks until target time
    payload = {
        "jsonrpc": "2.0",
        "method": "debug_node_api.debug_generate_blocks_until",
        "params": {
            "debug_key": "5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n",
            "head_block_time": target_time.isoformat(),
            "generate_sparsely": True
        },
        "id": 1
    }

    response = requests.post("http://localhost:8090", json=payload)
    return response.json()

# Test vesting withdrawal (requires 7 days)
advance_time(hours=24 * 7)
```

### Test Hardfork Behavior

```python
def test_hardfork_feature(hardfork_id):
    """Test feature that activates at specific hardfork."""
    # Check current hardfork
    current = get_hardfork_properties()
    print(f"Current hardfork: {current['current_hardfork_version']}")

    # Activate target hardfork
    payload = {
        "jsonrpc": "2.0",
        "method": "debug_node_api.debug_set_hardfork",
        "params": {"hardfork_id": hardfork_id},
        "id": 1
    }
    requests.post("http://localhost:8090", json=payload)

    # Verify activation
    payload = {
        "jsonrpc": "2.0",
        "method": "debug_node_api.debug_has_hardfork",
        "params": {"hardfork_id": hardfork_id},
        "id": 1
    }
    response = requests.post("http://localhost:8090", json=payload)
    result = response.json()

    if result["result"]["has_hardfork"]:
        print(f"Hardfork {hardfork_id} activated successfully")
        # Test hardfork-specific feature here
    else:
        print(f"Failed to activate hardfork {hardfork_id}")
```

### Block Generation Helper

```python
class DebugBlockchain:
    def __init__(self, debug_key):
        self.debug_key = debug_key
        self.url = "http://localhost:8090"

    def generate_blocks(self, count):
        """Generate specified number of blocks."""
        payload = {
            "jsonrpc": "2.0",
            "method": "debug_node_api.debug_generate_blocks",
            "params": {
                "debug_key": self.debug_key,
                "count": count
            },
            "id": 1
        }

        response = requests.post(self.url, json=payload)
        return response.json()

    def generate_until(self, timestamp):
        """Generate blocks until specific time."""
        payload = {
            "jsonrpc": "2.0",
            "method": "debug_node_api.debug_generate_blocks_until",
            "params": {
                "debug_key": self.debug_key,
                "head_block_time": timestamp,
                "generate_sparsely": True
            },
            "id": 1
        }

        response = requests.post(self.url, json=payload)
        return response.json()

    def pop_block(self):
        """Remove last block."""
        payload = {
            "jsonrpc": "2.0",
            "method": "debug_node_api.debug_pop_block",
            "params": {},
            "id": 1
        }

        response = requests.post(self.url, json=payload)
        return response.json()

# Usage
blockchain = DebugBlockchain("5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n")
blockchain.generate_blocks(10)
```

## Testing Scenarios

### Test Voting Power Regeneration

```python
def test_voting_power_regen():
    """Test that voting power regenerates correctly."""
    # Submit vote
    submit_vote("alice", "bob", "test-post", 10000)

    # Generate 1 block to include vote
    blockchain.generate_blocks(1)

    # Check voting power reduced
    account = get_account("alice")
    vp_before = account["voting_power"]

    # Advance 1 day
    advance_time(hours=24)

    # Check voting power increased
    account = get_account("alice")
    vp_after = account["voting_power"]

    assert vp_after > vp_before
    print(f"VP before: {vp_before}, after: {vp_after}")
```

### Test Cashout Window

```python
def test_post_cashout():
    """Test post payout after 7 days."""
    # Create post
    create_post("alice", "test-cashout", "Title", "Body")
    blockchain.generate_blocks(1)

    # Get initial rewards
    post = get_content("alice", "test-cashout")
    pending_before = post["pending_payout_value"]

    # Advance 7 days
    advance_time(hours=24 * 7)

    # Check payout occurred
    post = get_content("alice", "test-cashout")
    total_payout = post["total_payout_value"]

    assert total_payout > 0
    print(f"Pending: {pending_before}, Paid: {total_payout}")
```

## Security Warnings

### Never Use in Production

```bash
# WRONG - Never do this
steemd --plugin="debug_node debug_node_api"  # On mainnet

# RIGHT - Only on testnet
steemd --plugin="debug_node debug_node_api" \
  --data-dir=/tmp/testnet \
  --p2p-endpoint=0.0.0.0:0  # No P2P connections
```

### Isolate Test Environment

```ini
# config.ini for isolated testnet
plugin = debug_node debug_node_api

# Disable P2P to prevent network connection
p2p-endpoint = 0.0.0.0:0

# Use separate data directory
data-dir = /tmp/steem-testnet

# Enable stale production for testing
enable-stale-production = true
required-participation = 0
```

### Access Control

Even on testnet, restrict access:

```nginx
# Only allow local access to debug APIs
location / {
    if ($request_body ~* "debug_node_api") {
        # Only allow from localhost
        allow 127.0.0.1;
        deny all;
    }
    proxy_pass http://localhost:8090;
}
```

## Troubleshooting

### Plugin Not Available

**Problem**: API returns "method not found"

**Causes**:
1. Not built with testnet support
2. Plugin not enabled
3. Running production build

**Solution**:
```bash
# Rebuild with testnet support
cmake -DBUILD_STEEM_TESTNET=ON ..
make
./programs/steemd/steemd --plugin="debug_node debug_node_api"
```

### Cannot Generate Blocks

**Problem**: debug_generate_blocks fails

**Causes**:
1. Wrong debug key
2. Stale production not enabled
3. Required participation too high

**Solution**:
```ini
enable-stale-production = true
required-participation = 0
```

### State Inconsistency

**Problem**: Blockchain state becomes inconsistent

**Solution**: Reset and start fresh:
```bash
rm -rf /tmp/testnet
steemd --data-dir=/tmp/testnet --plugin="debug_node debug_node_api"
```

## Related Documentation

- [debug_node Plugin](debug_node.md) - Debug state plugin
- [Testing Guide](../development/testing.md) - Testing blockchain code
- [Python Debug Node](../development/python-debug-node.md) - Python testing framework

## Source Code

- **API Definition**: [libraries/plugins/apis/debug_node_api/include/steem/plugins/debug_node_api/debug_node_api.hpp](../../libraries/plugins/apis/debug_node_api/include/steem/plugins/debug_node_api/debug_node_api.hpp)
- **Implementation**: [libraries/plugins/apis/debug_node_api/debug_node_api_plugin.cpp](../../libraries/plugins/apis/debug_node_api/debug_node_api_plugin.cpp)

## License

See [LICENSE.md](../../LICENSE.md)
