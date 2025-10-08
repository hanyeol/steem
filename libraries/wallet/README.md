# Wallet Library

Command-line wallet implementation for interacting with the Steem blockchain.

## Overview

The `wallet` library provides:
- Transaction creation and signing
- Account management
- Key storage (encrypted)
- Remote API communication
- Interactive wallet commands

Used by `cli_wallet` program.

## Key Components

### Wallet (`wallet.cpp`)
Main wallet implementation:
- Transaction building
- Key management
- Account operations
- Content operations
- Market operations
- Multi-signature support

### Remote Node API (`remote_node_api.cpp`)
Communication layer for connecting to `steemd` nodes via WebSocket or HTTP.

### API Documentation (`api_documentation_standin.cpp`)
Placeholder for API documentation generation.

## Features

### Account Operations
- Create accounts
- Update account authorities
- Recover accounts
- Delegate Steem Power

### Transfer Operations
- Send STEEM/SBD
- Escrow transfers
- Savings transfers
- Conversions (SBD ↔ STEEM)

### Content Operations
- Post/edit content
- Vote on content
- Set beneficiaries
- Delete comments

### Market Operations
- Place limit orders
- Cancel orders
- View order book
- Market history

### Witness Operations
- Update witness properties
- Publish price feeds
- Vote for witnesses

## Usage

### Starting the Wallet

```bash
./cli_wallet -s ws://localhost:8090
```

### Basic Commands

```
# Unlock wallet
unlock <password>

# Get account info
get_account <account_name>

# Transfer funds
transfer <from> <to> <amount> <memo>

# Post content
post_comment <author> <permlink> <parent_author> <parent_permlink> <title> <body> <json>

# Vote
vote <voter> <author> <permlink> <weight>
```

## Key Management

### Creating a Wallet

```
set_password <password>
```

Creates encrypted `wallet.json` file.

### Importing Keys

```
import_key <wif_private_key>
```

### Exporting Keys

```
get_private_key <public_key>
```

**Security:** Keys are stored encrypted. Never share your password or private keys.

## Configuration

Connect to different nodes:

```bash
# Local node
./cli_wallet -s ws://localhost:8090

# Remote node (use TLS for security)
./cli_wallet -s wss://api.steemit.com

# Specify certificate
./cli_wallet -s wss://api.steemit.com -a cert.pem
```

## API Requirements

The wallet requires the connected node to have:
- `database_api` - Blockchain data
- `network_broadcast_api` - Transaction broadcasting
- `condenser_api` - Compatible API calls

Enable in node's `config.ini`:
```ini
plugin = database_api network_broadcast_api condenser_api
```

## Build

```bash
cd build
make steem_wallet
make cli_wallet
```

## Dependencies

- `steem_protocol` - Transaction types
- `steem_chain` - Chain utilities
- `fc` - Networking and crypto
- `appbase` - Configuration

## Security Considerations

1. **Encryption**: Wallet file is encrypted with password
2. **Connections**: Use TLS (wss://) for remote connections
3. **Key Storage**: Keep wallet.json secure
4. **Backups**: Backup wallet.json and password
5. **API Authentication**: Use authenticated connections when possible

## Development Notes

- All operations create signed transactions
- Transactions include expiration time (default: 30 seconds)
- Multi-signature support for complex authorities
- Auto-completion available for commands
- Help available: `help` or `gethelp <command>`
