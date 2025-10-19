# market_history Plugin

Internal market OHLCV data and order book tracking for the Steem/SBD trading pair.

## Overview

The `market_history` plugin tracks the internal market (STEEM/SBD decentralized exchange) on the Steem blockchain. It processes fill order operations, generates OHLCV (Open-High-Low-Close-Volume) candlestick data across multiple time buckets, and maintains order history for market analysis and trading applications.

**Plugin Type**: State Tracking
**Dependencies**: [chain](chain.md)
**Memory**: Low to Medium (10-100MB depending on buckets and history depth)
**Default**: Disabled (enable for exchange/trading features)

## Purpose

- **OHLCV Buckets**: Generate candlestick data for various time intervals
- **Order History**: Track all filled orders on the internal market
- **Price Discovery**: Historical price data for charts and analysis
- **Volume Tracking**: Monitor trading volume across different timeframes
- **Market Analytics**: Support technical analysis and trading bots

## Configuration

### Config File Options

```ini
# Enable market_history plugin
plugin = market_history

# Bucket sizes in seconds (JSON array format)
# Default: [15, 60, 300, 3600, 86400] (15s, 1m, 5m, 1h, 1d)
market-history-bucket-size = [15,60,300,3600,86400]

# Number of buckets to retain per bucket size
# Default: 5760 buckets (4 days for 1-min buckets)
market-history-buckets-per-size = 5760
```

### Command Line Options

```bash
steemd \
  --plugin=market_history \
  --market-history-bucket-size='[60,300,900,3600,86400]' \
  --market-history-buckets-per-size=10080
```

### Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `market-history-bucket-size` | [15,60,300,3600,86400] | Time bucket sizes in seconds (JSON array) |
| `market-history-buckets-per-size` | 5760 | Maximum number of buckets to keep per size |

### Bucket Sizes Explained

Common bucket configurations:

| Seconds | Duration | Common Use |
|---------|----------|------------|
| 15 | 15 seconds | Real-time tick data |
| 60 | 1 minute | Short-term trading |
| 300 | 5 minutes | Intraday trading |
| 900 | 15 minutes | Intraday analysis |
| 3600 | 1 hour | Daily trading |
| 14400 | 4 hours | Swing trading |
| 86400 | 1 day | Long-term analysis |

**Calculation Example**:
```
Bucket size: 60 seconds (1 minute)
Buckets per size: 5760
History depth: 5760 × 60 = 345,600 seconds = 4 days

Bucket size: 86400 seconds (1 day)
Buckets per size: 5760
History depth: 5760 × 86400 = 497,664,000 seconds = 16 years
```

## Internal Market Mechanics

### Trading Pairs

The plugin tracks the STEEM/SBD market:

**Base Asset**: STEEM
**Quote Asset**: SBD (Steem Backed Dollars)

### Fill Order Operation

Market trades generate `fill_order_operation`:

```cpp
struct fill_order_operation {
    account_name_type current_owner;  // Whose order was filled
    uint32_t current_orderid;         // Order ID
    asset current_pays;               // What they paid
    account_name_type open_owner;     // Counterparty
    uint32_t open_orderid;            // Counterparty order ID
    asset open_pays;                  // What counterparty paid
};
```

**Example Fill**:
```json
{
  "current_owner": "alice",
  "current_orderid": 12345,
  "current_pays": "100.000 SBD",
  "open_owner": "bob",
  "open_orderid": 67890,
  "open_pays": "300.000 STEEM"
}
```

This represents:
- Alice sold 100 SBD for 300 STEEM
- Bob sold 300 STEEM for 100 SBD
- Price: 3.0 STEEM/SBD

## Database Objects

### Bucket Object

OHLCV data for a specific time bucket:

