# BMIC Objects

Test objects for BMIC (Block Modifying Irreversible Constraint) system testing.

## Overview

This directory contains test object definitions and utilities for testing the BMIC system. BMIC ensures database consistency by managing which blocks can modify irreversible state.

## Files

| File | Purpose |
|------|---------|
| **[bmic_manager_tests.hpp](bmic_manager_tests.hpp)** | Test object definitions and multi-index containers |
| **[CMakeLists.txt](CMakeLists.txt)** | Build configuration |

## Test Objects

### test_object

Basic test object with multi-index support:

```cpp
struct test_object
{
    chainbase::oid<test_object> id;
    uint32_t val;
    std::string name;
};
```

**Indexes**:
- `OrderedIndex` - Ordered by ID
- `CompositeOrderedIndex` - Composite key (name, val)

### test_object2

Secondary test object:

```cpp
struct test_object2
{
    chainbase::oid<test_object2> id;
    uint32_t val;
};
```

**Indexes**:
- `OrderedIndex2` - Ordered by ID
- `CompositeOrderedIndex2` - Composite index

## Multi-Index Containers

### test_object_index

```cpp
using test_object_index = boost::multi_index_container<test_object,
    indexed_by<
        ordered_unique<
            tag<OrderedIndex>,
            member<test_object, chainbase::oid<test_object>, &test_object::id>
        >,
        ordered_unique<
            tag<CompositeOrderedIndex>,
            composite_key<test_object,
                member<test_object, std::string, &test_object::name>,
                member<test_object, uint32_t, &test_object::val>
            >
        >
    >
>;
```

### test_object_index2

Similar structure for `test_object2`.

## Purpose

Used in BMIC tests to verify:
- Object creation and modification tracking
- Index integrity across forks
- Irreversible block constraints
- Undo/redo functionality with complex objects

## Usage in Tests

```cpp
#include "../bmic_objects/bmic_manager_tests.hpp"

BOOST_AUTO_TEST_CASE( bmic_test )
{
    bmic::test_object_index idx;

    // Create test object
    bmic::test_object obj(
        [](auto& o) {
            o.val = 42;
            o.name = "test";
        },
        1,
        std::allocator<bmic::test_object>()
    );

    // Insert into index
    idx.insert(obj);

    // Query by ID
    auto& by_id = idx.get<bmic::OrderedIndex>();
    auto itr = by_id.find(1);

    // Query by composite key
    auto& by_composite = idx.get<bmic::CompositeOrderedIndex>();
    auto itr2 = by_composite.find(std::make_tuple("test", 42));
}
```

## CMake Configuration

From [CMakeLists.txt](CMakeLists.txt):

```cmake
add_library( bmic_objects
             # Source files
           )

target_link_libraries( bmic_objects
                       chainbase
                       fc
                     )
```

Linked by `chain_test` executable.

## Related

- [BMIC Tests](../tests/README.md#bmic-tests) - Tests using these objects
- [Undo Data](../undo_data/README.md) - Undo system utilities

## License

See [LICENSE](../../LICENSE.md)
