# witness_api Plugin

Query witness-specific information including bandwidth usage and reserve ratio data.

## Overview

The `witness_api` plugin provides read-only access to witness plugin state data. It enables applications to query account bandwidth consumption and network reserve ratio information, which are critical for understanding network capacity and rate limiting.

**Plugin Type**: API Plugin
**Dependencies**: [witness](witness.md), [json_rpc](json_rpc.md)
**Memory**: Minimal (queries existing state)
**Default**: Disabled

## Purpose

- **Bandwidth Queries**: Check account bandwidth usage and limits
- **Rate Limiting**: Understand transaction throttling
- **Network Capacity**: Monitor reserve ratio and network utilization
- **Resource Planning**: Estimate available bandwidth for operations

## Configuration

### Enable Plugin

```ini
# In config.ini
plugin = witness_api
```

Or via command line:

```bash
steemd --plugin=witness_api
```

### Prerequisites

The `witness_api` requires the `witness` state plugin:

```ini
# Enable witness plugin first
plugin = witness
plugin = witness_api
```

**Note**: The `witness` plugin is typically enabled by default on most nodes.

### No Configuration Options

This API plugin has no configurable parameters. All settings are managed by the underlying `witness` plugin.

## Bandwidth System

### Overview

Steem uses a bandwidth rate-limiting system to prevent spam and ensure fair resource allocation:

- **Based on STEEM Power**: More SP = more bandwidth
- **Regenerates over time**: Bandwidth recovers continuously
- **Three types**: Post, forum, and market bandwidth
- **Prevents abuse**: Limits transaction spam

### Bandwidth Types

```cpp
enum bandwidth_type {
   post,    // Rate limiting for posting rewards
   forum,   // Rate limiting for forum operations
   market   // Rate limiting for market operations
}
```

| Type | Operations Affected | Purpose |
|------|-------------------|---------|
| `post` | Posts, comments | Reward eligibility rate limiting |
| `forum` | All social operations | General spam prevention |
| `market` | Market operations | Exchange spam prevention |

### How Bandwidth Works

**Allocation**: Based on account's STEEM Power (vesting shares)
- More SP = higher bandwidth allocation
- Minimum for new accounts
- Scales proportionally with stake

**Consumption**: Each operation consumes bandwidth
- Transaction size affects consumption
- Operation type affects weight
- Bandwidth debited on transaction inclusion

**Regeneration**: Bandwidth recovers over time
- Continuous regeneration
- Full recovery in ~24 hours (configurable)
- Average bandwidth decays exponentially

## API Methods

All methods are called via JSON-RPC at the endpoint configured by the `webserver` plugin (default: `http://localhost:8090`).

### get_account_bandwidth

Get bandwidth usage information for a specific account.

**Request Parameters**:
- `account` (string) - Account name to query
- `type` (string) - Bandwidth type: "post", "forum", or "market"

**Returns**: Bandwidth object with usage statistics (or null if no usage recorded)

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"witness_api.get_account_bandwidth",
  "params":{
    "account":"alice",
    "type":"forum"
  }
}'
```

**Response**:
```json
{
  "result": {
    "bandwidth": {
      "id": 12345,
      "account": "alice",
      "type": "forum",
      "average_bandwidth": "123456789",
      "lifetime_bandwidth": "987654321000",
      "last_bandwidth_update": "2025-10-19T12:00:00"
    }
  }
}
```

**Response Fields**:
- `id` - Database object ID
- `account` - Account name
- `type` - Bandwidth type (post/forum/market)
- `average_bandwidth` - Current average bandwidth usage
- `lifetime_bandwidth` - Total bandwidth consumed (all time)
- `last_bandwidth_update` - Last update timestamp

**Null Response**:
```json
{
  "result": {
    "bandwidth": null
  }
}
```

Null indicates the account has never consumed bandwidth of this type.

### get_reserve_ratio

Get the current network reserve ratio information.

**Request Parameters**: None

**Returns**: Reserve ratio object with network capacity data

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"witness_api.get_reserve_ratio",
  "params":{}
}'
```

