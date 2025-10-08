# SMT Test Plugin

Testing utilities for Smart Media Tokens (SMT).

## Overview

The smt_test plugin provides:
- SMT operation testing
- Token creation scenarios
- SMT validation testing
- Development utilities

## Configuration

```ini
plugin = smt_test

# Requires SMT support enabled
# cmake -DENABLE_SMT_SUPPORT=ON
```

## Features

- **SMT Testing**: Token operation tests
- **Scenarios**: Pre-defined test cases
- **Validation**: Token rule checking
- **Development**: SMT debugging

## Use Cases

- SMT development
- Token testing
- Integration testing
- Scenario validation

## Dependencies

- `chain` - SMT operations
- Requires BUILD_STEEM_TESTNET=ON

## Note

For development and testing only. Requires SMT support compiled in.
