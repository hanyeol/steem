# account_by_key Plugin

Maintains a reverse lookup index mapping public keys to account names for efficient key-based account discovery.

## Overview

The `account_by_key` plugin creates and maintains an index that maps public keys to the accounts that use them. This enables efficient lookups to find which account(s) are associated with a given public key, supporting operations like account recovery and key management.

**Plugin Type**: State (Tracking)
**Dependencies**: [chain](chain.md)
**Memory**: Low (~1-5MB for typical usage)
**Default**: Enabled in most configurations

## Purpose

- **Key-to-Account Mapping**: Index public keys to account names
- **Account Discovery**: Find accounts associated with a specific public key
- **Key Management**: Support wallet operations that need to resolve keys to accounts
- **Account Recovery**: Enable recovery operations by identifying accounts from keys
- **Multi-signature Support**: Track accounts using the same key in different authority roles

## How It Works

### Key Tracking

The plugin monitors these operations that affect account keys:
- `account_create_operation` - New account creation
- `account_create_with_delegation_operation` - Account creation with delegation
- `account_update_operation` - Account key changes
- `recover_account_operation` - Account recovery
- `pow_operation` - Proof of work (mining)
- `pow2_operation` - Proof of work v2

### Authority Types

Keys are tracked across all three authority types:
- **Owner authority** - Highest level control keys
- **Active authority** - Transaction signing keys
- **Posting authority** - Content posting keys

### Index Structure

The plugin maintains a multi-index table with:
- **Primary index**: Unique ID
- **Composite index**: (public_key, account_name) pairs for efficient lookup

## Configuration

### Config File Options

This plugin has no configuration options - it is automatically enabled when loaded.

```ini
# Enable the plugin
plugin = account_by_key
```

### Command Line Options

```bash
steemd --plugin=account_by_key
```

### Dependencies

The plugin requires:
- `chain` plugin (automatically loaded)

## Database Objects

### key_lookup_object

Stores the key-to-account mapping:

```cpp
struct key_lookup_object {
    id_type           id;           // Unique object ID
    public_key_type   key;          // Public key
    account_name_type account;      // Account name
};
```

### Indexes

- **by_id**: Primary index by object ID
- **by_key**: Composite index by (key, account) for lookups

## Use Cases

### Wallet Account Discovery

**Purpose**: Find accounts controlled by wallet keys

When a wallet imports a private key, it can query the index to discover all accounts controlled by that key:

```cpp
// Pseudo-code for wallet operation
public_key_type key = wallet.get_public_key(private_key);
auto accounts = db.find_accounts_by_key(key);
```

### Account Recovery

**Purpose**: Identify accounts during recovery process

Recovery operations need to verify key ownership:

```cpp
// Verify recovery authority
public_key_type recovery_key = get_recovery_key();
auto accounts = lookup_by_key(recovery_key);
```

### Exchange Integration

**Purpose**: Map deposit addresses to user accounts

Exchanges can use public keys as deposit identifiers and resolve them to accounts:

```cpp
// Find account for deposit
public_key_type deposit_key = parse_deposit_address();
auto account = find_single_account_by_key(deposit_key);
```

### Multi-Signature Wallets

**Purpose**: Track shared key usage

Identify all accounts sharing the same authority keys:

```cpp
// Find all accounts using a shared key
public_key_type shared_key = get_multisig_key();
auto shared_accounts = lookup_all_by_key(shared_key);
```

## Performance Characteristics

### Memory Usage

**Typical Usage**:
- ~1-5MB for standard deployments
- Grows with number of unique (key, account) pairs
- Each entry: ~100 bytes (key + account name + overhead)

**Estimation Formula**:
```
Memory (MB) = (unique_key_account_pairs × 100 bytes) / 1,048,576
```

### Lookup Performance

**Key Lookup**: O(log n) - Binary search on composite index
**Account Creation**: O(log n) - Index insertion
**Key Update**: O(log n) - Delete old + insert new

### Write Performance

Updates occur only during:
- Account creation (~10-50 per block)
- Account updates (~1-5 per block)
- Recovery operations (rare)

## API Integration

### account_by_key_api

This plugin is typically accessed through the `account_by_key_api` plugin (if available):

```json
{
  "jsonrpc": "2.0",
  "method": "account_by_key_api.get_key_references",
  "params": {
    "keys": ["STM6vJmrwaX5TjgTS9dPH8KsArso5m91fVodJvv91j7G765wqcNM9"]
  },
  "id": 1
}
```

Response:
```json
{
  "jsonrpc": "2.0",
  "result": {
    "accounts": [["alice"], ["bob"]]
  },
  "id": 1
}
```

## Monitoring

### Health Checks

**Good indicators**:
- Plugin initializes without errors
- Index builds successfully during replay
- Memory usage remains stable

**Warning signs**:
- Excessive memory growth
- Slow account creation operations
- Index corruption errors

### Log Messages

**Initialization**:
```
Initializing account_by_key plugin
```

