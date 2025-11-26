# statsd Plugin

Send real-time blockchain metrics to StatsD for monitoring and visualization in Grafana, Datadog, or similar platforms.

## Overview

The `statsd` plugin integrates Steem blockchain nodes with StatsD monitoring infrastructure. It sends metrics over UDP to StatsD servers for aggregation, visualization, and alerting.

**Plugin Type**: Monitoring/Metrics
**Dependencies**: None (standalone)
**Memory**: Minimal (~10MB)
**Default**: Disabled

## Purpose

- **Real-time Metrics**: Send blockchain metrics to monitoring systems
- **Performance Monitoring**: Track node performance and health
- **Custom Dashboards**: Enable Grafana/Datadog visualizations
- **Alerting**: Trigger alerts based on metric thresholds
- **Network Statistics**: Monitor P2P and block propagation

## Configuration

### Config File Options

```ini
# Enable statsd plugin
plugin = statsd

# StatsD server endpoint (IP:PORT or hostname:PORT)
statsd-endpoint = 127.0.0.1:8125

# Batch size (number of metrics per UDP packet)
statsd-batchsize = 10

# Whitelist specific metrics (optional)
statsd-whitelist = chain.blocks database.objects
statsd-whitelist = p2p.connections witness.production

# OR blacklist specific metrics (optional)
statsd-blacklist = debug.verbose unnecessary.stats
```

### Command Line Options

```bash
steemd \
  --plugin=statsd \
  --statsd-endpoint=monitoring.local:8125 \
  --statsd-batchsize=20
```

### Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `statsd-endpoint` | (required) | StatsD server address (hostname:port or IP:port) |
| `statsd-batchsize` | 1 | Number of metrics batched per UDP packet (1-100) |
| `statsd-whitelist` | (none) | Only send specified metrics (space/tab separated) |
| `statsd-blacklist` | (none) | Exclude specified metrics (space/tab separated) |
| `statsd-record-on-replay` | false | Record metrics during blockchain replay |

## Metric Types

The plugin supports all StatsD metric types:

### Counter (Increment/Decrement)

**Track cumulative events**:
```cpp
// C++ API
increment("namespace", "stat", "key", frequency);
decrement("namespace", "stat", "key", frequency);
count("namespace", "stat", "key", delta, frequency);
```

**StatsD format**: `steemd.namespace.stat.key:1|c`

**Examples**:
- Block production: `steemd.witness.blocks.produced:1|c`
- Transaction received: `steemd.p2p.transactions.received:1|c`
- API calls: `steemd.api.database_api.calls:1|c`

### Gauge (Absolute Values)

**Track current state**:
```cpp
// C++ API
gauge("namespace", "stat", "key", value, frequency);
```

**StatsD format**: `steemd.namespace.stat.key:42|g`

**Examples**:
- Active connections: `steemd.p2p.connections.active:15|g`
- Pending transactions: `steemd.chain.pending.count:42|g`
- Memory usage: `steemd.system.memory.free:8589934592|g`

### Timing (Duration)

**Track operation duration**:
```cpp
// C++ API
timing("namespace", "stat", "key", milliseconds, frequency);
```

**StatsD format**: `steemd.namespace.stat.key:250|ms`

**Examples**:
- Block validation: `steemd.chain.validation.duration:150|ms`
- API response time: `steemd.api.response.time:25|ms`
- Database operations: `steemd.database.query.time:10|ms`

## Metric Filtering

### Whitelist Mode

**Only send specific metrics**:

```ini
# Enable filtering with whitelist
statsd-whitelist = chain witness

# Send all metrics in "chain" and "witness" namespaces
# Format: namespace.stat or just namespace

# Specific stats only
statsd-whitelist = chain.blocks witness.production
```

**Whitelist format**:
- `namespace` - All stats in namespace
- `namespace.stat` - Specific stat only
- Space or tab separated
- Multiple `statsd-whitelist` options combine

### Blacklist Mode

**Exclude specific metrics**:

```ini
# Enable filtering with blacklist
statsd-blacklist = debug test

# Exclude "debug" and "test" namespaces
# All other metrics are sent

# Specific exclusions
statsd-blacklist = api.verbose p2p.debug
```

**Blacklist format**:
- Same format as whitelist
- Excludes specified metrics
- All others are sent
- Cannot use whitelist and blacklist together

### Filtering Examples

**High-traffic API node** (reduce metric volume):
```ini
statsd-endpoint = 127.0.0.1:8125
statsd-blacklist = api.debug database.verbose
```