**Response**:
```json
{
  "result": {
    "id": 0,
    "average_block_size": 2048,
    "current_reserve_ratio": 20000,
    "max_virtual_bandwidth": "5497558138880"
  }
}
```

**Response Fields**:
- `id` - Database object ID (always 0, singleton)
- `average_block_size` - Moving average block size in bytes
- `current_reserve_ratio` - Current reserve ratio (precision: 10000)
- `max_virtual_bandwidth` - Maximum virtual bandwidth available

## Reserve Ratio System

### Purpose

The reserve ratio dynamically adjusts network capacity based on utilization:
- **High utilization**: Ratio decreases (more restrictive)
- **Low utilization**: Ratio increases (more permissive)
- **Target**: Maintain ~50% network capacity usage

### How It Works

**Average Block Size**:
```
average_block_size = (99 * average_block_size + new_block_size) / 100
```

Updated every block with exponential moving average.

**Reserve Ratio Adjustment**:
- **Increase**: When `average_block_size <= 50% max_block_size`
  - Grows by 1 per round (21 blocks)
  - Maximum: `STEEM_MAX_RESERVE_RATIO`

- **Decrease**: When `average_block_size > 50% max_block_size`
  - Falls by 1% per block
  - Minimum: 1

**Virtual Bandwidth**:
```
max_virtual_bandwidth = max_bandwidth * current_reserve_ratio / RESERVE_RATIO_PRECISION
```

Where:
- `max_bandwidth = max_block_size * BANDWIDTH_AVERAGE_WINDOW_SECONDS / BLOCK_INTERVAL`
- `RESERVE_RATIO_PRECISION = 10000`

### Interpreting Reserve Ratio

| Reserve Ratio | Meaning | Network State |
|---------------|---------|---------------|
| 20000 (2.0x) | High reserve | Low utilization, plenty of capacity |
| 10000 (1.0x) | Normal | Balanced utilization |
| 5000 (0.5x) | Low reserve | High utilization, near capacity |
| 1 (minimal) | Critical | Network at capacity |

## Use Cases

### Transaction Broadcasting Pre-check

Check if account has sufficient bandwidth before broadcasting:

```javascript
async function canBroadcast(account, estimatedSize = 500) {
  try {
    const result = await witness_api.get_account_bandwidth({
      account: account,
      type: "forum"
    });

    if (!result.bandwidth) {
      // New account, likely has bandwidth
      return true;
    }

    const bandwidth = result.bandwidth;
    const timeSinceUpdate = Date.now() - new Date(bandwidth.last_bandwidth_update);
    const regenerated = calculateRegeneration(
      bandwidth.average_bandwidth,
      timeSinceUpdate
    );

    // Check if enough bandwidth available
    const available = getAccountAllocation(account) - regenerated;
    return available >= estimatedSize;

  } catch (error) {
    console.error("Bandwidth check failed:", error);
    return false; // Assume can't broadcast if check fails
  }
}
```

### Bandwidth Monitoring Dashboard

Monitor account bandwidth usage:

```javascript
async function getBandwidthStatus(account) {
  const types = ['post', 'forum', 'market'];
  const status = {};

  for (const type of types) {
    const result = await witness_api.get_account_bandwidth({
      account: account,
      type: type
    });

    if (result.bandwidth) {
      status[type] = {
        used: result.bandwidth.average_bandwidth,
        lastUpdate: result.bandwidth.last_bandwidth_update,
        lifetime: result.bandwidth.lifetime_bandwidth,
        percentage: calculateUsagePercent(account, result.bandwidth)
      };
    } else {
      status[type] = { used: 0, percentage: 0 };
    }
  }

  return status;
}
```

### Network Capacity Monitor

Track network utilization:

