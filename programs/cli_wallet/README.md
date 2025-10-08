# cli_wallet

Command-line wallet for interacting with Steem blockchain.

## Overview

`cli_wallet` provides an interactive command-line interface for:
- Managing accounts and keys
- Creating and signing transactions
- Transferring STEEM/SBD
- Posting content
- Voting and social interactions
- Witness operations
- Market trading

## Building

```bash
cd build
make -j$(nproc) cli_wallet
```

The binary will be at `programs/cli_wallet/cli_wallet`.

## Running

### Connect to Local Node

```bash
./cli_wallet -s ws://localhost:8090
```

### Connect to Remote Node

```bash
# Use TLS for remote connections
./cli_wallet -s wss://api.steemit.com
```

### With Custom Wallet File

```bash
./cli_wallet -s ws://localhost:8090 -w my_wallet.json
```

## First-Time Setup

### Create New Wallet

```
new >>> set_password YOUR_SECURE_PASSWORD
set_password YOUR_SECURE_PASSWORD
null
locked >>>
```

### Unlock Wallet

```
locked >>> unlock YOUR_SECURE_PASSWORD
unlock YOUR_SECURE_PASSWORD
null
unlocked >>>
```

### Import Account Keys

```
unlocked >>> import_key WIF_PRIVATE_KEY
import_key WIF_PRIVATE_KEY
true
unlocked >>>
```

## Common Commands

### Account Operations

```
# Get account information
get_account alice

# List owned accounts
list_my_accounts

# Create new account (requires existing account)
create_account creator new_account "{}" true
```

### Transfers

```
# Transfer STEEM
transfer from_account to_account "10.000 STEEM" "memo" true

# Transfer SBD
transfer from_account to_account "5.000 SBD" "payment" true

# Transfer to savings
transfer_to_savings from_account to_account "100.000 STEEM" "savings" true
```

### Content Operations

```
# Post a new article
post_comment author "permlink" "" "category" "title" "body" "{}" true

# Vote on content
vote voter author permlink 100 true

# Edit comment
post_comment author "permlink" parent_author parent_permlink "title" "new_body" "{}" true
```

### Power Operations

```
# Power up (STEEM to Steem Power)
transfer_to_vesting from_account to_account "100.000 STEEM" true

# Power down (start withdrawing)
withdraw_vesting account "50.000000 VESTS" true

# Delegate Steem Power
delegate_vesting_shares delegator delegatee "1000.000000 VESTS" true
```

### Market Operations

```
# Get order book
get_order_book 10

# Place limit order
sell steem "10.000 STEEM" "30.000 SBD" false true
buy steem "30.000 SBD" "10.000 STEEM" false true

# Cancel order
cancel_order owner order_id true
```

### Witness Operations

```
# Update witness
update_witness witness_name "https://witness.url" \
  PUBLIC_SIGNING_KEY \
  {"account_creation_fee":"0.100 STEEM","maximum_block_size":65536,"sbd_interest_rate":0} \
  true

# Publish price feed (witnesses only)
publish_feed witness_name {"base":"1.000 SBD","quote":"1.000 STEEM"} true

# Vote for witness
vote_for_witness voter witness true true
```

### Key Management

```
# Get private key from public key
get_private_key PUBLIC_KEY

# Suggest brain key
suggest_brain_key

# Import key
import_key WIF_PRIVATE_KEY

# List wallet keys
list_keys
```

## Command Categories

### Wallet Management
- `set_password` - Set wallet password
- `unlock` - Unlock wallet
- `lock` - Lock wallet
- `import_key` - Import private key
- `get_private_key` - Get private key

### Account Info
- `get_account` - Get account details
- `get_account_history` - Get account history
- `list_my_accounts` - List imported accounts

### Transfers
- `transfer` - Send STEEM/SBD
- `transfer_to_vesting` - Power up
- `transfer_to_savings` - Transfer to savings
- `transfer_from_savings` - Transfer from savings

### Content
- `post_comment` - Post/edit content
- `vote` - Vote on content
- `delete_comment` - Delete content

