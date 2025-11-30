# Comment Permission Control Feature

## Overview

This document describes the implementation of a feature that allows post authors to control who can comment on their posts. Authors can specify a whitelist of accounts that are allowed to comment, or leave it unrestricted.

## Feature Description

When creating a post, the author can specify one of three comment permission modes:

1. **Open (default)** - Anyone can comment (existing behavior)
2. **Restricted to specific accounts** - Only whitelisted accounts can comment
3. **No comments allowed** - Nobody can comment (empty whitelist)

## Implementation Design

### 1. Protocol Changes

#### New Operation Field

Add a new optional field to the `comment_operation` structure in [src/core/protocol/include/steem/protocol/steem_operations.hpp](../../../src/core/protocol/include/steem/protocol/steem_operations.hpp):

```cpp
struct comment_operation : public base_operation
{
   // ... existing fields ...

   optional< flat_set< account_name_type > > allowed_comment_accounts;

   void validate() const;
   // ...
};
```

**Field semantics:**
- `nullopt` (not set) - Open to all comments (default, backward compatible)
- Empty set `{}` - No comments allowed
- Set with accounts `{"alice", "bob"}` - Only these accounts can comment

#### Validation Rules

In the `validate()` method:

```cpp
void comment_operation::validate() const
{
   // ... existing validation ...

   if( allowed_comment_accounts.valid() )
   {
      // Check for duplicate accounts (flat_set should prevent this)
      // Validate each account name format
      for( const auto& account : *allowed_comment_accounts )
      {
         FC_ASSERT( is_valid_account_name( account ),
                    "Invalid account name in allowed_comment_accounts: ${a}",
                    ("a", account) );
      }

      // Optional: Set maximum whitelist size to prevent abuse
      FC_ASSERT( allowed_comment_accounts->size() <= 1000,
                 "Comment whitelist cannot exceed 1000 accounts" );
   }
}
```

### 2. Chain State Changes

#### Database Object

Add a new field to `comment_object` in [src/core/chain/include/steem/chain/comment_object.hpp](../../../src/core/chain/include/steem/chain/comment_object.hpp):

```cpp
class comment_object : public object< comment_object_type, comment_object >
{
   // ... existing fields ...

   // Comment permission control
   // If not set (nullopt), all accounts can comment
   // If set with accounts, only those accounts can comment
   // If set but empty (empty vector), no one can comment
   optional< shared_vector< account_name_type > > allowed_comment_accounts;

   // ... rest of the class ...
};
```

**Note:** Use `shared_vector` instead of `flat_set` because chainbase requires allocator-aware containers for memory-mapped storage.

#### Index Updates

Update the `FC_REFLECT` macro to include the new field:

```cpp
FC_REFLECT( steem::chain::comment_object,
            // ... existing fields ...
            (allowed_comment_accounts)
          )
```

### 3. Evaluator Changes

#### Comment Evaluator

Modify `comment_evaluator` in [src/core/chain/steem_evaluator.cpp](../../../src/core/chain/steem_evaluator.cpp):

```cpp
void comment_evaluator::do_apply( const comment_operation& o )
{
   // ... existing code ...

   // Handle parent comment (reply) permission check
   if( o.parent_author != STEEM_ROOT_POST_PARENT_ACCOUNT )
   {
      const auto& parent = _db.get_comment( o.parent_author, o.parent_permlink );

      // Check if this account is allowed to comment
      if( parent.allowed_comment_accounts.valid() )
      {
         const auto& allowed_accounts = *parent.allowed_comment_accounts;

         if( allowed_accounts.empty() )
         {
            // Empty whitelist means no comments allowed
            FC_ASSERT( false,
                      "Comments are disabled for this post" );
         }
         else
         {
            // Check if commenter is in the whitelist
            auto it = std::find( allowed_accounts.begin(),
                                allowed_accounts.end(),
                                o.author );
            FC_ASSERT( it != allowed_accounts.end(),
                      "Account ${a} is not allowed to comment on this post",
                      ("a", o.author) );
         }
      }
   }

   // ... existing code for creating/updating comment ...

   // For root posts (not replies), set the comment permission if specified
   if( o.parent_author == STEEM_ROOT_POST_PARENT_ACCOUNT )
   {
      if( o.allowed_comment_accounts.valid() )
      {
         _db.modify( comment, [&]( comment_object& c )
         {
            c.allowed_comment_accounts = shared_vector< account_name_type >(
               o.allowed_comment_accounts->begin(),
               o.allowed_comment_accounts->end(),
               c.allowed_comment_accounts.get_allocator()
            );
         });
      }
   }

   // ... rest of the function ...
}
```

