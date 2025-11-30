# market_history_api Plugin

Query market data, trading history, and order book information for the internal STEEM/SBD exchange.

## Overview

The `market_history_api` plugin provides read-only access to market data from Steem's internal decentralized exchange. It enables applications to retrieve ticker data, order books, trade history, and time-bucketed market statistics.

**Plugin Type**: API Plugin
**Dependencies**: [market_history](market_history.md), [json_rpc](json_rpc.md)
**Memory**: Minimal (queries existing state)
**Default**: Disabled

## Purpose

- **Price Discovery**: Real-time ticker data for STEEM/SBD markets
- **Order Book Access**: Query current buy and sell orders
- **Trade History**: Access historical trade executions
- **Market Charts**: Time-bucketed OHLCV data for charting
- **Volume Tracking**: Monitor trading volumes

## Configuration

### Enable Plugin

```ini
# In config.ini
plugin = market_history_api
```

Or via command line:

```bash
steemd --plugin=market_history_api
```

### Prerequisites

The `market_history_api` requires the `market_history` state plugin:

```ini
# Enable state tracking plugin first
plugin = market_history
plugin = market_history_api

# Configure bucket sizes (optional)
market-history-bucket-size = 15
market-history-bucket-size = 60
market-history-bucket-size = 300
market-history-bucket-size = 3600
market-history-bucket-size = 86400

# Configure history depth per bucket (optional)
market-history-buckets-per-size = 5760
```

### Configuration Parameters

Market history configuration is handled by the `market_history` state plugin:

| Parameter | Default | Description |
|-----------|---------|-------------|
| `market-history-bucket-size` | Various | Time bucket sizes in seconds |
| `market-history-buckets-per-size` | 5760 | Number of buckets to retain per size |

**Common bucket sizes**:
- 15 seconds (tick data)
- 60 seconds (1 minute)
- 300 seconds (5 minutes)
- 3600 seconds (1 hour)
- 86400 seconds (1 day)

## API Methods

All methods are called via JSON-RPC at the endpoint configured by the `webserver` plugin (default: `http://localhost:8090`).

### get_ticker

Get current ticker data for the STEEM/SBD market.

**Request Parameters**: None

**Returns**: Current market ticker information

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"market_history_api.get_ticker",
  "params":{}
}'
```

**Response**:
```json
{
  "result": {
    "latest": 0.875,
    "lowest_ask": 0.880,
    "highest_bid": 0.870,
    "percent_change": 2.35,
    "steem_volume": {
      "amount": "123456789",
      "precision": 3,
      "nai": "@@000000021"
    },
    "sbd_volume": {
      "amount": "108000000",
      "precision": 3,
      "nai": "@@000000013"
    }
  }
}
```

**Fields**:
- `latest` - Price of last executed trade (SBD per STEEM)
- `lowest_ask` - Lowest current sell order price
- `highest_bid` - Highest current buy order price
- `percent_change` - 24-hour price change percentage
- `steem_volume` - 24-hour STEEM trading volume
- `sbd_volume` - 24-hour SBD trading volume

### get_volume

Get 24-hour trading volume.

**Request Parameters**: None

**Returns**: Trading volumes for STEEM and SBD

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"market_history_api.get_volume",
  "params":{}
}'
```

**Response**:
```json
{
  "result": {
    "steem_volume": {
      "amount": "123456789",
      "precision": 3,
      "nai": "@@000000021"
    },
    "sbd_volume": {
      "amount": "108000000",
      "precision": 3,
      "nai": "@@000000013"
    }
  }
}
```

### get_order_book

Get current order book (pending buy and sell orders).

**Request Parameters**:
- `limit` (uint32, optional) - Maximum orders per side (max: 500, default: 500)

**Returns**: Arrays of bid and ask orders

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"market_history_api.get_order_book",
  "params":{
    "limit":10
  }
}'
```

**Response**:
```json
{
  "result": {
    "bids": [
      {
        "order_price": {
          "base": {"amount": "870", "precision": 3, "nai": "@@000000013"},
          "quote": {"amount": "1000", "precision": 3, "nai": "@@000000021"}
        },
        "real_price": 0.870,
        "steem": "50000",
        "sbd": "43500",
        "created": "2025-10-19T12:00:00"
      }
    ],
    "asks": [
      {
        "order_price": {
          "base": {"amount": "880", "precision": 3, "nai": "@@000000013"},
          "quote": {"amount": "1000", "precision": 3, "nai": "@@000000021"}
        },
        "real_price": 0.880,
        "steem": "25000",
        "sbd": "22000",
        "created": "2025-10-19T11:30:00"
      }
    ]
  }
}
```

**Order Fields**:
- `order_price` - Price as base/quote ratio
- `real_price` - Decimal price (SBD per STEEM)
- `steem` - STEEM amount in order
- `sbd` - SBD amount in order
- `created` - Order creation timestamp

### get_trade_history

Get historical trades within a time range.

**Request Parameters**:
- `start` (timestamp) - Start time (RFC 3339 format)
- `end` (timestamp) - End time
- `limit` (uint32, optional) - Maximum trades to return (max: 1000, default: 1000)

**Returns**: Array of executed trades

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"market_history_api.get_trade_history",
  "params":{
    "start":"2025-10-19T00:00:00",
    "end":"2025-10-19T23:59:59",
    "limit":100
  }
}'
```