**Witness node** (focus on production):
```ini
statsd-endpoint = 127.0.0.1:8125
statsd-whitelist = witness chain.blocks p2p.connections
```

**Development** (all metrics):
```ini
statsd-endpoint = 127.0.0.1:8125
# No whitelist/blacklist = all metrics sent
```

## Batching

### Batch Size Configuration

**UDP packet efficiency**:

```ini
# Batch size 1 (default) - immediate send
statsd-batchsize = 1

# Batch size 10 - moderate batching
statsd-batchsize = 10

# Batch size 50 - aggressive batching
statsd-batchsize = 50
```

### Batch Size Trade-offs

| Batch Size | Latency | Network Overhead | Risk |
|------------|---------|------------------|------|
| 1 | Lowest | Highest | Minimal loss |
| 10 | Low | Moderate | Low loss |
| 50 | Moderate | Lowest | Moderate loss |
| 100+ | High | Very low | High loss |

**Recommendations**:
- **Witness nodes**: 1-10 (prioritize latency)
- **API nodes**: 10-20 (balance latency/overhead)
- **Seed nodes**: 20-50 (minimize overhead)

## StatsD Integration

### StatsD Server Setup

**Install StatsD**:
```bash
# Ubuntu/Debian
sudo apt-get install statsd

# Configuration (/etc/statsd/config.js)
{
  port: 8125,
  backends: ["./backends/graphite"],
  graphitePort: 2003,
  graphiteHost: "127.0.0.1"
}

# Start StatsD
sudo systemctl start statsd
```

### Graphite Backend

**Store metrics for visualization**:
```bash
# Install Graphite
sudo apt-get install graphite-carbon graphite-web

# Configure retention (/etc/carbon/storage-schemas.conf)
[steem]
pattern = ^steemd\.
retentions = 10s:1d,1m:7d,10m:30d,1h:1y
```

### Grafana Dashboards

**Visualize metrics**:
```bash
# Install Grafana
sudo apt-get install grafana

# Configure Graphite datasource
# Create dashboards for:
# - Block production rates
# - API response times
# - P2P connection counts
# - Memory/CPU usage
# - Transaction throughput
```

## Use Cases

### Witness Node Monitoring

**Purpose**: Track block production and performance

```ini
plugin = chain p2p witness statsd

# Send to local StatsD
statsd-endpoint = 127.0.0.1:8125
statsd-batchsize = 5

# Focus on production metrics
statsd-whitelist = witness chain.blocks p2p

# Grafana alerts:
# - Block production failures
# - Low witness participation
# - High block validation time
```

### API Node Performance

**Purpose**: Monitor API response times and load

```ini
plugin = chain p2p webserver database_api statsd

statsd-endpoint = monitoring.local:8125
statsd-batchsize = 10

# Track API performance
statsd-whitelist = api database webserver

# Dashboard metrics:
# - Requests per second
# - Response time percentiles
# - Error rates
# - Active connections
```

### Network Health Monitoring

**Purpose**: Track P2P network status

```ini
plugin = chain p2p statsd

statsd-endpoint = 127.0.0.1:8125

# Network metrics
statsd-whitelist = p2p chain.sync

# Monitor:
# - Peer connection count
# - Block sync rate
# - Fork detection
# - Network participation
```

### Infrastructure Monitoring

**Purpose**: Send metrics to Datadog/AWS CloudWatch

```ini
plugin = statsd

# DogStatsD endpoint (Datadog agent)
statsd-endpoint = 127.0.0.1:8125
statsd-batchsize = 20

# Or AWS CloudWatch via StatsD proxy
# statsd-endpoint = cloudwatch-proxy:8125
```

## Monitoring

### Verify Metrics Sending

**Check StatsD receiving metrics**:
```bash
# Monitor StatsD logs
tail -f /var/log/statsd/statsd.log

# Watch UDP packets
sudo tcpdump -i lo -n udp port 8125 -A

# Example output:
# steemd.chain.blocks.applied:1|c
# steemd.p2p.connections.active:15|g
```

### Grafana Query Examples

**Block production rate**:
```
sumSeries(steemd.witness.blocks.produced)
```

**API response time (95th percentile)**:
```
steemd.api.*.response_time.upper_95
```

**Active P2P connections**:
```
steemd.p2p.connections.active
```

### Common Metrics

**Chain metrics**:
- `steemd.chain.blocks.applied` - Blocks processed
- `steemd.chain.blocks.validation_time` - Validation duration
- `steemd.chain.transactions.processed` - Transactions applied