**Normal Operation**:
```
# No routine log messages during normal operation
```

## Troubleshooting

### High Memory Usage

**Problem**: Plugin consuming excessive memory

**Symptoms**:
- Memory usage growing unexpectedly
- System swapping

**Solutions**:
1. Check for duplicate entries (indicates index corruption)
2. Verify blockchain state integrity
3. Consider replaying with fresh database
4. Monitor for unusual account creation patterns

### Missing Key Lookups

**Problem**: Known keys not returning expected accounts

**Symptoms**:
- API queries return empty results
- Wallet cannot find accounts

**Causes**:
- Index not fully built (during replay)
- Key format mismatch
- Plugin not enabled during replay

**Solutions**:
1. Verify plugin was enabled during full replay
2. Check key format (STM prefix vs raw public key)
3. Ensure blockchain is fully synced
4. Replay with plugin enabled from genesis

### Replay Requirements

**Important**: This plugin tracks state from blockchain history. To have a complete index:

1. Enable plugin before starting replay:
   ```ini
   plugin = account_by_key
   ```

2. Replay from genesis or block log:
   ```bash
   steemd --replay-blockchain
   ```

3. Do NOT enable plugin mid-sync (incomplete index)

## Integration with Other Plugins

### Required By

- `account_by_key_api` - Provides RPC interface for key lookups
- Wallet implementations - Account discovery
- Exchange integrations - Deposit management

### Works With

- `account_history` - Combined account and key lookups
- `database_api` - Account information queries

## Implementation Details

### Hardfork Considerations

**Hardfork 9 Special Case**:
The plugin includes special handling for compromised accounts identified in Hardfork 9:

```cpp
if (hardfork_id == STEEM_HARDFORK_0_9) {
    // Add special recovery key for compromised accounts
    db.create<key_lookup_object>([&](key_lookup_object& o) {
        o.key = public_key_type("STM7sw22HqsXbz7D2CmJfmMwt9rimtk518dRzsR1f8Cgw52dQR1pR");
        o.account = compromised_account_name;
    });
}
```

### Key Caching

The plugin uses a temporary cache during operation processing:
- **Cache Purpose**: Optimize updates when multiple operations affect same account
- **Cache Lifetime**: Cleared after each operation
- **Cache Strategy**: Stores keys before update to detect changes

### Update Algorithm

When an account's authority changes:

1. **Pre-operation**: Cache existing keys
2. **Post-operation**: Get new keys
3. **Diff calculation**: Compare old vs new
4. **Index update**:
   - Add new keys not in cache
   - Remove cached keys not in new set

## Resource Requirements

### Disk Space

- **State Data**: 1-5MB (in shared memory file)
- **No persistent storage**: Data reconstructed from blockchain

### CPU Usage

- **Minimal**: Updates only on authority changes
- **Replay**: Moderate (builds entire index)

### Network

- **No network usage**: State-only plugin

## Best Practices

### For Node Operators

1. **Enable from start**: Always enable before replay
2. **Monitor memory**: Track index size growth
3. **Regular verification**: Periodically verify key lookups work
4. **Backup strategy**: Index rebuilds from blockchain replay

### For API Providers

1. **Enable plugin**: Required for `account_by_key_api`
2. **Cache results**: Key-to-account mappings rarely change
3. **Handle multiples**: One key may map to multiple accounts
4. **Format consistency**: Ensure key format matches client expectations

### For Wallet Developers

1. **Batch queries**: Query multiple keys at once for efficiency
2. **Handle empty results**: Not all keys are registered on-chain
3. **Key normalization**: Standardize key format before queries
4. **Error handling**: Gracefully handle API unavailability

## Security Considerations

### Privacy

**Key Exposure**: Public keys are publicly visible on blockchain
**Account Privacy**: Plugin reveals which accounts use which keys
**Recommendation**: Use unique keys per account for privacy

### Key Reuse

**Detection**: Plugin makes key reuse visible
**Implications**: Multiple accounts sharing keys can be identified
**Best Practice**: Avoid reusing keys across accounts

## Related Documentation

- [account_by_key_api Plugin](account_by_key_api.md) - RPC API interface
- [chain Plugin](chain.md) - Core blockchain database
- [cli_wallet Guide](../getting-started/cli-wallet-guide.md) - Wallet key management

## Source Code

- **Implementation**: [src/plugins/account_by_key/account_by_key_plugin.cpp](../../src/plugins/account_by_key/account_by_key_plugin.cpp)
- **Objects**: [src/plugins/account_by_key/include/steem/plugins/account_by_key/account_by_key_objects.hpp](../../src/plugins/account_by_key/include/steem/plugins/account_by_key/account_by_key_objects.hpp)
- **Plugin Header**: [src/plugins/account_by_key/include/steem/plugins/account_by_key/account_by_key_plugin.hpp](../../src/plugins/account_by_key/include/steem/plugins/account_by_key/account_by_key_plugin.hpp)

## License

See [LICENSE.md](../../LICENSE.md)