### 4. API Changes

#### Database API

Add a method to `database_api` to query comment permissions in [src/plugins/apis/database_api/database_api.cpp](../../../src/plugins/apis/database_api/database_api.cpp):

```cpp
struct get_comment_permissions_return
{
   bool comments_enabled = true;
   optional< vector< account_name_type > > allowed_accounts;
};

get_comment_permissions_return database_api::get_comment_permissions(
   string author,
   string permlink ) const
{
   return my->_db.with_read_lock( [&]()
   {
      get_comment_permissions_return result;

      const auto& comment = my->_db.get_comment( author, permlink );

      if( comment.allowed_comment_accounts.valid() )
      {
         if( comment.allowed_comment_accounts->empty() )
         {
            result.comments_enabled = false;
         }
         else
         {
            result.allowed_accounts = vector< account_name_type >(
               comment.allowed_comment_accounts->begin(),
               comment.allowed_comment_accounts->end()
            );
         }
      }

      return result;
   });
}
```

## Usage Examples

### Creating a Post with Comment Restrictions

#### Open to all (default):

```json
{
  "type": "comment_operation",
  "value": {
    "parent_author": "",
    "parent_permlink": "blog",
    "author": "alice",
    "permlink": "my-post",
    "title": "My Post",
    "body": "Post content",
    "json_metadata": "{}"
  }
}
```

#### Only specific accounts can comment:

```json
{
  "type": "comment_operation",
  "value": {
    "parent_author": "",
    "parent_permlink": "blog",
    "author": "alice",
    "permlink": "my-post",
    "title": "My Post",
    "body": "Post content",
    "json_metadata": "{}",
    "allowed_comment_accounts": ["bob", "charlie"]
  }
}
```

#### No comments allowed:

```json
{
  "type": "comment_operation",
  "value": {
    "parent_author": "",
    "parent_permlink": "blog",
    "author": "alice",
    "permlink": "my-post",
    "title": "My Post",
    "body": "Post content",
    "json_metadata": "{}",
    "allowed_comment_accounts": []
  }
}
```

### Querying Comment Permissions

Using `database_api`:

```javascript
// Check if you can comment on a post
const permissions = await api.call('database_api', 'get_comment_permissions', {
  author: 'alice',
  permlink: 'my-post'
});

if (!permissions.comments_enabled) {
  console.log('Comments are disabled for this post');
} else if (permissions.allowed_accounts) {
  console.log('Only these accounts can comment:', permissions.allowed_accounts);
} else {
  console.log('Anyone can comment');
}
```

## Testing

### Unit Tests

Add tests to [tests/tests/operation_tests.cpp](../../../tests/tests/operation_tests.cpp):

