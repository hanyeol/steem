# P2P Plugin

Peer-to-peer networking plugin for connecting to and communicating with other Steem nodes.

## Overview

The p2p plugin handles:
- Peer discovery and connection management
- Block propagation across the network
- Transaction broadcasting
- Network synchronization
- Peer reputation and banning

## Configuration

```ini
plugin = p2p

# Listen endpoint for incoming connections
p2p-endpoint = 0.0.0.0:2001

# Seed nodes for initial peer discovery
p2p-seed-node = seed1.steemit.com:2001
p2p-seed-node = seed2.steemit.com:2001

# Maximum incoming connections
p2p-max-connections = 100

# User agent to advertise
p2p-user-agent = Steem/0.19.12
```

## Features

- **Peer Discovery**: Find and connect to network peers
- **Block Sync**: Download and validate blockchain history
- **Transaction Relay**: Broadcast transactions to the network
- **Fork Detection**: Identify and resolve chain forks
- **Peer Management**: Connection pooling and peer scoring

## Network Protocol

The p2p plugin implements the Steem network protocol:
- Block announcements
- Transaction broadcasts
- Inventory requests
- Sync messages

## Dependencies

- `chain` - Access to blockchain state

## Used By

Required for nodes that participate in the P2P network.
