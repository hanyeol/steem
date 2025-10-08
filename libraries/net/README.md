# Net Library

Networking layer for peer-to-peer communication between Steem nodes.

## Overview

The `net` library implements:
- P2P protocol
- Peer discovery and management
- Block propagation
- Transaction broadcasting
- Network message handling

## Features

### Peer Management
- Connection establishment
- Peer discovery via seed nodes
- Connection pool management
- Peer scoring and banning

### Message Types
- Block announcements
- Transaction broadcasts
- Inventory queries
- Sync requests

### Block Propagation
Efficient distribution of new blocks across the network to maintain consensus.

### Transaction Relay
Broadcast transactions to the network for inclusion in blocks.

## Configuration

Set in `config.ini`:

```ini
# P2P endpoint
p2p-endpoint = 0.0.0.0:2001

# Seed nodes
p2p-seed-node = seed1.example.com:2001
p2p-seed-node = seed2.example.com:2001

# Maximum connections
p2p-max-connections = 100

# User agent
p2p-user-agent = Steem/0.19.12
```

## Usage

Used by the `p2p` plugin to enable network communication.

## Build

```bash
cd build
make steem_net
```

## Dependencies

- `steem_chain` - Blockchain types
- `steem_protocol` - Message formats
- `fc` - Networking primitives
