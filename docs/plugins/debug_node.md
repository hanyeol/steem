# debug_node Plugin

Development and testing utilities for blockchain manipulation in testnet environments.

## Overview

The `debug_node` plugin provides powerful tools for testing and development by enabling direct manipulation of the blockchain state. It allows developers to generate blocks on demand, modify witness schedules, and control time progression for comprehensive testing scenarios.

**Plugin Type**: Development/Testing Only
**Dependencies**: [chain](chain.md)
**Memory**: Minimal (~10MB)
**Default**: Disabled (development only)

**WARNING**: This plugin should NEVER be enabled on production or mainnet nodes. It bypasses normal consensus rules and can compromise blockchain integrity.

## Purpose

- **Block Generation**: Produce blocks on demand with custom private keys
- **Time Control**: Fast-forward blockchain time for testing time-dependent operations
- **Witness Manipulation**: Temporarily modify witness signing keys for testing
- **State Editing**: Apply custom database edits for specific test scenarios
- **Integration Testing**: Simulate complex blockchain scenarios without mining delays

## Configuration

### Config File Options

```ini
# Enable debug_node plugin (TESTNET ONLY)
plugin = debug_node

# Optional: Database edit scripts to apply on startup
# debug-node-edit-script = /path/to/edit-script1.json
# debug-node-edit-script = /path/to/edit-script2.json
```

### Command Line Options

```bash
steemd \
  --plugin=debug_node \
  --debug-node-edit-script=/path/to/edits.json
```

### Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `debug-node-edit-script` | (none) | Path to JSON file with database edits to apply on startup |

## Build Requirements

The debug_node plugin is only available when building with testnet support:

```bash
cmake -DBUILD_STEEM_TESTNET=ON -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc) steemd
```

**Important**: Regular production builds will not include this plugin.

## Core Functionality

### Block Generation

Generate blocks on demand using a debug private key:

```cpp
// Generate 10 blocks
uint32_t blocks_generated = debug_generate_blocks(
    debug_private_key_wif,  // WIF-encoded private key
    10,                      // Number of blocks to generate
    0,                       // Skip flags (0 = validate normally)
    0                        // Missed blocks (0 = consecutive)
);
```

**Parameters**:
- `debug_key`: WIF-format private key for signing blocks
- `count`: Number of blocks to generate
- `skip`: Validation skip flags (use `database::skip_nothing` for normal validation)
- `miss_blocks`: Number of block slots to skip between generated blocks
- `edit_if_needed`: Automatically modify witness keys if needed (default: true)

### Time-Based Generation

Generate blocks until a specific time:

```cpp
// Generate blocks until specific timestamp
uint32_t blocks = debug_generate_blocks_until(
    debug_private_key_wif,
    target_time,        // fc::time_point_sec
    true,              // generate_sparsely (skip intermediate slots)
    0                  // skip flags
);
```

**Sparse Generation**: When enabled, only generates blocks at the start and end of the time range, skipping intermediate slots for faster testing.

### Witness Key Modification

The plugin can automatically modify witness signing keys to match your debug key:

```cpp
// When edit_if_needed=true, the plugin will:
// 1. Check if scheduled witness has matching signing key
// 2. If not, temporarily update witness signing key
// 3. Generate block with updated key
// 4. Key change persists until block is popped or chain resets
```

### Database State Manipulation

Apply custom state changes via the `debug_update` method:

```cpp
debug_update([&](database& db) {
    // Modify database objects
    db.modify(witness_obj, [&](witness_object& w) {
        w.signing_key = new_public_key;
    });
}, skip_flags);
```

**How it works**:
1. Registers callback for current block
2. Pops the head block
3. Re-applies block with registered modifications
4. Changes are applied during block re-application

## Use Cases

### Unit Testing

Test time-dependent operations like vesting withdrawals:

```cpp
// Setup test
generate_blocks(1);
BOOST_TEST_MESSAGE("Starting withdrawal...");
start_withdraw_operation op;
op.account = "alice";
op.vesting_shares = ASSET("100.000000 VESTS");

// Submit operation
push_transaction(op);

// Fast-forward 7 days (1 week vesting period)
auto target_time = db.head_block_time() + fc::days(7);
debug_generate_blocks_until(debug_key, target_time, true);

// Verify withdrawal completed
auto& alice = db.get_account("alice");
BOOST_CHECK_EQUAL(alice.withdrawn, ASSET("100.000000 VESTS"));
```

