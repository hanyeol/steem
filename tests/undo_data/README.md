# Undo Data

Utilities for testing database undo/redo functionality.

## Overview

Provides helper classes for testing the database undo/redo mechanism. Simplifies writing tests that verify state consistency across database sessions and rollbacks.

## Files

| File | Purpose |
|------|---------|
| **[undo.hpp](undo.hpp)** | `undo_scenario` helper class for undo testing |

## undo_scenario Class

Template class for testing database object lifecycle with undo functionality.

### Purpose

Simplifies undo tests by:
- Wrapping `database::create`, `database::modify`, `database::remove`
- Tracking old object values before modifications
- Verifying state restoration after undo

### Usage

```cpp
#include "../undo_data/undo.hpp"

BOOST_AUTO_TEST_CASE( undo_test )
{
    // Create undo scenario for account objects
    undo_scenario<account_object> scenario(db);

    // Start session
    auto session = db.start_undo_session();

    // Remember state before changes
    scenario.remember_old_values();

    // Make changes
    const auto& alice = scenario.create([](account_object& a) {
        a.name = "alice";
        a.balance = asset(1000, STEEM_SYMBOL);
    });

    scenario.modify(alice, [](account_object& a) {
        a.balance = asset(2000, STEEM_SYMBOL);
    });

    // Undo changes
    session.undo();

    // Verify state restored
    scenario.check();  // Throws if state doesn't match
}
```

### Operation Types

```cpp
namespace u_types
{
    enum op_type {
        create,  // Create new object
        modify,  // Modify existing object
        remove   // Remove object
    };
};
```

### Methods

#### create()

Wrapper for `database::create()`:

```cpp
template<typename CALL>
const Object& create(CALL callback)
{
    return db.create<Object>(callback);
}

// Usage
const auto& obj = scenario.create([](my_object& o) {
    o.field = value;
});
```

#### modify()

Wrapper for `database::modify()`:

```cpp
template<typename CALL>
void modify(const Object& obj, CALL callback)
{
    db.modify(obj, callback);
}

// Usage
scenario.modify(obj, [](my_object& o) {
    o.field = new_value;
});
```

#### remove()

Wrapper for `database::remove()`:

```cpp
void remove(const Object& obj)
{
    db.remove(obj);
}

// Usage
scenario.remove(obj);
```

#### remember_old_values()

Store current database state for later comparison:

```cpp
void remember_old_values()
{
    // Saves all Object instances to old_values list
}
```

#### check()

Verify database state matches remembered values:

```cpp
void check()
{
    // Throws if current state != old_values
}
```

## Example Test Pattern

```cpp
#include "../db_fixture/database_fixture.hpp"
#include "../undo_data/undo.hpp"

BOOST_FIXTURE_TEST_SUITE( undo_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( test_create_undo )
{
    try {
        undo_scenario<account_object> scenario(db);

        // Remember initial state
        scenario.remember_old_values();

        // Start undo session
        auto session = db.start_undo_session();

        // Create object
        const auto& alice = scenario.create([](account_object& a) {
            a.name = "alice";
            a.balance = asset(1000, STEEM_SYMBOL);
        });

        BOOST_REQUIRE( db.find_account("alice") != nullptr );

        // Undo
        session.undo();

        // Verify creation was undone
        scenario.check();
        BOOST_REQUIRE( db.find_account("alice") == nullptr );
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( test_modify_undo )
{
    try {
        ACTORS( (alice) )
        fund( "alice", 1000 );

        undo_scenario<account_object> scenario(db);

        // Remember state before modification
        scenario.remember_old_values();

        auto session = db.start_undo_session();

        // Modify account
        const auto& alice_acc = db.get_account("alice");
        scenario.modify(alice_acc, [](account_object& a) {
            a.balance = asset(2000, STEEM_SYMBOL);
        });

        BOOST_REQUIRE_EQUAL( alice_acc.balance.amount.value, 2000 );

        // Undo
        session.undo();

        // Verify modification was undone
        scenario.check();
        BOOST_REQUIRE_EQUAL( db.get_account("alice").balance.amount.value, 1000 );
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( test_remove_undo )
{
    try {
        ACTORS( (alice) )

        undo_scenario<account_object> scenario(db);
        scenario.remember_old_values();

        auto session = db.start_undo_session();

        // Remove account
        const auto& alice_acc = db.get_account("alice");
        scenario.remove(alice_acc);

        BOOST_REQUIRE( db.find_account("alice") == nullptr );

        // Undo
        session.undo();

        // Verify removal was undone
        scenario.check();
        BOOST_REQUIRE( db.find_account("alice") != nullptr );
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
```

## Implementation Details

### State Storage

```cpp
std::list<Object> old_values;  // Stores copies of objects
```

Objects are copied before modifications to enable comparison after undo.

### Operation Dispatch

```cpp
template<typename CALL>
const Object* run_impl(const Object* old_obj, u_types::op_type op, CALL call)
{
    switch(op) {
        case u_types::create:
            assert(!old_obj);
            return &db.create<Object>(call);

        case u_types::modify:
            assert(old_obj);
            db.modify(*old_obj, call);
            return old_obj;

        case u_types::remove:
            assert(old_obj);
            db.remove(*old_obj);
            return nullptr;
    }
}
```

## Use Cases

1. **Session Rollback**: Verify undo restores exact state
2. **Nested Sessions**: Test multi-level undo/redo
3. **Fork Handling**: Validate state across blockchain forks
4. **Transaction Atomicity**: Ensure all-or-nothing semantics

## Related Tests

Used in [tests/undo_tests.cpp](../tests/undo_tests.cpp):
- Database session management
- Object lifecycle across undo
- Complex modification scenarios
- Fork simulation

## Additional Resources

- [Undo Tests](../tests/README.md#undo-tests)
- [Database Fixture](../db_fixture/README.md)
- [Chainbase Documentation](../../libraries/chainbase/README.md)

## License

See [LICENSE](../../LICENSE.md)
