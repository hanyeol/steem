# p2p Plugin

Peer-to-peer networking for block propagation and transaction relay across the Steem network.

## Overview

The `p2p` plugin manages network connectivity between Steem nodes. It discovers peers, propagates blocks and transactions, and maintains consensus across the distributed network.

**Plugin Type**: Core (Always Required)
**Dependencies**: [chain](chain.md)
**Memory**: Minimal (~50MB)
**Default**: Always enabled

## Purpose

- **Peer Discovery**: Find and connect to other Steem nodes
- **Block Propagation**: Relay new blocks across the network
- **Transaction Broadcasting**: Distribute unconfirmed transactions
- **Network Sync**: Download blockchain history from peers
- **Fork Detection**: Identify and resolve blockchain forks

## Configuration

### Config File Options

```ini
# P2P listening endpoint (IP:PORT)
p2p-endpoint = 0.0.0.0:2001

# Publicly advertised P2P address (for NAT)
# p2p-server-address = your-domain.com:2001

# Seed nodes (bootstrap network connection)
p2p-seed-node = seed.steemit.com:2001
p2p-seed-node = seed.steemitstage.com:2001
p2p-seed-node = seed.liondani.com:2001

# Maximum number of peers
# p2p-max-connections = 200

# User agent string
# p2p-user-agent = steem/0.23.0

# Block synchronization parameters
# block-sync-rate-limit = 1000
```

### Command Line Options

```bash
steemd \
  --p2p-endpoint=0.0.0.0:2001 \
  --p2p-seed-node=seed.steemit.com:2001 \
  --p2p-max-connections=100
```

### Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `p2p-endpoint` | 0.0.0.0:2001 | Local endpoint for peer connections |
| `p2p-server-address` | (auto) | Public address advertised to network |
| `p2p-seed-node` | (multiple) | Bootstrap nodes for initial connection |
| `p2p-max-connections` | 200 | Maximum concurrent peer connections |
| `p2p-user-agent` | steem/version | User agent string |
| `block-sync-rate-limit` | 1000 | Max blocks/sec during sync |

## Network Protocol

### Connection Handshake

```
Node A                               Node B
  |                                     |
  |--- TCP Connect (port 2001) -------→ |
  |←-- TCP Accept --------------------  |
  |                                     |
  |--- hello_message -----------------→ |
  |     (chain_id, version, addr)       |
  |←-- hello_message ----------------   |
  |                                     |
  |--- handshake_complete ------------→ |
  |←-- handshake_complete -----------   |
  |                                     |
  |←--→ Synchronized                ←--→|
```

### Message Types

**Handshake**:
- `hello_message` - Initial connection info
- `connection_rejected_message` - Connection refused

**Sync**:
- `sync_request_message` - Request block range
- `sync_response_message` - Send requested blocks
- `notice_message` - Advertise available items

**Blocks**:
- `block_message` - Broadcast new block
- `signed_block_message` - Full block with signatures

**Transactions**:
- `trx_message` - Broadcast transaction

**Maintenance**:
- `ping_message` / `pong_message` - Keepalive
- `address_request_message` - Request peer list
- `address_message` - Send peer list

## Peer Discovery

### Seed Nodes

Seed nodes are hardcoded entry points:

```ini
# Official seed nodes
p2p-seed-node = seed.steemit.com:2001
p2p-seed-node = seed.steemitstage.com:2001
p2p-seed-node = seed.liondani.com:2001
p2p-seed-node = gtg.steem.house:2001
p2p-seed-node = seed.curie.steem.com:2001
```

### Peer Exchange

After connecting to seeds, nodes learn about other peers:

1. Connect to seed node
2. Request peer list (`address_request_message`)
3. Receive peer addresses (`address_message`)
4. Connect to discovered peers
5. Share own peer list with others

### Connection Limits

```ini
# Inbound connections (others connecting to you)
# Unlimited by default

# Outbound connections (you connecting to others)
# Limited by p2p-max-connections

# Total connections
p2p-max-connections = 200
```

## Block Propagation

### New Block Flow

