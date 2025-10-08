# Utilities Library

Common utility functions and helpers used across the Steem codebase.

## Overview

The `utilities` library provides:
- String manipulation and parsing
- Encoding/decoding utilities
- Benchmark helpers
- Git version information
- Common helpers for blockchain operations

## Key Components

### String Utilities
Functions for string processing, validation, and conversion used throughout the codebase.

### Benchmark Utilities
Performance measurement and profiling helpers for optimization work.

### Git Version
Embedded version information from git repository for build tracking.

## Usage

Used internally by:
- `steem_chain` - Core blockchain logic
- `steem_wallet` - Wallet operations
- Plugins - Various utility needs

## Build

```bash
cd build
make steem_utilities
```

## Dependencies

- `fc` - Foundational utilities
- `steem_protocol` - Protocol types