```javascript
async function monitorNetworkCapacity() {
  const reserveRatio = await witness_api.get_reserve_ratio();

  const utilizationPercent =
    (reserveRatio.average_block_size / MAX_BLOCK_SIZE) * 100;

  const ratioValue =
    reserveRatio.current_reserve_ratio / 10000;

  return {
    avgBlockSize: reserveRatio.average_block_size,
    utilization: utilizationPercent.toFixed(2) + '%',
    reserveRatio: ratioValue.toFixed(2) + 'x',
    maxVirtualBandwidth: reserveRatio.max_virtual_bandwidth,
    status: utilizationPercent > 50 ? 'High' : 'Normal'
  };
}
```

### Rate Limiting Implementation

Implement client-side rate limiting:

```javascript
class BandwidthLimiter {
  constructor(account) {
    this.account = account;
    this.cache = new Map();
  }

  async checkLimit(type = 'forum', requiredBytes = 500) {
    const result = await witness_api.get_account_bandwidth({
      account: this.account,
      type: type
    });

    if (!result.bandwidth) return true; // New account

    const now = Date.now();
    const lastUpdate = new Date(result.bandwidth.last_bandwidth_update).getTime();
    const elapsed = (now - lastUpdate) / 1000; // seconds

    // Estimate available bandwidth after regeneration
    const used = parseInt(result.bandwidth.average_bandwidth);
    const decayRate = 0.0000116; // Approximate decay per second
    const regenerated = used * decayRate * elapsed;
    const current = Math.max(0, used - regenerated);

    // Get account allocation (needs SP data from database_api)
    const allocation = await this.getAccountAllocation();

    return (allocation - current) >= requiredBytes;
  }

  async getAccountAllocation() {
    // Cache allocation as it changes slowly
    const cached = this.cache.get('allocation');
    if (cached && Date.now() - cached.time < 60000) {
      return cached.value;
    }

    // Get account SP and calculate allocation
    // (requires database_api query - implementation depends on chain parameters)
    const allocation = 1000000; // Placeholder

    this.cache.set('allocation', {value: allocation, time: Date.now()});
    return allocation;
  }
}
```

### Witness Operations

Witnesses can monitor network health:

```javascript
async function witnessNetworkCheck() {
  const reserve = await witness_api.get_reserve_ratio();

  const alerts = [];

  // Check for network congestion
  if (reserve.current_reserve_ratio < 5000) {
    alerts.push({
      level: 'warning',
      message: 'Network approaching capacity',
      ratio: reserve.current_reserve_ratio / 10000
    });
  }

  // Check for unusual block sizes
  if (reserve.average_block_size > MAX_BLOCK_SIZE * 0.75) {
    alerts.push({
      level: 'warning',
      message: 'Large average block size',
      size: reserve.average_block_size
    });
  }

  return {
    timestamp: new Date(),
    reserveRatio: reserve,
    alerts: alerts
  };
}
```

## Performance Notes

### Caching

Bandwidth data changes with every transaction. Cache strategically:

**Reserve Ratio**: Changes slowly (every block)
- Cache for 3-10 seconds
- Safe for network monitoring dashboards

**Account Bandwidth**: Changes with account activity
- Cache for 10-30 seconds for display
- Query fresh for transaction validation

### Batch Queries

For multiple accounts, query sequentially (no batch endpoint):

```javascript
async function getBatchBandwidth(accounts, type = 'forum') {
  const results = await Promise.all(
    accounts.map(account =>
      witness_api.get_account_bandwidth({account, type})
    )
  );

  return results.map((r, i) => ({
    account: accounts[i],
    bandwidth: r.bandwidth
  }));
}
```

## Troubleshooting

### Always Returns Null

**Problem**: `get_account_bandwidth` always returns `null`

**Causes**:
- Account has never used this bandwidth type
- Account is new
- witness plugin not tracking bandwidth

**Solution**:
- Null is normal for accounts that haven't consumed bandwidth
- Use default values (assume bandwidth available)
- Check witness plugin is enabled

### Reserve Ratio Not Updating

**Problem**: `current_reserve_ratio` stays constant

**Causes**:
- Node not syncing
- witness plugin not active
- Reading from old snapshot

