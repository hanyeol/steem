# Vendor Libraries

Third-party dependencies bundled with the Steem source code.

## Contents

### RocksDB
High-performance embedded key-value database used by `account_history_rocksdb` plugin.

**Purpose**: Scalable account history storage as an alternative to memory-mapped chainbase.

**Source**: [facebook/rocksdb](https://github.com/facebook/rocksdb)

## Why Vendored?

Bundling dependencies:
- Ensures version compatibility
- Simplifies build process
- Reduces external dependencies
- Guarantees reproducible builds

## Build

Vendor libraries are built automatically as part of the main build process:

```bash
cd build
cmake ..
make
```

RocksDB is built when `account_history_rocksdb` plugin is enabled.

## Updates

Vendor libraries are updated periodically to incorporate:
- Security fixes
- Performance improvements
- Bug fixes
- New features

Updates are tested thoroughly before integration.

## Licensing

Each vendored library retains its original license. See individual library directories for license files.