**Response**:
```json
{
  "result": {
    "trades": [
      {
        "date": "2025-10-19T15:30:00",
        "current_pays": {
          "amount": "10000",
          "precision": 3,
          "nai": "@@000000021"
        },
        "open_pays": {
          "amount": "8750",
          "precision": 3,
          "nai": "@@000000013"
        }
      }
    ]
  }
}
```

**Trade Fields**:
- `date` - Trade execution timestamp
- `current_pays` - Amount paid by taker
- `open_pays` - Amount paid by maker

### get_recent_trades

Get most recent trades.

**Request Parameters**:
- `limit` (uint32, optional) - Maximum trades (max: 1000, default: 1000)

**Returns**: Array of recent trades (same format as `get_trade_history`)

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"market_history_api.get_recent_trades",
  "params":{
    "limit":50
  }
}'
```

### get_market_history

Get time-bucketed OHLCV data for charting.

**Request Parameters**:
- `bucket_seconds` (uint32) - Bucket size in seconds (must match configured bucket)
- `start` (timestamp) - Start time
- `end` (timestamp) - End time

**Returns**: Array of market history buckets

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"market_history_api.get_market_history",
  "params":{
    "bucket_seconds":300,
    "start":"2025-10-19T00:00:00",
    "end":"2025-10-19T23:59:59"
  }
}'
```

**Response**:
```json
{
  "result": {
    "buckets": [
      {
        "id": 12345,
        "open": "2025-10-19T15:00:00",
        "seconds": 300,
        "steem": {
          "high": "10000",
          "low": "8000",
          "open": "9000",
          "close": "9500",
          "volume": "150000"
        },
        "non_steem": {
          "high": "8800",
          "low": "7200",
          "open": "8100",
          "close": "8550",
          "volume": "135000"
        }
      }
    ]
  }
}
```

**Bucket Fields**:
- `open` - Bucket start timestamp
- `seconds` - Bucket duration
- `steem` - OHLCV data for STEEM side
- `non_steem` - OHLCV data for SBD side
- `high`, `low`, `open`, `close` - Price extremes and endpoints
- `volume` - Trading volume in bucket

### get_market_history_buckets

Get list of configured bucket sizes.

**Request Parameters**: None

**Returns**: Available bucket sizes in seconds

**Example**:
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc":"2.0",
  "id":1,
  "method":"market_history_api.get_market_history_buckets",
  "params":{}
}'
```

**Response**:
```json
{
  "result": {
    "bucket_sizes": [15, 60, 300, 3600, 86400]
  }
}
```

## Data Structures

### Price Representation

Prices are represented as both:
- **Ratio**: `base/quote` price object
- **Decimal**: `real_price` floating point

For STEEM/SBD market:
- Base = SBD
- Quote = STEEM
- Price = SBD per STEEM

### Asset Amounts

Assets use precision-aware representation:

```javascript
function formatAsset(asset) {
    const amount = parseInt(asset.amount);
    const precision = asset.precision;
    return (amount / Math.pow(10, precision)).toFixed(precision);
}

// Example: {amount: "123456", precision: 3} = "123.456"
```

### Order Book Depth

Bids (buy orders) sorted by price descending (highest first)
Asks (sell orders) sorted by price ascending (lowest first)

## Use Cases

### Trading Interface

Build a decentralized exchange UI:

```ini
plugin = market_history
plugin = market_history_api
plugin = network_broadcast_api
plugin = database_api
```

**Features enabled**:
- Real-time ticker display
- Order book visualization
- Trade history charts
- Order placement (via `network_broadcast_api`)

### Price Tracking

Monitor STEEM/SBD exchange rates:

```bash
# Get current price
market_history_api.get_ticker -> latest

# Track price over time
market_history_api.get_market_history -> analyze buckets
```

### Trading Bot

Automated market making or arbitrage:

```javascript
// Monitor order book depth
const orderBook = await get_order_book({limit: 50});
const spread = orderBook.asks[0].real_price - orderBook.bids[0].real_price;

// Analyze recent trades
const trades = await get_recent_trades({limit: 100});
const avgPrice = calculateVWAP(trades);