```
Witness produces block
        ↓
Local validation
        ↓
Broadcast to all connected peers
        ↓
Peers validate and relay
        ↓
Network-wide distribution (~1-2 seconds)
```

### Sync Modes

**Fast Sync** (from P2P):
- Download blocks from peers
- Validate signatures only
- Faster than replay
- Used when first starting node

**Replay** (from block log):
- Re-execute all operations
- Full validation
- Slower but more thorough
- Required when changing plugins

### Fork Handling

When receiving conflicting blocks:

1. **Detect fork**: Multiple blocks at same height
2. **Compare chains**: Choose chain with more witness signatures
3. **Switch if needed**: Pop blocks and apply from fork point
4. **Relay decision**: Broadcast chosen fork to peers

## Transaction Broadcasting

### Transaction Flow

```
Client submits transaction
        ↓
network_broadcast_api
        ↓
Local validation
        ↓
Add to pending transaction pool
        ↓
Broadcast to peers (trx_message)
        ↓
Peers relay to their peers
        ↓
Network-wide distribution (~1 second)
        ↓
Witness includes in next block
```

### Transaction Propagation

**Flood Protocol**: Each node broadcasts to all peers

**Deduplication**: Track seen transaction IDs to prevent loops

**Expiration**: Transactions expire after 1 hour

## Network Topology

### Target Topology

```
         Seed Nodes (high connectivity)
              /  |  \
             /   |   \
       Witness Nodes (30-50 connections)
          /    |    \
         /     |     \
   API Nodes   |   Full Nodes (8-30 connections)
       |       |       |
    Users   Exchanges  |
                    Users
```

### Optimal Connections

| Node Type | Recommended Connections | Reasoning |
|-----------|-------------------------|-----------|
| Seed Node | 200+ | Maximum peer exposure |
| Witness Node | 30-50 | Fast block propagation |
| API Node | 20-30 | Balanced sync/API load |
| Full Node | 8-30 | Sufficient for sync |

## Firewall Configuration

### Port Requirements

**Inbound**:
```bash
# Allow P2P connections
ufw allow 2001/tcp

# For witnesses (recommended)
ufw allow from trusted-peer-ip to any port 2001
```

**Outbound**:
```bash
# Allow connections to seed nodes (usually open by default)
# No special configuration needed
```

### NAT Configuration

If behind NAT, configure public address:

```ini
p2p-endpoint = 0.0.0.0:2001
p2p-server-address = your-public-ip:2001
# or
p2p-server-address = your-domain.com:2001
```

## Performance Tuning

### Connection Limits

**Seed Node** (maximize connectivity):
```ini
p2p-max-connections = 500
```

**Witness Node** (balance propagation & stability):
```ini
p2p-max-connections = 50
```

**Regular Node** (minimize resources):
```ini
p2p-max-connections = 20
```

### Bandwidth Usage

**Formula**: `connections × block_size × blocks_per_day / 86400`

**Example**: 30 connections, 40KB avg block, 28,800 blocks/day
```
30 × 40KB × 28,800 / 86400 = ~400 KB/s = ~3 Mbps
```

**Recommendation**: At least 10 Mbps upload for witness nodes

### Sync Speed

**Slow sync** (< 100 blocks/sec):
- Check network bandwidth
- Increase `block-sync-rate-limit`
- Connect to faster peers

**Fast sync** (1000+ blocks/sec):
```ini
block-sync-rate-limit = 5000  # Increase if bottlenecked
```

## Monitoring

### Connection Status

```bash
# Get network info
curl -s http://localhost:8090 \
  -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_dynamic_global_properties"}' \
  | jq -r '.result.head_block_number'

# Check P2P port
netstat -tlnp | grep 2001

# Count connections
netstat -tn | grep :2001 | wc -l
```

### Peer Information

**Via logs**:
```
Syncing blockchain from peer 203.0.113.42
Received block #12345678 from 203.0.113.42
Peer 203.0.113.42 is 5 blocks ahead
```

### Health Checks

**Good indicators**:
- Block number increasing (every 3 seconds)
- 8-30 active peer connections
- No fork warnings
- Sync lag < 10 blocks

