# StatsD Plugin

Exports metrics to StatsD for monitoring.

## Overview

The statsd plugin:
- Sends metrics to StatsD server
- Tracks node performance
- Monitors blockchain metrics
- Integrates with Grafana/DataDog

## Configuration

```ini
plugin = statsd

# StatsD server address
statsd-endpoint = 127.0.0.1:8125

# Metric prefix
statsd-prefix = steem.node
```

## Features

- **StatsD Protocol**: Standard metrics format
- **Counter/Gauge/Timer**: Multiple metric types
- **Low Overhead**: Minimal performance impact
- **Dashboard Integration**: Works with Grafana, etc.

## Metrics

Tracks:
- Block processing time
- Transaction counts
- API request rates
- Memory usage
- Network statistics

## Use Cases

- Production monitoring
- Performance analysis
- Alerting
- Capacity planning

## Dependencies

- `chain` - Blockchain metrics

## Integration

Works with:
- Grafana
- DataDog
- Hosted Graphite
- Custom StatsD servers