```cpp
struct bucket_object {
    fc::time_point_sec open;     // Bucket start time
    uint32_t seconds;            // Bucket size in seconds

    // STEEM side
    share_type steem_high;       // Highest STEEM amount
    share_type steem_low;        // Lowest STEEM amount
    share_type steem_open;       // Opening STEEM amount
    share_type steem_close;      // Closing STEEM amount
    share_type steem_volume;     // Total STEEM volume

    // SBD side
    share_type sbd_high;         // Highest SBD amount
    share_type sbd_low;          // Lowest SBD amount
    share_type sbd_open;         // Opening SBD amount
    share_type sbd_close;        // Closing SBD amount
    share_type sbd_volume;       // Total SBD volume
};
```

**Price Calculation**:
```cpp
price high = sbd_high / steem_high;
price low = sbd_low / steem_low;
price open = sbd_open / steem_open;
price close = sbd_close / steem_close;
```

### Order History Object

Record of each filled order:

```cpp
struct order_history_object {
    fc::time_point_sec time;           // When order filled
    fill_order_operation op;           // Complete fill operation
};
```

## Data Processing

### Bucket Generation Flow

```
fill_order_operation occurs
        ↓
Calculate bucket start time
    open_time = (current_time / bucket_size) × bucket_size
        ↓
For each configured bucket size:
    ↓
    Find or create bucket
    ↓
    Update OHLCV data:
        - Volume += trade volume
        - Close = latest price
        - High = max(high, current_price)
        - Low = min(low, current_price)
        ↓
    Remove old buckets (beyond buckets-per-size limit)
        ↓
Store order in order_history
```

### OHLCV Update Example

**Initial bucket** (first trade at 10:00:00):
```
open_time: 10:00:00
steem_open: 300.000 STEEM
sbd_open: 100.000 SBD
steem_high: 300.000 STEEM
sbd_high: 100.000 SBD
steem_low: 300.000 STEEM
sbd_low: 100.000 SBD
steem_close: 300.000 STEEM
sbd_close: 100.000 SBD
steem_volume: 300.000 STEEM
sbd_volume: 100.000 SBD
```

**After second trade** (10:00:30, higher price):
```
steem_close: 310.000 STEEM (updated)
sbd_close: 100.000 SBD (updated)
steem_high: 310.000 STEEM (updated, higher than 300)
sbd_high: 100.000 SBD (same)
steem_low: 300.000 STEEM (unchanged)
sbd_low: 100.000 SBD (unchanged)
steem_volume: 610.000 STEEM (300 + 310)
sbd_volume: 200.000 SBD (100 + 100)
```

### Bucket Pruning

Old buckets automatically removed:

```cpp
cutoff_time = current_time - (bucket_size × buckets_per_size)

// Remove all buckets older than cutoff_time
for (bucket in buckets_for_size) {
    if (bucket.open < cutoff_time) {
        remove(bucket);
    }
}
```

**Example** (1-minute buckets, 1440 max):
```
Current time: 2023-01-15 10:00:00
Bucket size: 60 seconds
Max buckets: 1440
Cutoff: 10:00:00 - (60 × 1440) = 2023-01-14 10:00:00

All buckets before 2023-01-14 10:00:00 deleted
Keeps exactly 24 hours of 1-minute data
```

## Memory Usage

### Estimation Formula

```
memory_per_bucket = 128 bytes
total_buckets = sum(buckets_per_size for each bucket_size)
total_memory = total_buckets × memory_per_bucket

order_history_memory = num_orders × 256 bytes
```

### Example Configurations

**Light** (minimal history):
```ini
market-history-bucket-size = [300,3600,86400]
market-history-buckets-per-size = 1000

Total buckets: 3 × 1000 = 3,000
Memory: 3,000 × 128 = 384 KB
```

**Standard** (default):
```ini
market-history-bucket-size = [15,60,300,3600,86400]
market-history-buckets-per-size = 5760

Total buckets: 5 × 5760 = 28,800
Memory: 28,800 × 128 = 3.6 MB
```

**Heavy** (extensive history):
```ini
market-history-bucket-size = [15,60,300,900,3600,14400,86400]
market-history-buckets-per-size = 20000

Total buckets: 7 × 20,000 = 140,000
Memory: 140,000 × 128 = 17.9 MB
```

