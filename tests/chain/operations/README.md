# Operation Tests

This directory contains unit tests for blockchain operations, organized by domain.

## Test Files

- **account_test.cpp** - Account operations (create, update, witness vote, post rate limit, clear null account)
- **comment_test.cpp** - Comment operations (comment, delete, vote, options, payout, nested comments, freeze)
- **vesting_test.cpp** - Vesting operations (transfer to vesting, withdraw vesting, vesting withdrawals, withdraw routes)
- **transfer_test.cpp** - Transfer operations
- **witness_test.cpp** - Witness operations (update, set properties, feed publish)
- **market_test.cpp** - Market operations (limit orders, convert, SBD interest, price feeds)
- **reward_test.cpp** - Reward operations (reward funds, claims decay)
- **escrow_test.cpp** - Escrow operations (transfer, approve, dispute, release)
- **proposal_test.cpp** - Proposal operations (create, update, delete)

## Test Suite

All tests in this directory use the `operation_tests` test suite for backward compatibility.

## Running Tests

```bash
# Run all operation tests
./tests/chain_test --run_test=operation_tests

# Run specific test file (by test case name)
./tests/chain_test -t operation_tests/account_create_validate
./tests/chain_test -t operation_tests/vesting_withdrawals
```

## Adding New Tests

When adding new operation tests:

1. Determine the appropriate domain file (account, comment, vesting, etc.)
2. If a new domain is needed, create a new `<domain>_test.cpp` file
3. Use the `operation_tests` test suite for consistency
4. Add the file to `CHAIN_TEST_SOURCES` in tests/CMakeLists.txt