### Integration Testing

Test complex multi-step scenarios:

```cpp
// Generate initial state
debug_generate_blocks(debug_key, 100);

// Create test accounts and fund them
create_account("alice");
create_account("bob");
fund("alice", ASSET("1000.000 STEEM"));

// Test voting scenario
vote("alice", "bob", "post-1", 10000);
generate_blocks(1);

// Fast-forward to payout
auto payout_time = db.calculate_discussion_payout_time(comment);
debug_generate_blocks_until(debug_key, payout_time, false);

// Verify rewards distributed
verify_rewards("bob");
```

### Hardfork Testing

Test behavior across hardfork boundaries:

```cpp
// Generate blocks up to hardfork
auto hf_time = HARDFORK_0_20_TIME;
debug_generate_blocks_until(debug_key, hf_time - fc::seconds(30), true);

BOOST_TEST_MESSAGE("Before HF20...");
test_pre_hardfork_behavior();

// Cross hardfork boundary
debug_generate_blocks_until(debug_key, hf_time + fc::seconds(30), false);

BOOST_TEST_MESSAGE("After HF20...");
test_post_hardfork_behavior();
```

## API Methods

The debug_node plugin does not expose public APIs. All functionality is accessed programmatically within test code.

### Internal Methods

**debug_generate_blocks**:
- Generates specified number of blocks
- Returns count of blocks actually generated
- Automatically handles witness key matching

**debug_generate_blocks_until**:
- Generates blocks until target time reached
- Supports sparse generation for performance
- Returns total blocks generated

**debug_update**:
- Registers state modification callback
- Re-applies current block with modifications
- Changes persist in blockchain state

## Monitoring

### Block Generation

Monitor block generation in test output:

```
slot: 12345   time: 2023-01-15T10:30:00   scheduled key is: STM7...   dbg key is: STM8...
Modified key for witness alice
Generated block #12345 by alice
```

### State Changes

Enable logging to see state modifications:

```cpp
// Enable debug logging
logging = true;

// Logs will show:
// - Witness key modifications
// - Block generation details
// - Skip flags applied
// - Database updates
```

## Testing Best Practices

### 1. Isolated Test Fixtures

Use clean database fixture for each test:

```cpp
BOOST_FIXTURE_TEST_CASE(my_test, clean_database_fixture)
{
    // Fresh database state
    generate_blocks(1);

    // Test logic

    // Cleanup automatic
}
```

### 2. Deterministic Key Usage

Use consistent private keys:

```cpp
// Standard test key
const std::string debug_key = "5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n";
```

### 3. Validate State After Changes

Always verify database state after operations:

```cpp
generate_blocks(10);

BOOST_REQUIRE_EQUAL(db.head_block_num(), 10);
BOOST_CHECK(db.get_account("alice").balance.amount > 0);
```

### 4. Use Skip Flags Carefully

Skip validation only when necessary:

```cpp
// Normal validation (recommended)
generate_blocks(10, database::skip_nothing);

// Skip signature validation (faster, less thorough)
generate_blocks(10, database::skip_transaction_signatures);

// Skip everything (dangerous, testing only)
generate_blocks(10, database::skip_transaction_dupe_check
                   | database::skip_tapos_check
                   | database::skip_witness_signature);
```

## Troubleshooting

### Private Key Mismatch

**Problem**: Blocks not generating, witness key errors

**Symptoms**:
```
Scheduled key is: STM7abc... dbg key is: STM8xyz...
```

**Solutions**:
1. Ensure `edit_if_needed = true` (default)
2. Use correct WIF private key format
3. Verify key corresponds to a witness

### Block Generation Stops

**Problem**: Fewer blocks generated than requested

**Symptoms**:
```
Requested 100 blocks, got 50
```

**Causes**:
- Witness key mismatch with `edit_if_needed = false`
- Skip flags preventing block production
- Database state preventing block generation