**Note**: Order history adds additional memory proportional to trading volume.

## Performance Tuning

### Optimizing for Different Use Cases

**Trading Bot** (short-term data):
```ini
# Focus on short timeframes
market-history-bucket-size = [15,60,300]
market-history-buckets-per-size = 2880  # 12 hours of 15s data
```

**Analytics Platform** (comprehensive data):
```ini
# All timeframes, deep history
market-history-bucket-size = [60,300,900,3600,14400,86400]
market-history-buckets-per-size = 10080  # ~1 week at shortest interval
```

**Archive Node** (maximum history):
```ini
# Keep all data
market-history-bucket-size = [15,60,300,900,3600,14400,86400]
market-history-buckets-per-size = 100000  # ~2 years at 15s interval
```

**Minimal Node** (disabled):
```ini
# Don't enable market_history plugin
# Use external service for market data
```

### CPU Impact

Market history processing is lightweight:
- Only triggered by fill_order operations
- ~10-50μs per fill
- Negligible impact on block processing

### Disk I/O

Stored in chainbase shared memory:
- Memory-mapped file
- Automatic persistence
- No manual disk operations

## Monitoring

### Bucket Coverage

Check available history:

```bash
# Query via market_history_api
curl -s http://localhost:8090 \
  -d '{
    "jsonrpc":"2.0",
    "id":1,
    "method":"market_history_api.get_market_history",
    "params":{"bucket_seconds":60,"start":"2023-01-01T00:00:00","end":"2023-01-02T00:00:00"}
  }'
```

### Trading Volume

Monitor market activity:

```bash
# Get recent trades
curl -s http://localhost:8090 \
  -d '{
    "jsonrpc":"2.0",
    "id":1,
    "method":"market_history_api.get_recent_trades",
    "params":{"limit":100}
  }'
```

### Plugin Status

Check plugin logs:

```bash
tail -f witness_node_data_dir/logs/stderr.txt | grep -i market

# Sample output:
# market_history: plugin_initialize() begin
# bucket-size [15,60,300,3600,86400]
# history-per-size 5760
# market_history: plugin_initialize() end
```

## Troubleshooting

### Missing Market Data

**Problem**: Market history queries return empty results

**Symptoms**:
```json
{
  "buckets": []
}
```

**Solutions**:
1. Verify plugin enabled:
   ```ini
   plugin = market_history
   ```

2. Check bucket configuration:
   ```ini
   # Ensure requested bucket_seconds is in configuration
   market-history-bucket-size = [60,300,3600,86400]
   ```

3. Verify time range within retention:
   ```
   Query time must be within:
   current_time - (bucket_size × buckets_per_size)
   ```

4. Restart with plugin:
   ```bash
   # Stop node
   # Add plugin to config
   # Replay blockchain to populate historical data
   steemd --replay-blockchain
   ```

### Incomplete Buckets

**Problem**: Recent buckets show zero volume

**Symptoms**:
```json
{
  "steem_volume": "0.000 STEEM",
  "sbd_volume": "0.000 SBD"
}
```

**Causes**:
- No trading activity in that period
- Very sparse trading (normal)

**Verification**:
```bash
# Check if any fills occurred
curl -s http://localhost:8090 \
  -d '{
    "jsonrpc":"2.0",
    "id":1,
    "method":"market_history_api.get_trade_history",
    "params":{"start":"2023-01-01T00:00:00","end":"2023-01-02T00:00:00","limit":100}
  }'
```

### High Memory Usage

**Problem**: market_history using excessive memory

**Symptoms**:
```
shared_memory.bin growing large
Memory usage increasing over time
```

**Solutions**:
1. Reduce buckets retained:
   ```ini
   market-history-buckets-per-size = 1000  # Reduce from 5760
   ```

2. Remove unnecessary bucket sizes:
   ```ini
   # Keep only needed intervals
   market-history-bucket-size = [300,3600,86400]
   ```