**Solution**:
```bash
# Verify node is syncing
curl -s http://localhost:8090 -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_dynamic_global_properties"}' | jq -r '.result.head_block_number'

# Check block numbers are increasing
```

### Bandwidth Calculation Errors

**Problem**: Bandwidth calculations don't match expected values

**Cause**: Complex decay formula, time-dependent

**Solution**: Use approximate calculations, don't rely on exact values. The blockchain is authoritative - client calculations are estimates only.

## Bandwidth Formulas

### Average Bandwidth Decay

Bandwidth decays exponentially over time:

```javascript
function calculateDecay(currentBandwidth, elapsedSeconds) {
  // Simplified approximation
  const decayConstant = 0.0000116; // ~24 hour full recovery
  const decay = currentBandwidth * decayConstant * elapsedSeconds;
  return Math.max(0, currentBandwidth - decay);
}
```

### Account Bandwidth Allocation

Allocation is proportional to STEEM Power:

```javascript
function calculateAllocation(accountVestingShares, totalVestingShares, maxVirtualBandwidth) {
  return (accountVestingShares / totalVestingShares) * maxVirtualBandwidth;
}
```

**Note**: Precise calculations require chain parameters from `database_api`.

## Integration Examples

### React Bandwidth Indicator

```jsx
import { useEffect, useState } from 'react';

function BandwidthIndicator({ account }) {
  const [bandwidth, setBandwidth] = useState(null);

  useEffect(() => {
    async function fetch() {
      const result = await witness_api.get_account_bandwidth({
        account,
        type: 'forum'
      });
      setBandwidth(result.bandwidth);
    }

    fetch();
    const interval = setInterval(fetch, 10000); // Update every 10s
    return () => clearInterval(interval);
  }, [account]);

  if (!bandwidth) {
    return <span className="bandwidth-indicator green">100%</span>;
  }

  const percent = calculateUsagePercent(bandwidth);
  const color = percent > 90 ? 'red' : percent > 70 ? 'yellow' : 'green';

  return (
    <span className={`bandwidth-indicator ${color}`}>
      {(100 - percent).toFixed(0)}% available
    </span>
  );
}
```

### CLI Bandwidth Tool

```bash
#!/bin/bash
# Check bandwidth for account

ACCOUNT=$1

if [ -z "$ACCOUNT" ]; then
  echo "Usage: $0 <account>"
  exit 1
fi

echo "Checking bandwidth for @$ACCOUNT..."
echo

for TYPE in post forum market; do
  RESULT=$(curl -s http://localhost:8090 -d "{
    \"jsonrpc\":\"2.0\",
    \"id\":1,
    \"method\":\"witness_api.get_account_bandwidth\",
    \"params\":{\"account\":\"$ACCOUNT\",\"type\":\"$TYPE\"}
  }")

  echo "$TYPE bandwidth:"
  echo "$RESULT" | jq -r '.result.bandwidth // "Not used yet"'
  echo
done
```

## Related Documentation

- [witness Plugin](witness.md) - Block production and state tracking
- [network_broadcast_api](network_broadcast_api.md) - Transaction broadcasting
- [database_api](database_api.md) - Account and chain queries
- [Chain Parameters](../technical-reference/chain-parameters.md) - Bandwidth parameters

## Source Code

- **API Implementation**: [libraries/plugins/apis/witness_api/witness_api.cpp](../../libraries/plugins/apis/witness_api/witness_api.cpp)
- **API Header**: [libraries/plugins/apis/witness_api/include/steem/plugins/witness_api/witness_api.hpp](../../libraries/plugins/apis/witness_api/include/steem/plugins/witness_api/witness_api.hpp)
- **State Plugin**: [libraries/plugins/witness/witness_plugin.cpp](../../libraries/plugins/witness/witness_plugin.cpp)
- **Objects**: [libraries/plugins/witness/include/steem/plugins/witness/witness_objects.hpp](../../libraries/plugins/witness/include/steem/plugins/witness/witness_objects.hpp)

## License

See [LICENSE.md](../../LICENSE.md)