**Solutions**:
1. Enable `edit_if_needed`
2. Check skip flags are appropriate
3. Verify database state allows block production

### Time Not Advancing

**Problem**: Blockchain time stuck

**Symptoms**:
```
head_block_time: 2023-01-01T00:00:00 (not changing)
```

**Solutions**:
1. Generate at least one block to advance time
2. Use `debug_generate_blocks_until` for specific times
3. Verify blocks are actually being produced

## Security Considerations

### Production Environment

**NEVER enable debug_node on production nodes**:
- Bypasses consensus rules
- Allows arbitrary state modification
- Compromises blockchain integrity
- No authentication/authorization

### Testnet Safety

Even on testnets, limit access:

```bash
# Don't expose RPC if debug_node enabled
# No webserver-http-endpoint or webserver-ws-endpoint
```

### Code Review

Debug code should never reach production:

```cpp
#ifdef STEEM_BUILD_TESTNET
    // Debug operations only in testnet builds
    debug_generate_blocks(key, 100);
#else
    #error "Debug operations in production build!"
#endif
```

## Performance Considerations

### Sparse Block Generation

For large time jumps, use sparse generation:

```cpp
// Slow: Generate every block for 1 year
debug_generate_blocks(key, 365 * 24 * 3600 / 3);  // ~10M blocks

// Fast: Only generate start and end blocks
debug_generate_blocks_until(key,
    db.head_block_time() + fc::days(365),
    true  // sparse mode
);
```

### Skip Flags for Speed

Balance speed vs validation:

```cpp
// Fastest (minimal validation)
const uint32_t fast_skip =
    database::skip_witness_signature |
    database::skip_transaction_signatures |
    database::skip_transaction_dupe_check |
    database::skip_tapos_check;

generate_blocks(1000, fast_skip);

// Balanced (skip expensive checks)
const uint32_t balanced_skip =
    database::skip_transaction_signatures;

generate_blocks(1000, balanced_skip);
```

## Example Test Suite

Complete example integrating all features:

```cpp
BOOST_FIXTURE_TEST_SUITE(my_tests, clean_database_fixture)

BOOST_AUTO_TEST_CASE(test_voting_over_time)
{
    try {
        ACTORS((alice)(bob)(charlie))

        // Setup: Fund accounts
        fund("alice", ASSET("1000.000 STEEM"));
        vest("alice", ASSET("1000.000 STEEM"));

        // Create post
        comment_operation post;
        post.author = "bob";
        post.permlink = "test-post";
        post.title = "Test Post";
        post.body = "Content";
        push_transaction(post, alice_private_key);

        // Vote on post
        vote_operation vote;
        vote.voter = "alice";
        vote.author = "bob";
        vote.permlink = "test-post";
        vote.weight = 10000;
        push_transaction(vote, alice_private_key);

        generate_blocks(1);

        // Fast-forward to payout
        const auto& comment = db.get_comment("bob", "test-post");
        auto payout_time = db.calculate_discussion_payout_time(comment);

        debug_generate_blocks_until(
            get_debug_key(),
            payout_time + fc::seconds(3),
            true  // sparse
        );

        // Verify payout occurred
        const auto& bob_account = db.get_account("bob");
        BOOST_CHECK(bob_account.sbd_balance.amount > 0);

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
```

## Related Documentation

- [Testing Guide](../development/testing.md) - Comprehensive testing guide
- [Python Debug Node](../development/python-debug-node.md) - Python interface
- [Debug Node Plugin Dev Guide](../development/debug-node-plugin.md) - Advanced usage
- [chain Plugin](chain.md) - Blockchain database

## Source Code

- **Implementation**: [libraries/plugins/debug_node/debug_node_plugin.cpp](../../libraries/plugins/debug_node/debug_node_plugin.cpp)
- **Header**: [libraries/plugins/debug_node/include/steem/plugins/debug_node/debug_node_plugin.hpp](../../libraries/plugins/debug_node/include/steem/plugins/debug_node/debug_node_plugin.hpp)

## License

See [LICENSE.md](../../LICENSE.md)