**Warning signs**:
- No peer connections
- Stuck sync (block number not increasing)
- Frequent fork switches
- All peers timing out

## Troubleshooting

### No Peer Connections

**Problem**: No peers connecting

**Check**:
```bash
# Port listening?
netstat -tlnp | grep 2001

# Firewall blocking?
ufw status

# Seed nodes reachable?
nc -zv seed.steemit.com 2001
```

**Solutions**:
1. Check `p2p-endpoint` configuration
2. Open firewall port 2001
3. Add more seed nodes
4. Check NAT/router port forwarding

### Stuck Sync

**Problem**: Block number not increasing

**Symptoms**:
```
Head block: 12345678 (last updated 5 minutes ago)
```

**Solutions**:
1. Restart node (often resolves temporary issues)
2. Check internet connection
3. Verify seed node connectivity
4. Delete peers file and reconnect:
   ```bash
   rm witness_node_data_dir/p2p/peers.json
   ```

### Fork Warnings

**Problem**: Frequent fork messages

**Symptoms**:
```
Switching to fork from peer X
Switching to fork from peer Y
```

**Causes**:
- Network partition
- Running outdated version
- System clock drift

**Solutions**:
1. Upgrade to latest version
2. Sync system clock (NTP)
3. Check for network issues
4. Ensure sufficient peer connections

### High Bandwidth Usage

**Problem**: Excessive data transfer

**Solutions**:
```ini
# Reduce connections
p2p-max-connections = 20

# Limit sync rate
block-sync-rate-limit = 1000
```

## Security Considerations

### DDoS Protection

**Limit connections**:
```ini
p2p-max-connections = 50  # Prevent connection exhaustion
```

**Firewall rate limiting**:
```bash
# Limit connection rate
iptables -A INPUT -p tcp --dport 2001 -m state --state NEW -m recent --set
iptables -A INPUT -p tcp --dport 2001 -m state --state NEW -m recent --update --seconds 60 --hitcount 10 -j DROP
```

### Witness Security

**Dedicated peers only**:
```ini
# Don't accept public connections
p2p-endpoint = 0.0.0.0:0  # Disable listening

# Connect to trusted peers only
p2p-seed-node = trusted-peer-1.example.com:2001
p2p-seed-node = trusted-peer-2.example.com:2001
```

### Sybil Attack Prevention

The network uses multiple mechanisms:
- Witness reputation
- Block signature validation
- Checkpoint hardcoding
- Irreversibility after 15 confirmations

## Use Cases

### Seed Node

**Purpose**: Maximize network connectivity

```ini
plugin = chain p2p

p2p-endpoint = 0.0.0.0:2001
p2p-server-address = seed.example.com:2001
p2p-max-connections = 500

# No API needed for seed node
```

### Witness Node

**Purpose**: Fast block propagation

```ini
plugin = chain p2p witness

p2p-endpoint = 0.0.0.0:2001
p2p-max-connections = 50

# Connect to trusted witnesses
p2p-seed-node = witness1.example.com:2001
p2p-seed-node = witness2.example.com:2001
```

### Private Network

**Purpose**: Testnet or private chain

```ini
plugin = chain p2p

p2p-endpoint = 10.0.1.100:2001

# Only connect to private network peers
p2p-seed-node = 10.0.1.101:2001
p2p-seed-node = 10.0.1.102:2001

# Don't connect to public network
# (no public seed nodes)
```

## Related Documentation

- [chain Plugin](chain.md) - Blockchain database
- [Node Types Guide](../operations/node-types-guide.md) - Configuration by use case
- [Genesis Launch](../operations/genesis-launch.md) - Start private network
- [DDoS Protection](../operations/ddos-protection.md) - Network security

## Source Code

- **Implementation**: [src/plugins/p2p/p2p_plugin.cpp](../../src/plugins/p2p/p2p_plugin.cpp)
- **Network Layer**: [src/base/fc/src/network/](../../src/base/fc/src/network/)

## License

See [LICENSE.md](../../LICENSE.md)
