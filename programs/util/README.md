# Utilities

Collection of utility programs for development, testing, and blockchain operations.

## Overview

The `util` directory contains various helper programs:

- `get_dev_key` - Generate development keys
- `inflation_model` - Model Steem inflation
- `saltpass.py` - Generate password hashes for API authentication
- `schema_test` - Test schema generation
- `sign_digest` - Sign data with private key
- `sign_transaction` - Sign transaction offline
- `test_block_log` - Test block log operations
- `test_fixed_string` - Test fixed string implementation
- `test_shared_mem` - Test shared memory
- `test_sqrt` - Test sqrt implementation
- `newplugin.py` - Generate new plugin skeleton

## Building

```bash
cd build
make  # Builds all utilities
```

Individual utilities are in `programs/util/`.

## Utility Programs

### get_dev_key

Generate deterministic development keys.

```bash
./get_dev_key prefix-name-role
```

Example:
```bash
./get_dev_key init-alice-owner
./get_dev_key init-alice-active
```

**Use**: Development and testing only.

### inflation_model

Model Steem inflation over time.

```bash
./inflation_model
```

Outputs inflation projections showing:
- Annual inflation rate
- Token supply growth
- Distribution to witnesses, curators, authors

### saltpass.py

Generate salted password hashes for API authentication.

```bash
python3 saltpass.py
```

Prompts for password, outputs:
- `password_hash_b64`
- `password_salt_b64`

Used in `config.ini`:
```ini
api-user = {"username":"user","password_hash_b64":"...","password_salt_b64":"...","allowed_apis":["database_api"]}
```

### sign_digest

Sign arbitrary data with a private key.

```bash
./sign_digest
```

Interactive program for signing digests.

**Use**: Offline signing, testing cryptographic operations.

### sign_transaction

Sign transactions offline.

```bash
./sign_transaction transaction.json private_key
```

**Use**: Cold wallet signing, secure transaction creation.

### test_block_log

Test block log read/write operations.

```bash
./test_block_log block_log_file
```

**Use**: Debugging block log issues, testing block log integrity.

### test_shared_mem

Test shared memory operations.

```bash
./test_shared_mem
```

**Use**: Testing chainbase shared memory functionality.

### schema_test

Test schema generation.

```bash
./schema_test
```

**Use**: Validate schema generation for types and operations.

## Python Scripts

### newplugin.py

Generate skeleton code for new plugins.

```bash
python3 newplugin.py
```

Interactive script that asks:
- Plugin name
- Plugin type (state/API)
- Dependencies

Generates:
- Plugin header file
- Plugin source file
- CMakeLists.txt
- Basic structure

**Output**: `output/` directory with plugin files

### Usage Example

```
$ python3 newplugin.py
Enter plugin name: my_feature
Enter plugin type (state/api): state
Enter dependencies (comma-separated): chain,p2p

Generating plugin skeleton...
Created: output/my_feature_plugin.hpp
Created: output/my_feature_plugin.cpp
Created: output/CMakeLists.txt
```

Move generated files to `external_plugins/my_feature/`.

## Development Workflows

### Creating a New Plugin

1. Generate skeleton:
   ```bash
   python3 newplugin.py
   ```

2. Move to external_plugins:
   ```bash
   mv output/my_feature external_plugins/
   ```

3. Implement plugin logic

4. Build and test

### Testing Shared Memory

```bash
# Test shared memory operations
./test_shared_mem

# Test block log
./test_block_log witness_node_data_dir/blockchain/block_log
```

### Offline Transaction Signing

```bash
# 1. Create transaction (on online machine)
# 2. Transfer transaction.json to offline machine
# 3. Sign offline
./sign_transaction transaction.json WIF_PRIVATE_KEY > signed.json
# 4. Broadcast signed transaction
```

## Security Tools

### Password Hash Generation

For securing APIs:

```bash
python3 saltpass.py
# Enter password when prompted
# Copy output to config.ini
```

### Key Generation

Development keys only:

```bash
./get_dev_key testnet-account-owner
```

**Warning**: NEVER use get_dev_key for production keys!

## Testing Tools

### Inflation Modeling

```bash
./inflation_model > inflation_projection.txt
```

Useful for:
- Economic analysis
- Understanding token distribution
- Planning economic changes

### Schema Validation

```bash
./schema_test
```

Ensures:
- All types are properly reflected
- Operations serialize correctly
- Schema generation works

## Build-Time Tools

Some utilities are used during the build process:

- **cat_parts**: Concatenate hardfork definition parts
- **build_helpers**: CMake build utilities

These are typically not run manually.

## Additional Resources

- [Plugin Development](../../doc/plugin.md)
- [Testing Guide](../../doc/testing.md)
- [Building Instructions](../../doc/building.md)
