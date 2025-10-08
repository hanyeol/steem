# Network Broadcast API Plugin

Transaction broadcasting to the network.

## Overview

Handles transaction submission to the blockchain.

## Configuration

```ini
plugin = network_broadcast_api
```

## Methods

- `broadcast_transaction` - Submit signed transaction
- `broadcast_transaction_synchronous` - Submit and wait for confirmation
- `broadcast_block` - Broadcast block (witness only)

## Usage

```bash
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"network_broadcast_api.broadcast_transaction",
  "params":{"trx": {...}},
  "id":1
}' http://localhost:8090
```

## Security

**Important**:
- Transactions must be properly signed
- Verify transaction before broadcasting
- Check expiration times
- Validate operation formats

## Dependencies

- `chain` - Transaction processing
- `p2p` - Network propagation
- `json_rpc`
- `webserver`
