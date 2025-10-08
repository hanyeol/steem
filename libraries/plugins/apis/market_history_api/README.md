# Market History API Plugin

STEEM:SBD market data queries.

## Overview

Exposes internal market trading data.

## Configuration

```ini
plugin = market_history_api

# Requires state plugin
plugin = market_history
```

## Methods

- `get_ticker` - 24h market statistics
- `get_volume` - Trading volume
- `get_order_book` - Current order book
- `get_trade_history` - Recent trades
- `get_recent_trades` - Trade history
- `get_market_history` - OHLCV candles
- `get_market_history_buckets` - Available bucket sizes

## Usage

```bash
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"market_history_api.get_ticker",
  "params":{},
  "id":1
}' http://localhost:8090
```

## Dependencies

- `market_history`
- `json_rpc`
- `webserver`