3. Replay to apply changes:
   ```bash
   steemd --replay-blockchain
   ```

### Data Not Updating

**Problem**: Market history not updating with new trades

**Symptoms**:
```
Latest bucket timestamp not advancing
Order history not growing
```

**Solutions**:
1. Check if trades actually occurring:
   ```bash
   # Query order book - if empty, no trades happening
   curl -s http://localhost:8090 \
     -d '{"jsonrpc":"2.0","id":1,"method":"condenser_api.get_order_book","params":[10]}'
   ```

2. Verify plugin running:
   ```bash
   # Check logs for market_history initialization
   grep market_history witness_node_data_dir/logs/stderr.txt
   ```

3. Ensure not replaying:
   ```
   During replay, plugin only processes historical data
   Wait for sync to complete
   ```

## Use Cases

### Trading Platform

Real-time market data for exchange:

```ini
plugin = chain p2p webserver market_history market_history_api

# High-resolution recent data
market-history-bucket-size = [15,60,300,3600,86400]
market-history-buckets-per-size = 5760

# API access
plugin = market_history_api
```

### Analytics Dashboard

Historical price analysis:

```ini
plugin = chain market_history market_history_api

# Focus on longer timeframes
market-history-bucket-size = [300,900,3600,86400]
market-history-buckets-per-size = 10080  # ~1 week at 5-min
```

### Trading Bot

Algorithmic trading:

```ini
plugin = chain market_history market_history_api

# Short-term data only
market-history-bucket-size = [60,300]
market-history-buckets-per-size = 1440  # 24 hours at 1-min
```

### Price Feed Provider

External price service:

```ini
plugin = chain market_history market_history_api

# Daily data for long-term trends
market-history-bucket-size = [3600,86400]
market-history-buckets-per-size = 2000  # ~2 months at 1-hour
```

## API Integration

Requires `market_history_api` plugin for RPC access:

```ini
plugin = market_history market_history_api
```

**Common APIs**:
- `market_history_api.get_market_history` - Get OHLCV buckets
- `market_history_api.get_market_history_buckets` - List available bucket sizes
- `market_history_api.get_recent_trades` - Recent order fills
- `market_history_api.get_trade_history` - Historical trades in time range
- `market_history_api.get_ticker` - Current market ticker (24h stats)
- `market_history_api.get_volume` - 24-hour volume

See API documentation for detailed usage.

## Best Practices

### 1. Choose Appropriate Bucket Sizes

Match your application needs:
- **Real-time trading**: Include 15s, 60s
- **Charting only**: 60s, 300s, 3600s, 86400s sufficient
- **Long-term analysis**: 3600s, 86400s only

### 2. Balance History vs Memory

Consider retention period:
```
Retention = bucket_size × buckets_per_size

1-minute buckets, 1440 max → 24 hours
1-hour buckets, 720 max → 30 days
1-day buckets, 365 max → 1 year
```

### 3. Plan for Replay Time

More buckets = longer replay:
- 5 bucket sizes × 5760 buckets = ~30 seconds extra replay
- 7 bucket sizes × 20000 buckets = ~2 minutes extra replay

### 4. Monitor Trading Volume

Low-volume markets:
- May have sparse data
- Consider longer bucket sizes
- Reduce retention period

## Related Documentation

- [market_history_api Plugin](market_history_api.md) - RPC API for market data
- [Witness Guide](../operations/node-types-guide.md) - Witness node configuration
- [condenser_api](condenser_api.md) - Order book queries
- [chain Plugin](chain.md) - Blockchain database

## Source Code

- **Implementation**: [libraries/plugins/market_history/market_history_plugin.cpp](../../libraries/plugins/market_history/market_history_plugin.cpp)
- **Header**: [libraries/plugins/market_history/include/steem/plugins/market_history/market_history_plugin.hpp](../../libraries/plugins/market_history/include/steem/plugins/market_history/market_history_plugin.hpp)

## License

See [LICENSE.md](../../LICENSE.md)
