# JS Operation Serializer

Generates JavaScript serialization code for Steem operations.

## Overview

`js_operation_serializer` creates JavaScript/TypeScript bindings for Steem blockchain operations, enabling:
- Client-side transaction signing
- JavaScript SDK development
- Browser-based wallets
- Node.js applications

## Building

```bash
cd build
make js_operation_serializer
```

## Usage

```bash
./js_operation_serializer > operations.js
```

## Output

Generates JavaScript code containing:
- Operation type definitions
- Serialization functions
- Deserialization functions
- Type validators

## Use Cases

### JavaScript SDKs

Generated code is used by:
- steem-js
- dsteem
- Other JavaScript Steem libraries

### Browser Wallets

Enables client-side transaction creation:
- Create operations in browser
- Sign transactions locally
- Broadcast to network

### Type Safety

Provides TypeScript type definitions for:
- All Steem operations
- Asset types
- Account authorities
- Transaction structures

## Integration

### In JavaScript Projects

```javascript
// Import generated operations
import { operations } from './operations.js';

// Create transfer operation
const transfer = operations.transfer({
  from: 'alice',
  to: 'bob',
  amount: '10.000 STEEM',
  memo: 'payment'
});

// Serialize for signing
const serialized = operations.serialize(transfer);
```

## Development

When adding new operations:

1. Define operation in `libraries/protocol/`
2. Rebuild `js_operation_serializer`
3. Regenerate JavaScript bindings
4. Update JavaScript SDKs

## Additional Resources

- [Protocol Library](../../libraries/protocol/)
- [Steem Operations](../../libraries/protocol/include/steem/protocol/steem_operations.hpp)
