# Market History Plugin

STEEM:SBD market data tracking and aggregation.

## Overview

The market_history plugin:
- Tracks internal market trades
- Maintains order book history
- Calculates OHLCV data
- Aggregates market statistics
- Supports multiple time buckets

## Configuration

```ini
plugin = market_history

# Time buckets for OHLCV data (in seconds)
market-history-bucket-size = [15,60,300,3600,86400]

# Number of buckets to maintain per size
market-history-buckets-per-size = 5760
```

## Features

- **Trade History**: Complete market trade log
- **Order Book**: Historical limit orders
- **OHLCV Candles**: Multiple timeframes
- **Volume Tracking**: 24h volume statistics
- **Price History**: Historical price data

## Bucket Sizes

Default buckets:
- 15 seconds
- 60 seconds (1 minute)
- 300 seconds (5 minutes)
- 3600 seconds (1 hour)
- 86400 seconds (1 day)

## Memory Usage

Memory usage = bucket_sizes × buckets_per_size × data_per_bucket

Adjust `market-history-buckets-per-size` to control memory usage.

## Dependencies

- `chain` - Market operations

## Used By

- `market_history_api` - Market data queries
