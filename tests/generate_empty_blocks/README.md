# Generate Empty Blocks

Utility for generating empty blocks for testing purposes.

## Overview

This directory contains utilities for creating blockchain data with empty blocks (no transactions). Useful for:
- Testing block production mechanisms
- Benchmarking block processing
- Creating minimal test blockchains
- Testing time-based protocol features

## Status

**Note**: Currently commented out in parent [CMakeLists.txt](../CMakeLists.txt):
```cmake
#add_subdirectory( generate_empty_blocks )
```

Not actively built or used in current test suite.

## Purpose

Generate test blockchain data consisting only of:
- Block headers
- Witness signatures
- Timestamp progression
- No user transactions

## Use Cases

1. **Performance Testing**: Measure baseline block processing speed
2. **Time-Based Testing**: Test features that depend on block timestamps
3. **Minimal Blockchain**: Create smallest possible blockchain for testing
4. **Witness Schedule**: Test witness rotation without transaction overhead

## Build Configuration

From [CMakeLists.txt](CMakeLists.txt):

```cmake
# Currently disabled
# To enable:
add_subdirectory( generate_empty_blocks )
```

## License

See [LICENSE](../../LICENSE.md)
