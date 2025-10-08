# Size Checker

Utility to check and report sizes of blockchain types and operations.

## Overview

`size_checker` analyzes and reports:
- Operation sizes (in bytes)
- Transaction sizes
- Type sizes
- Serialization overhead

## Building

```bash
cd build
make size_checker
```

## Usage

```bash
./size_checker
```

## Output

Reports sizes for:

### Operations
```
transfer_operation: 120 bytes
vote_operation: 80 bytes
comment_operation: 450 bytes
...
```

### Transactions
```
Minimum transaction: 50 bytes
Maximum transaction: varies
Average transaction: ~200 bytes
```

### Impact

Size information is important for:
- Network bandwidth planning
- Block size calculations
- Fee structures
- Performance optimization

## Use Cases

### Protocol Development

- Optimize operation structures
- Plan protocol changes
- Estimate bandwidth requirements

### Performance Analysis

- Identify large operations
- Optimize serialization
- Network efficiency

### Economic Planning

- Bandwidth cost calculations
- Fee estimations
- Resource credit planning

## Example Output

```
=== Operation Sizes ===
transfer_operation: 120
vote_operation: 80
comment_operation: 450
custom_json_operation: 250

=== Maximum Sizes ===
Max operation: 65536
Max transaction: 65536
Max block: 2097152
```

## Development

Update when:
- Adding new operations
- Changing operation structures
- Modifying serialization

Helps ensure operations stay within size limits.