```cpp
BOOST_AUTO_TEST_SUITE( comment_permission_tests )

BOOST_AUTO_TEST_CASE( comment_permission_validation )
{ try {
   BOOST_TEST_MESSAGE( "Testing comment permission validation" );

   comment_operation op;
   op.parent_author = "";
   op.parent_permlink = "blog";
   op.author = "alice";
   op.permlink = "test-post";
   op.title = "Test";
   op.body = "Body";

   // Valid: No restriction
   REQUIRE_OP_VALIDATION_SUCCESS( op, comment_operation );

   // Valid: Empty whitelist (no comments)
   op.allowed_comment_accounts = flat_set<account_name_type>();
   REQUIRE_OP_VALIDATION_SUCCESS( op, comment_operation );

   // Valid: Whitelist with accounts
   op.allowed_comment_accounts = flat_set<account_name_type>{"bob", "charlie"};
   REQUIRE_OP_VALIDATION_SUCCESS( op, comment_operation );

   // Invalid: Invalid account name
   op.allowed_comment_accounts = flat_set<account_name_type>{"invalid.name!"};
   REQUIRE_OP_VALIDATION_FAILURE( op, comment_operation );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( comment_permission_enforcement )
{ try {
   BOOST_TEST_MESSAGE( "Testing comment permission enforcement" );

   ACTORS( (alice)(bob)(charlie)(dan) )

   generate_block();

   // Alice creates a post with comment restrictions
   comment_operation post;
   post.parent_author = "";
   post.parent_permlink = "blog";
   post.author = "alice";
   post.permlink = "restricted-post";
   post.title = "Restricted Post";
   post.body = "Only bob and charlie can comment";
   post.allowed_comment_accounts = flat_set<account_name_type>{"bob", "charlie"};

   push_transaction( post, alice_private_key );
   generate_block();

   // Bob can comment (in whitelist)
   comment_operation bob_comment;
   bob_comment.parent_author = "alice";
   bob_comment.parent_permlink = "restricted-post";
   bob_comment.author = "bob";
   bob_comment.permlink = "bobs-comment";
   bob_comment.body = "Bob's comment";

   push_transaction( bob_comment, bob_private_key );
   generate_block();

   // Dan cannot comment (not in whitelist)
   comment_operation dan_comment;
   dan_comment.parent_author = "alice";
   dan_comment.parent_permlink = "restricted-post";
   dan_comment.author = "dan";
   dan_comment.permlink = "dans-comment";
   dan_comment.body = "Dan's comment";

   REQUIRE_THROW( push_transaction( dan_comment, dan_private_key ), fc::exception );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( no_comments_allowed )
{ try {
   BOOST_TEST_MESSAGE( "Testing no comments allowed" );

   ACTORS( (alice)(bob) )

   generate_block();

   // Alice creates a post with no comments allowed
   comment_operation post;
   post.parent_author = "";
   post.parent_permlink = "blog";
   post.author = "alice";
   post.permlink = "no-comments";
   post.title = "No Comments";
   post.body = "Comments disabled";
   post.allowed_comment_accounts = flat_set<account_name_type>(); // Empty = no comments

   push_transaction( post, alice_private_key );
   generate_block();

   // Bob cannot comment
   comment_operation bob_comment;
   bob_comment.parent_author = "alice";
   bob_comment.parent_permlink = "no-comments";
   bob_comment.author = "bob";
   bob_comment.permlink = "bobs-comment";
   bob_comment.body = "Bob's comment";

   REQUIRE_THROW( push_transaction( bob_comment, bob_private_key ), fc::exception );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
```

### Integration Tests

Test the feature end-to-end:

1. Create posts with different permission settings
2. Attempt comments from whitelisted and non-whitelisted accounts
3. Query comment permissions via API
4. Verify error messages are clear and helpful

## Security Considerations

1. **Whitelist Size Limit** - Prevent abuse by limiting whitelist size (suggested: 1000 accounts)
2. **Gas/Resource Cost** - Comment operations with large whitelists may require more computational resources
3. **Backward Compatibility** - The `optional` field ensures existing operations remain valid
4. **Author Control** - Only the post author can set comment permissions during post creation
5. **Immutability** - Comment permissions cannot be changed after post creation (edits don't modify whitelist)

## Performance Considerations

1. **Memory Usage** - Each comment with a whitelist uses additional memory proportional to the number of accounts
2. **Lookup Performance** - Checking whitelist membership is O(n) with current implementation; consider optimization for large whitelists
3. **Serialization** - Additional data in operations increases transaction size

## Future Enhancements

Potential improvements for future hardforks:

1. **Update Permissions** - Allow authors to modify comment permissions after creation
2. **Role-Based Permissions** - Allow comments from accounts with certain reputation or stake levels
3. **Blacklist Mode** - Block specific accounts instead of whitelisting
4. **Time-Based Restrictions** - Enable/disable comments after a certain time period
5. **Delegate Permission Control** - Allow authors to delegate permission management to another account

## Related Documentation

- [Create Operation Guide](create-operation.md)
- [Plugin Development Guide](plugin.md)
- [Testing Guide](testing.md)
- [Hardfork Procedure](../../operations/hardfork-procedure.md)

## References

- **Protocol Operations**: [src/core/protocol/include/steem/protocol/steem_operations.hpp](../../../src/core/protocol/include/steem/protocol/steem_operations.hpp)
- **Chain Objects**: [src/core/chain/include/steem/chain/comment_object.hpp](../../../src/core/chain/include/steem/chain/comment_object.hpp)
- **Evaluators**: [src/core/chain/steem_evaluator.cpp](../../../src/core/chain/steem_evaluator.cpp)
- **Operation Tests**: [tests/tests/operation_tests.cpp](../../../tests/tests/operation_tests.cpp)