// Place orders using network_broadcast_api
```

### Market Analytics

Generate trading reports and statistics:
- Daily volume tracking
- Price volatility analysis
- Liquidity metrics
- Historical OHLCV charting

## Performance Notes

### Bucket Selection

Choose appropriate bucket sizes for your use case:

| Bucket Size | Use Case | Memory Impact |
|-------------|----------|---------------|
| 15s | Tick data, HFT | High |
| 60s | Minute charts | Medium |
| 300s | 5-min charts | Medium |
| 3600s | Hourly analysis | Low |
| 86400s | Daily summaries | Very Low |

### Query Optimization

**Efficient**: Query specific time ranges
```javascript
get_market_history({
    bucket_seconds: 300,
    start: "2025-10-19T00:00:00",
    end: "2025-10-19T23:59:59"
})
```

**Inefficient**: Large time ranges with small buckets
```javascript
// Avoid: Returns 10+ days of 15-second buckets
get_market_history({
    bucket_seconds: 15,
    start: "2025-10-01T00:00:00",
    end: "2025-10-11T23:59:59"
})
```

### Caching Strategy

- **Ticker**: Cache for 3-10 seconds
- **Order Book**: Cache for 1-3 seconds
- **Trade History**: Cache for 30-60 seconds
- **Historical Buckets**: Cache indefinitely (immutable)

## Troubleshooting

### No Market Data

**Problem**: Empty results from all queries

**Causes**:
- `market_history` plugin not enabled
- Plugin started mid-chain without replay
- No trading activity

**Solution**:
```bash
# Verify plugin is active
grep "market_history" witness_node_data_dir/config.ini

# Check for recent trades
curl -s http://localhost:8090 -d '{"jsonrpc":"2.0","id":1,"method":"market_history_api.get_recent_trades","params":{"limit":1}}'
```

If no data, replay blockchain with `market_history` plugin enabled.

### Invalid Bucket Size

**Problem**: `get_market_history` returns error for bucket size

**Cause**: Requested bucket size not configured

**Solution**:
```bash
# Check available buckets
curl -s http://localhost:8090 -d '{"jsonrpc":"2.0","id":1,"method":"market_history_api.get_market_history_buckets","params":{}}'

# Add bucket to config.ini
market-history-bucket-size = 900  # 15 minutes

# Restart node
```

### Missing Historical Data

**Problem**: Old bucket data not available

**Cause**: `market-history-buckets-per-size` limit reached

**Explanation**: The plugin only retains a configured number of buckets per size. For example, with `buckets-per-size = 5760`:
- 300s buckets: 5760 * 300s = 20 days of history
- 3600s buckets: 5760 * 3600s = 240 days of history

**Solution**: Increase `market-history-buckets-per-size` (requires more memory) or use larger bucket sizes for historical analysis.

### Price Calculation

**Problem**: Confusing price representation

**Solution**: Always use `real_price` for display. The `order_price` object is for blockchain operations.

```javascript
// Display price
const displayPrice = order.real_price.toFixed(6);

// Calculate spread
const spread = asks[0].real_price - bids[0].real_price;
```

## Internal Exchange

### How It Works

Steem has a built-in DEX for STEEM/SBD trading:

1. Users place limit orders via `limit_order_create` operation
2. Orders matched automatically by blockchain
3. Trades executed in each block
4. `market_history` plugin records all market activity

### Market Participants

- **SBD Holders**: Convert SBD to STEEM
- **STEEM Holders**: Acquire SBD for promotions
- **Arbitrageurs**: Profit from price inefficiencies
- **Market Makers**: Provide liquidity

### Conversion Alternative

Note: Steem also has direct SBD conversion mechanism (separate from market):
- `convert_operation` converts SBD to STEEM at median price feed
- 3.5 day conversion delay
- Market may offer better prices for immediate conversion

## Related Documentation

- [market_history Plugin](market_history.md) - State plugin configuration
- [network_broadcast_api](network_broadcast_api.md) - Place orders
- [database_api](database_api.md) - Query account balances
- [witness API](witness_api.md) - Price feed data

## Source Code

- **API Implementation**: [src/plugins/apis/market_history_api/market_history_api.cpp](../../src/plugins/apis/market_history_api/market_history_api.cpp)
- **API Header**: [src/plugins/apis/market_history_api/include/steem/plugins/market_history_api/market_history_api.hpp](../../src/plugins/apis/market_history_api/include/steem/plugins/market_history_api/market_history_api.hpp)
- **State Plugin**: [src/plugins/market_history/market_history_plugin.cpp](../../src/plugins/market_history/market_history_plugin.cpp)

## License

See [LICENSE.md](../../LICENSE.md)
