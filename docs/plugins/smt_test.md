# smt_test Plugin

Smart Media Token (SMT) testing and example operation generation for development and validation.

## Overview

The `smt_test` plugin is a development tool for testing Smart Media Token (SMT) functionality. It generates example SMT setup operations and validates SMT operation structures during development and testing phases.

**Plugin Type**: Development/Testing
**Dependencies**: [chain](chain.md)
**Memory**: Minimal (~10MB)
**Default**: Disabled

## Purpose

- **SMT Example Generation**: Create sample SMT setup operations for testing
- **Operation Validation**: Test SMT operation structure and serialization
- **Development Testing**: Validate SMT functionality during development
- **Operation Monitoring**: Track pre/post SMT operation execution
- **Reference Examples**: Provide working examples of SMT configurations

## Configuration

### Config File Options

```ini
# Enable the SMT test plugin
plugin = smt_test
```

### Command Line Options

```bash
steemd --plugin=smt_test
```

### Configuration Parameters

This plugin has no configurable parameters. It operates automatically when enabled.

## Functionality

### Example SMT Tokens

The plugin generates three example SMT configurations on startup:

#### ALPHA Token
- **Control Account**: alpha
- **Decimal Places**: 4
- **Generation Period**: 2017-08-10 to 2017-08-17
- **Launch Time**: 2017-08-21
- **Unit Ratio**: 1000:1 (fixed)
- **Beneficiaries**: founder_a (7%), founder_b (23%), founder_c (70%)

#### BETA Token
- **Control Account**: beta
- **Decimal Places**: 4
- **Generation Period**: 2017-06-01 to 2017-06-30
- **Launch Time**: 2017-07-01
- **Unit Ratio**: 50-100 (variable)
- **Beneficiaries**: fred (3/5), george (2/5 + token share)

#### DELTA Token
- **Control Account**: delta
- **Decimal Places**: 5
- **Generation Period**: 2017-06-01 to 2017-06-30
- **Launch Time**: 2017-07-01
- **Unit Ratio**: 1000:1 (fixed)
- **Beneficiaries**: founder (100%)

### Operation Tracking

The plugin monitors:
- **Pre-operation validation**: Before operations are applied
- **Post-operation tracking**: After operations complete
- **Operation structure**: Validates SMT operation formats

## Build Requirements

### SMT Support Enabled

The plugin only functions when built with SMT support:

```bash
cd build
cmake -DENABLE_SMT_SUPPORT=ON -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) steemd
```

### Without SMT Support

When built without `-DENABLE_SMT_SUPPORT=ON`, the plugin:
- Compiles successfully
- Does not generate example operations
- Has minimal runtime impact

## Use Cases

### Development Testing

**Purpose**: Test SMT operations during development

```ini
plugin = chain smt_test

# Build with SMT support
# Check logs for example operations
```

### Operation Examples

**Purpose**: Generate reference SMT operations

```bash
# Start node with plugin enabled
./steemd --plugin=smt_test

# Check logs for JSON output
# Example operations appear in logs:
# "ALPHA example tx: ..."
# "BETA example tx: ..."
# "DELTA example tx: ..."
```

### Integration Testing

**Purpose**: Validate SMT operation serialization

```ini
plugin = chain smt_test

# Plugin validates:
# - SMT setup operations
# - Cap reveal operations
# - Generation policies
# - Beneficiary distributions
```

## Monitoring

### Log Messages

**Startup**:
```
Initializing smt_test plugin
ALPHA example tx: {...}
BETA example tx: {...}
DELTA example tx: {...}
```

**Operation Tracking**:
```
# Pre-operation validation
# Post-operation completion
# (Logged by internal handlers)
```

### Example Output

The plugin logs complete SMT operation structures including:
- `smt_setup_operation` - Token configuration
- `smt_cap_reveal_operation` - Commitment reveals
- Generation policies - Token distribution rules

## Troubleshooting

### No Example Operations

**Problem**: No SMT examples in logs

**Check**:
```bash
# Verify SMT support enabled
./steemd --version | grep -i smt

# Check build configuration
grep ENABLE_SMT_SUPPORT build/CMakeCache.txt
```

**Solutions**:
1. Rebuild with `-DENABLE_SMT_SUPPORT=ON`
2. Verify plugin is enabled in config
3. Check logs at startup

### Plugin Not Loading

**Problem**: smt_test plugin fails to initialize

**Symptoms**:
```
Error loading plugin: smt_test
```

**Solutions**:
1. Ensure chain plugin is enabled
2. Check plugin dependencies
3. Verify build includes plugin

## Security Considerations

### Development Only

**Important**: This plugin is for development/testing:
- Not intended for production use
- Contains hardcoded test data
- No security-sensitive operations

### Safe Operations

The plugin:
- Only reads blockchain state
- Does not modify database
- Does not process user input
- Generates examples on startup only

## Technical Details

### Operation Structure

The plugin demonstrates SMT operations including:

**Capped Generation Policy**:
- Pre-soft-cap unit distributions
- Post-soft-cap unit distributions
- Minimum/maximum unit ratios
- Hard cap commitments

**Setup Operation**:
- Control account specification
- Decimal precision configuration
- Generation time windows
- Launch time scheduling

**Cap Reveal Operation**:
- Commitment reveals
- Minimum commitment values
- Hard cap values

### Internal Architecture

```
smt_test_plugin
    |
    +-- pre_operation_visitor (validates before apply)
    +-- post_operation_visitor (tracks after apply)
    +-- test_alpha() (generates ALPHA example)
    +-- test_beta() (generates BETA example)
    +-- test_delta() (generates DELTA example)
```

### Database Interaction

- Reads account objects
- No database modifications
- Operates in read-only mode

## Related Documentation

- [SMT Operations](../technical-reference/smt-operations.md) - SMT operation reference
- [Testing Guide](../development/testing.md) - General testing procedures
- [Plugin Development](../development/plugin.md) - Creating plugins

## Source Code

- **Implementation**: [libraries/plugins/smt_test/smt_test_plugin.cpp](../../libraries/plugins/smt_test/smt_test_plugin.cpp)
- **Header**: [libraries/plugins/smt_test/include/steem/plugins/smt_test/smt_test_plugin.hpp](../../libraries/plugins/smt_test/include/steem/plugins/smt_test/smt_test_plugin.hpp)
- **Objects**: [libraries/plugins/smt_test/include/steem/plugins/smt_test/smt_test_objects.hpp](../../libraries/plugins/smt_test/include/steem/plugins/smt_test/smt_test_objects.hpp)

## License

See [LICENSE.md](../../LICENSE.md)