### Social
- `follow` - Follow account
- `get_followers` - Get followers
- `get_following` - Get following

### Market
- `get_order_book` - View order book
- `sell` / `buy` - Place orders
- `cancel_order` - Cancel order

### Witness
- `update_witness` - Update witness
- `vote_for_witness` - Vote for witness
- `publish_feed` - Publish price feed

## Interactive Features

### Auto-Completion

Press TAB to auto-complete commands:

```
unlocked >>> trans[TAB]
transfer  transfer_from_savings  transfer_to_savings  transfer_to_vesting
```

### Command History

Use UP/DOWN arrows to navigate command history.

### Help

```
# General help
help

# Command-specific help
gethelp transfer
```

## Configuration

### Wallet File Location

Default: `wallet.json` in current directory

Change with `-w` flag:
```bash
./cli_wallet -s ws://localhost:8090 -w /path/to/wallet.json
```

### Node Connection

The node must have required APIs enabled:

```ini
# In steemd config.ini
plugin = webserver json_rpc
plugin = database_api network_broadcast_api condenser_api
```

## Security Best Practices

### Password Security

1. **Strong Password**: Use long, random password
2. **Never Share**: Don't share wallet password
3. **Backup**: Keep encrypted backup
4. **Secure Storage**: Store wallet.json securely

### Key Security

1. **Separate Keys**: Use posting key for social, active for transfers
2. **Key Hierarchy**: owner > active > posting
3. **Backup Keys**: Store keys securely offline
4. **Revoke Compromised**: Change keys if compromised

### Connection Security

1. **Use TLS**: Always use `wss://` for remote connections
2. **Verify Certificates**: Check certificate with `-a` flag
3. **Trusted Nodes**: Only connect to trusted nodes
4. **Local Node**: Run own node for maximum security

## Scripting

### Batch Operations

Create a script file:

```bash
# commands.txt
unlock MY_PASSWORD
transfer alice bob "1.000 STEEM" "payment 1" true
transfer alice charlie "2.000 STEEM" "payment 2" true
lock
```

Execute:

```bash
./cli_wallet -s ws://localhost:8090 < commands.txt
```

### Non-Interactive Mode

```bash
echo "unlock PASSWORD
get_account alice
lock" | ./cli_wallet -s ws://localhost:8090
```

## Troubleshooting

### Cannot Connect

```
Error: unable to connect to remote host
```

**Solutions**:
1. Check node is running
2. Verify endpoint address
3. Check firewall rules
4. For remote: ensure TLS certificate is valid

### Wallet Locked

```
Error: wallet must be unlocked
```

**Solution**: Run `unlock PASSWORD` first

### Key Not Found

```
Error: missing required key
```

**Solution**: Import required key with `import_key`

### Invalid Transaction

```
Error: transaction validation failed
```

**Common causes**:
1. Insufficient balance
2. Wrong key authority (use active for transfers)
3. Malformed operation
4. Account doesn't exist

## Advanced Usage

### Custom Authorities

Update account authorities:

```
update_account_auth_key alice owner/active/posting NEW_PUBLIC_KEY 1 true
```

### Multi-Signature

Create multi-sig account authority:

```
# Requires manual transaction construction
# See documentation for advanced usage
```

### Escrow Operations

```
escrow_transfer from to agent "10.000 STEEM" "1.000 SBD" fee \
  ratification_deadline escrow_expiration "{}" true
```

## Example Workflows

### Daily Curation

```
unlock PASSWORD
get_account my_account
vote my_account author1 permlink1 100 true
vote my_account author2 permlink2 100 true
lock
```

### Witness Maintenance

```
unlock PASSWORD
publish_feed my_witness {"base":"1.000 SBD","quote":"0.950 STEEM"} true
get_witness my_witness
lock
```

### Trading

```
unlock PASSWORD
get_order_book 20
sell steem "100.000 STEEM" "300.000 SBD" false true
lock
```

## Additional Resources

- [Wallet Library Documentation](../../libraries/wallet/)
- [Steem Operations](../../libraries/protocol/)
- [API Documentation](../../libraries/plugins/apis/)