**Witness metrics**:
- `steemd.witness.blocks.produced` - Blocks produced
- `steemd.witness.blocks.missed` - Missed block slots
- `steemd.witness.participation_rate` - Network participation

**P2P metrics**:
- `steemd.p2p.connections.active` - Active peers
- `steemd.p2p.blocks.received` - Blocks from network
- `steemd.p2p.transactions.broadcast` - Transactions sent

**API metrics**:
- `steemd.api.calls.total` - Total API calls
- `steemd.api.response_time` - Response duration
- `steemd.api.errors` - API failures

## Troubleshooting

### No Metrics Received

**Problem**: StatsD server not receiving metrics

**Check**:
```bash
# Verify plugin enabled
grep "plugin.*statsd" config.ini

# Test StatsD endpoint reachable
nc -zv monitoring.local 8125

# Check Steem logs
grep "statsd" logs/stderr.txt
```

**Solutions**:
1. Verify `statsd-endpoint` configuration
2. Check firewall allows UDP port 8125
3. Ensure StatsD server running
4. Test with manual metric: `echo "test:1|c" | nc -u -w0 127.0.0.1 8125`

### Metrics Filtered Out

**Problem**: Expected metrics not appearing

**Check**:
```bash
# Review whitelist/blacklist config
grep "statsd-" config.ini

# Verify metric namespace matches
# Example: "chain.blocks" not "chain"
```

**Solutions**:
1. Remove whitelist/blacklist to see all metrics
2. Check namespace matches filter exactly
3. Use broader namespace in whitelist
4. Verify metric is actually generated

### High Network Overhead

**Problem**: Excessive UDP traffic to StatsD

**Symptoms**:
```bash
# Many small UDP packets
sudo netstat -su | grep "packets sent"
```

**Solutions**:
```ini
# Increase batch size
statsd-batchsize = 20

# Filter metrics
statsd-blacklist = debug verbose test

# Reduce sampling frequency in code
```

### Packet Loss

**Problem**: Metrics missing or inconsistent

**Cause**: UDP is unreliable, packets can be dropped

**Solutions**:
1. Increase `statsd-batchsize` (fewer packets)
2. Run StatsD on same host (reduce network hops)
3. Monitor StatsD packet drops: `netstat -su`
4. Ensure sufficient network bandwidth

## Performance Tuning

### Minimize Overhead

**Low-impact configuration**:
```ini
statsd-endpoint = 127.0.0.1:8125
statsd-batchsize = 20
statsd-blacklist = debug verbose test internal
```

### Optimize for Latency

**Real-time metrics**:
```ini
statsd-endpoint = 127.0.0.1:8125
statsd-batchsize = 1
statsd-whitelist = witness.production chain.blocks
```

### Optimize for Throughput

**High-volume metrics**:
```ini
statsd-endpoint = 127.0.0.1:8125
statsd-batchsize = 50
# No filtering - send all metrics
```

## Security Considerations

### Network Exposure

**StatsD protocol is unencrypted**:
- UDP packets are not authenticated
- Metrics contain operational data
- No built-in access control

**Mitigation**:
```bash
# Use firewall to restrict StatsD access
sudo ufw allow from 10.0.0.0/8 to any port 8125 proto udp

# Or use local-only StatsD
statsd-endpoint = 127.0.0.1:8125
```

### Information Disclosure

**Metrics may reveal**:
- Node operational status
- API usage patterns
- Network topology
- Performance characteristics

**Recommendations**:
1. Restrict access to StatsD/Grafana
2. Use VPN for remote monitoring
3. Sanitize sensitive metric names
4. Filter out debug/internal metrics

## Related Documentation

- [p2p Plugin](p2p.md) - Network connectivity
- [witness Plugin](witness.md) - Block production
- [Monitoring Guide](../operations/monitoring.md) - Node monitoring
- [System Architecture](../technical-reference/system-architecture.md) - Architecture overview

## Source Code

- **Implementation**: [libraries/plugins/statsd/statsd_plugin.cpp](../../libraries/plugins/statsd/statsd_plugin.cpp)
- **Header**: [libraries/plugins/statsd/include/steem/plugins/statsd/statsd_plugin.hpp](../../libraries/plugins/statsd/include/steem/plugins/statsd/statsd_plugin.hpp)
- **StatsD Client**: [libraries/plugins/statsd/StatsdClient.hpp](../../libraries/plugins/statsd/StatsdClient.hpp)

## License

See [LICENSE.md](../../LICENSE.md)
