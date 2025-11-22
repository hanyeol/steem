# Reward Level System

## Overview

The Reward Level System is a feature that classifies accounts into different reward tiers (0-3) and applies differential reward rates based on their level. This mechanism allows for customized reward distribution strategies that can incentivize specific user behaviors or account characteristics.

**Important**: Both reward level changes and multiplier adjustments require **witness consensus** to ensure decentralized governance and prevent arbitrary manipulation of reward distribution.

## Reward Levels

The system defines four distinct reward levels:

- **Level 0**: Base level (default for new accounts)
- **Level 1**: Enhanced rewards
- **Level 2**: Premium rewards
- **Level 3**: Maximum rewards

## Architecture

### Core Components

#### 1. Account Object Extension

The reward level is stored as part of the account object in the blockchain state.

**Location**: `libraries/chain/include/steem/chain/account_object.hpp`

```cpp
// Add to account_object class
uint8_t reward_level = 0; // Default level 0
```

#### 2. Reward Level Proposal Operation

Witnesses propose reward level changes, which require consensus approval.

**Location**: `libraries/protocol/include/steem/protocol/steem_operations.hpp`

```cpp
struct reward_level_proposal_operation
{
   account_name_type proposing_witness;  // Witness proposing the change
   account_name_type target_account;     // Account to adjust
   uint8_t           new_reward_level;   // Proposed level (0-3)
   string            justification;      // Reason for the change

   void validate() const;
   void get_required_active_authorities( flat_set<account_name_type>& a ) const
   {
      a.insert( proposing_witness );
   }
};
```

**Validation**:
```cpp
void reward_level_proposal_operation::validate() const
{
   FC_ASSERT( is_valid_account_name( proposing_witness ), "Invalid witness account name" );
   FC_ASSERT( is_valid_account_name( target_account ), "Invalid target account name" );
   FC_ASSERT( new_reward_level <= 3, "Reward level must be between 0 and 3" );
   FC_ASSERT( justification.size() > 0 && justification.size() <= 1000,
              "Justification required (max 1000 characters)" );
}
```

#### 3. Reward Level Proposal Object

Track pending proposals and witness approvals.

**Location**: `libraries/chain/include/steem/chain/reward_level_objects.hpp`

```cpp
class reward_level_proposal_object : public object< reward_level_proposal_object_type, reward_level_proposal_object >
{
   public:
      template< typename Constructor, typename Allocator >
      reward_level_proposal_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      id_type                    id;
      account_name_type          target_account;
      uint8_t                    proposed_level;
      account_name_type          proposing_witness;
      time_point_sec             created;
      string                     justification;
      flat_set<account_name_type> approvals;  // Witnesses who approved
};

struct by_target_account;
struct by_created;

typedef multi_index_container<
   reward_level_proposal_object,
   indexed_by<
      ordered_unique< tag< by_id >,
         member< reward_level_proposal_object, reward_level_proposal_id_type, &reward_level_proposal_object::id >
      >,
      ordered_non_unique< tag< by_target_account >,
         member< reward_level_proposal_object, account_name_type, &reward_level_proposal_object::target_account >
      >,
      ordered_non_unique< tag< by_created >,
         member< reward_level_proposal_object, time_point_sec, &reward_level_proposal_object::created >
      >
   >
> reward_level_proposal_index;
```

#### 4. Approval Operation

Witnesses vote to approve or reject proposals.

```cpp
struct reward_level_approval_operation
{
   account_name_type          witness;
   reward_level_proposal_id_type proposal_id;
   bool                       approve;  // true = approve, false = reject

   void validate() const;
   void get_required_active_authorities( flat_set<account_name_type>& a ) const
   {
      a.insert( witness );
   }
};
```

#### 5. Reward Level Evaluators

**Proposal Evaluator** - Creates new proposals:

**Location**: `libraries/chain/steem_evaluator.cpp`

```cpp
void reward_level_proposal_evaluator::do_apply( const reward_level_proposal_operation& o )
{
   const auto& witness = _db.get_witness( o.proposing_witness );
   FC_ASSERT( witness.signing_key != public_key_type(), "Not an active witness" );

   const auto& target = _db.get_account( o.target_account );

   // Check for existing proposal for this account
   const auto& proposal_idx = _db.get_index< reward_level_proposal_index >().indices().get< by_target_account >();
   auto existing = proposal_idx.find( o.target_account );
   FC_ASSERT( existing == proposal_idx.end(), "Proposal already exists for this account" );

   _db.create< reward_level_proposal_object >( [&]( reward_level_proposal_object& p )
   {
      p.target_account = o.target_account;
      p.proposed_level = o.new_reward_level;
      p.proposing_witness = o.proposing_witness;
      p.created = _db.head_block_time();
      p.justification = o.justification;
      p.approvals.insert( o.proposing_witness );  // Auto-approve by proposer
   });
}
```

**Approval Evaluator** - Processes witness votes and applies changes when consensus reached:

```cpp
void reward_level_approval_evaluator::do_apply( const reward_level_approval_operation& o )
{
   const auto& witness = _db.get_witness( o.witness );
   FC_ASSERT( witness.signing_key != public_key_type(), "Not an active witness" );

   const auto& proposal = _db.get< reward_level_proposal_object >( o.proposal_id );

   _db.modify( proposal, [&]( reward_level_proposal_object& p )
   {
      if( o.approve )
      {
         p.approvals.insert( o.witness );
      }
      else
      {
         p.approvals.erase( o.witness );
      }
   });

   // Check if consensus reached (majority of active witnesses)
   const auto& witness_idx = _db.get_index< witness_index >().indices().get< by_vote_name >();
   uint32_t total_active_witnesses = 0;
   for( auto itr = witness_idx.begin(); itr != witness_idx.end() && total_active_witnesses < STEEM_MAX_WITNESSES; ++itr )
   {
      if( itr->signing_key != public_key_type() )
         total_active_witnesses++;
   }

   const uint32_t required_approvals = (total_active_witnesses * 2 / 3) + 1;  // 2/3 majority

   if( proposal.approvals.size() >= required_approvals )
   {
      // Apply the reward level change
      const auto& account = _db.get_account( proposal.target_account );
      _db.modify( account, [&]( account_object& a )
      {
         a.reward_level = proposal.proposed_level;
      });

      // Remove the proposal
      _db.remove( proposal );

      ilog( "Reward level for ${account} changed to ${level} by witness consensus",
            ("account", proposal.target_account)("level", proposal.proposed_level) );
   }
}
```

### Reward Multiplier Management

#### 6. Reward Multiplier Object

Store current multiplier values in a global property object managed by witness consensus.

**Location**: `libraries/chain/include/steem/chain/global_property_object.hpp`

```cpp
// Add to global_property_object or create separate reward_multiplier_object
struct reward_multiplier_object : public object< reward_multiplier_object_type, reward_multiplier_object >
{
   id_type id;
   std::array<uint16_t, 4> multipliers = {
      10000,  // Level 0: 100% (1.00x) - default
      12500,  // Level 1: 125% (1.25x) - default
      15000,  // Level 2: 150% (1.50x) - default
      20000   // Level 3: 200% (2.00x) - default
   };
};
```

#### 7. Multiplier Proposal Operation

Witnesses propose changes to reward multipliers.

```cpp
struct reward_multiplier_proposal_operation
{
   account_name_type proposing_witness;
   std::array<uint16_t, 4> new_multipliers;  // New multipliers for levels 0-3
   string            justification;

   void validate() const;
   void get_required_active_authorities( flat_set<account_name_type>& a ) const
   {
      a.insert( proposing_witness );
   }
};
```

**Validation**:
```cpp
void reward_multiplier_proposal_operation::validate() const
{
   FC_ASSERT( is_valid_account_name( proposing_witness ), "Invalid witness account name" );
   FC_ASSERT( justification.size() > 0 && justification.size() <= 1000,
              "Justification required (max 1000 characters)" );

   // Ensure multipliers are within reasonable bounds
   for( size_t i = 0; i < 4; i++ )
   {
      FC_ASSERT( new_multipliers[i] >= 1000 && new_multipliers[i] <= 50000,
                 "Multipliers must be between 10% (1000) and 500% (50000)" );
   }

   // Ensure monotonic increase (higher levels = higher or equal multipliers)
   for( size_t i = 1; i < 4; i++ )
   {
      FC_ASSERT( new_multipliers[i] >= new_multipliers[i-1],
                 "Higher levels must have equal or greater multipliers" );
   }
}
```

#### 8. Multiplier Proposal Object

Track pending multiplier proposals.

```cpp
class reward_multiplier_proposal_object : public object< reward_multiplier_proposal_object_type, reward_multiplier_proposal_object >
{
   public:
      template< typename Constructor, typename Allocator >
      reward_multiplier_proposal_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      id_type                    id;
      std::array<uint16_t, 4>    proposed_multipliers;
      account_name_type          proposing_witness;
      time_point_sec             created;
      string                     justification;
      flat_set<account_name_type> approvals;
};

typedef multi_index_container<
   reward_multiplier_proposal_object,
   indexed_by<
      ordered_unique< tag< by_id >,
         member< reward_multiplier_proposal_object, reward_multiplier_proposal_id_type, &reward_multiplier_proposal_object::id >
      >,
      ordered_non_unique< tag< by_created >,
         member< reward_multiplier_proposal_object, time_point_sec, &reward_multiplier_proposal_object::created >
      >
   >
> reward_multiplier_proposal_index;
```

#### 9. Multiplier Approval Operation

```cpp
struct reward_multiplier_approval_operation
{
   account_name_type              witness;
   reward_multiplier_proposal_id_type proposal_id;
   bool                           approve;

   void validate() const;
   void get_required_active_authorities( flat_set<account_name_type>& a ) const
   {
      a.insert( witness );
   }
};
```

#### 10. Multiplier Evaluators

**Multiplier Proposal Evaluator**:

```cpp
void reward_multiplier_proposal_evaluator::do_apply( const reward_multiplier_proposal_operation& o )
{
   const auto& witness = _db.get_witness( o.proposing_witness );
   FC_ASSERT( witness.signing_key != public_key_type(), "Not an active witness" );

   // Only one active multiplier proposal allowed at a time
   const auto& proposal_idx = _db.get_index< reward_multiplier_proposal_index >();
   FC_ASSERT( proposal_idx.size() == 0, "Multiplier proposal already exists" );

   _db.create< reward_multiplier_proposal_object >( [&]( reward_multiplier_proposal_object& p )
   {
      p.proposed_multipliers = o.new_multipliers;
      p.proposing_witness = o.proposing_witness;
      p.created = _db.head_block_time();
      p.justification = o.justification;
      p.approvals.insert( o.proposing_witness );
   });
}
```

**Multiplier Approval Evaluator**:

```cpp
void reward_multiplier_approval_evaluator::do_apply( const reward_multiplier_approval_operation& o )
{
   const auto& witness = _db.get_witness( o.witness );
   FC_ASSERT( witness.signing_key != public_key_type(), "Not an active witness" );

   const auto& proposal = _db.get< reward_multiplier_proposal_object >( o.proposal_id );

   _db.modify( proposal, [&]( reward_multiplier_proposal_object& p )
   {
      if( o.approve )
      {
         p.approvals.insert( o.witness );
      }
      else
      {
         p.approvals.erase( o.witness );
      }
   });

   // Check consensus
   const auto& witness_idx = _db.get_index< witness_index >().indices().get< by_vote_name >();
   uint32_t total_active_witnesses = 0;
   for( auto itr = witness_idx.begin(); itr != witness_idx.end() && total_active_witnesses < STEEM_MAX_WITNESSES; ++itr )
   {
      if( itr->signing_key != public_key_type() )
         total_active_witnesses++;
   }

   const uint32_t required_approvals = (total_active_witnesses * 2 / 3) + 1;

   if( proposal.approvals.size() >= required_approvals )
   {
      // Apply multiplier changes
      const auto& multiplier_obj = _db.get< reward_multiplier_object >();
      _db.modify( multiplier_obj, [&]( reward_multiplier_object& m )
      {
         m.multipliers = proposal.proposed_multipliers;
      });

      _db.remove( proposal );

      ilog( "Reward multipliers updated by witness consensus: [${l0}, ${l1}, ${l2}, ${l3}]",
            ("l0", proposal.proposed_multipliers[0])
            ("l1", proposal.proposed_multipliers[1])
            ("l2", proposal.proposed_multipliers[2])
            ("l3", proposal.proposed_multipliers[3]) );
   }
}
```

### Reward Calculation

#### Accessing Multipliers

Retrieve multipliers from the database instead of using static constants:

```cpp
inline uint16_t get_reward_multiplier( const database& db, uint8_t level )
{
   FC_ASSERT( level <= 3, "Invalid reward level" );
   const auto& multiplier_obj = db.get< reward_multiplier_object >();
   return multiplier_obj.multipliers[level];
}
```

#### Integration with Existing Reward Logic

The reward level multiplier is applied during reward calculation:

```cpp
// Example: Author rewards
asset calculate_author_reward( const database& db, const account_object& author, asset base_reward )
{
   uint16_t multiplier = get_reward_multiplier( db, author.reward_level );
   return asset( (base_reward.amount.value * multiplier) / 10000, base_reward.symbol );
}

// Example: Curation rewards
asset calculate_curation_reward( const database& db, const account_object& curator, asset base_reward )
{
   uint16_t multiplier = get_reward_multiplier( db, curator.reward_level );
   return asset( (base_reward.amount.value * multiplier) / 10000, base_reward.symbol );
}
```

## Implementation Guide

### Step 1: Define the Operations

1. Add all four operations to `steem_operations.hpp`:
   - `reward_level_proposal_operation`
   - `reward_level_approval_operation`
   - `reward_multiplier_proposal_operation`
   - `reward_multiplier_approval_operation`

2. Add all to the `operation` typedef variant

3. Implement `FC_REFLECT` macros for serialization:

```cpp
FC_REFLECT( steem::protocol::reward_level_proposal_operation,
            (proposing_witness)
            (target_account)
            (new_reward_level)
            (justification) )

FC_REFLECT( steem::protocol::reward_level_approval_operation,
            (witness)
            (proposal_id)
            (approve) )

FC_REFLECT( steem::protocol::reward_multiplier_proposal_operation,
            (proposing_witness)
            (new_multipliers)
            (justification) )

FC_REFLECT( steem::protocol::reward_multiplier_approval_operation,
            (witness)
            (proposal_id)
            (approve) )
```

### Step 2: Extend Account Object

1. Add `reward_level` field to `account_object`
2. Update account object index definitions if needed
3. Add index by reward level if querying by level is required:

```cpp
struct by_reward_level;

typedef multi_index_container<
   account_object,
   indexed_by<
      // ... existing indexes ...
      ordered_non_unique< tag< by_reward_level >,
         composite_key< account_object,
            member< account_object, uint8_t, &account_object::reward_level >,
            member< account_object, account_id_type, &account_object::id >
         >
      >
   >
> account_index;
```

### Step 3: Create Database Objects

1. Create `reward_level_objects.hpp` with:
   - `reward_level_proposal_object` and its index
   - `reward_multiplier_proposal_object` and its index

2. Create or extend `reward_multiplier_object` in global properties

3. Add object indexes to database

4. Register object types in object ID enumeration

### Step 4: Implement Evaluators

1. Create evaluator classes:
   - `reward_level_proposal_evaluator`
   - `reward_level_approval_evaluator`
   - `reward_multiplier_proposal_evaluator`
   - `reward_multiplier_approval_evaluator`

2. Register all evaluators in database initialization

3. Implement witness validation and consensus logic for both level and multiplier changes

### Step 5: Modify Reward Calculations

1. Locate reward calculation functions:
   - Author rewards (content creation)
   - Curation rewards (voting)
   - Witness rewards (block production)
   - Interest/APR calculations

2. Update all reward calculations to:
   - Retrieve multipliers from database (not static constants)
   - Pass database reference to `get_reward_multiplier()` function
   - Apply multiplier to each reward type

### Step 6: Add API Support

Add API methods to query reward level information, multipliers, and proposals:

```cpp
// In database_api or appropriate plugin

// Account reward level
struct get_reward_level_args
{
   account_name_type account;
};

struct get_reward_level_return
{
   uint8_t reward_level;
   uint16_t reward_multiplier;
};

// Current multipliers
struct get_reward_multipliers_return
{
   std::array<uint16_t, 4> multipliers;
};

// Level proposals
struct get_reward_level_proposals_args
{
   account_name_type account;  // Optional filter by account
};

struct get_reward_level_proposals_return
{
   vector<reward_level_proposal_api_object> proposals;
};

// Multiplier proposals
struct get_reward_multiplier_proposals_return
{
   vector<reward_multiplier_proposal_api_object> proposals;
};

get_reward_level_return get_reward_level( get_reward_level_args args );
get_reward_multipliers_return get_reward_multipliers();
get_reward_level_proposals_return get_reward_level_proposals( get_reward_level_proposals_args args );
get_reward_multiplier_proposals_return get_reward_multiplier_proposals();
```

### Step 7: Write Tests

**Location**: `tests/tests/operation_tests.cpp`

```cpp
BOOST_AUTO_TEST_SUITE(reward_level_tests)

BOOST_AUTO_TEST_CASE( reward_level_proposal_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: reward_level_proposal_operation validation" );

      reward_level_proposal_operation op;
      op.proposing_witness = "witness1";
      op.target_account = "alice";
      op.justification = "Good contributor";

      // Valid levels
      for( uint8_t level = 0; level <= 3; level++ )
      {
         op.new_reward_level = level;
         REQUIRE_OP_VALIDATION_SUCCESS( op );
      }

      // Invalid level
      op.new_reward_level = 4;
      REQUIRE_OP_VALIDATION_FAILURE( op );

      // Empty justification
      op.new_reward_level = 1;
      op.justification = "";
      REQUIRE_OP_VALIDATION_FAILURE( op );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( reward_level_proposal_create )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: reward level proposal creation" );

      ACTORS( (alice)(witness1) )
      fund( "witness1", ASSET( "1.000 TESTS" ) );

      // Make witness1 an active witness
      witness_create( "witness1", witness1_private_key, "foo.bar", witness1_private_key.get_public_key(), 0 );

      const auto& alice_account = db->get_account( "alice" );
      BOOST_REQUIRE_EQUAL( alice_account.reward_level, 0 ); // Default level

      reward_level_proposal_operation op;
      op.proposing_witness = "witness1";
      op.target_account = "alice";
      op.new_reward_level = 2;
      op.justification = "Outstanding community contribution";

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      push_transaction( tx, witness1_private_key );

      // Verify proposal created
      const auto& proposal_idx = db->get_index< reward_level_proposal_index >().indices().get< by_target_account >();
      auto proposal = proposal_idx.find( "alice" );
      BOOST_REQUIRE( proposal != proposal_idx.end() );
      BOOST_REQUIRE_EQUAL( proposal->proposed_level, 2 );
      BOOST_REQUIRE_EQUAL( proposal->approvals.size(), 1 );  // Auto-approved by proposer
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( reward_level_consensus )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: reward level change via witness consensus" );

      ACTORS( (alice)(witness1)(witness2)(witness3)(witness4) )

      // Create 4 active witnesses
      for( auto witness : { "witness1", "witness2", "witness3", "witness4" } )
      {
         fund( witness, ASSET( "1.000 TESTS" ) );
         witness_create( witness, generate_private_key( witness ), "foo.bar",
                        generate_private_key( witness ).get_public_key(), 0 );
      }

      const auto& alice_account = db->get_account( "alice" );
      BOOST_REQUIRE_EQUAL( alice_account.reward_level, 0 );

      // Witness1 proposes level change
      reward_level_proposal_operation prop_op;
      prop_op.proposing_witness = "witness1";
      prop_op.target_account = "alice";
      prop_op.new_reward_level = 3;
      prop_op.justification = "Exceptional contributor";

      signed_transaction tx;
      tx.operations.push_back( prop_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      push_transaction( tx, witness1_private_key );

      // Get proposal ID
      const auto& proposal_idx = db->get_index< reward_level_proposal_index >().indices().get< by_target_account >();
      auto proposal = proposal_idx.find( "alice" );
      BOOST_REQUIRE( proposal != proposal_idx.end() );
      auto proposal_id = proposal->id;

      // Level should not change yet (only 1/4 approvals)
      BOOST_REQUIRE_EQUAL( alice_account.reward_level, 0 );

      // Witness2 and witness3 approve (total 3/4 = 75%, reaches 2/3 threshold)
      for( auto witness : { "witness2", "witness3" } )
      {
         reward_level_approval_operation approve_op;
         approve_op.witness = witness;
         approve_op.proposal_id = proposal_id;
         approve_op.approve = true;

         tx.operations.clear();
         tx.operations.push_back( approve_op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         push_transaction( tx, generate_private_key( witness ) );
      }

      // Level should now be changed (3/4 = 75% >= 66.7% required)
      BOOST_REQUIRE_EQUAL( alice_account.reward_level, 3 );

      // Proposal should be removed
      proposal = proposal_idx.find( "alice" );
      BOOST_REQUIRE( proposal == proposal_idx.end() );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( reward_multiplier_proposal_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: reward_multiplier_proposal_operation validation" );

      reward_multiplier_proposal_operation op;
      op.proposing_witness = "witness1";
      op.justification = "Economic adjustment";

      // Valid multipliers
      op.new_multipliers = { 10000, 15000, 20000, 30000 };
      REQUIRE_OP_VALIDATION_SUCCESS( op );

      // Multiplier too low
      op.new_multipliers = { 500, 15000, 20000, 30000 };
      REQUIRE_OP_VALIDATION_FAILURE( op );

      // Multiplier too high
      op.new_multipliers = { 10000, 15000, 20000, 60000 };
      REQUIRE_OP_VALIDATION_FAILURE( op );

      // Non-monotonic (level 2 < level 1)
      op.new_multipliers = { 10000, 20000, 15000, 30000 };
      REQUIRE_OP_VALIDATION_FAILURE( op );

      // Empty justification
      op.new_multipliers = { 10000, 15000, 20000, 30000 };
      op.justification = "";
      REQUIRE_OP_VALIDATION_FAILURE( op );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( reward_multiplier_consensus )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: multiplier change via witness consensus" );

      ACTORS( (witness1)(witness2)(witness3)(witness4) )

      // Create 4 active witnesses
      for( auto witness : { "witness1", "witness2", "witness3", "witness4" } )
      {
         fund( witness, ASSET( "1.000 TESTS" ) );
         witness_create( witness, generate_private_key( witness ), "foo.bar",
                        generate_private_key( witness ).get_public_key(), 0 );
      }

      // Get initial multipliers
      const auto& multiplier_obj = db->get< reward_multiplier_object >();
      auto initial_multipliers = multiplier_obj.multipliers;

      // Witness1 proposes multiplier change
      reward_multiplier_proposal_operation prop_op;
      prop_op.proposing_witness = "witness1";
      prop_op.new_multipliers = { 10000, 13000, 16000, 25000 }; // New values
      prop_op.justification = "Adjusting rewards for better distribution";

      signed_transaction tx;
      tx.operations.push_back( prop_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      push_transaction( tx, witness1_private_key );

      // Get proposal ID
      const auto& proposal_idx = db->get_index< reward_multiplier_proposal_index >();
      BOOST_REQUIRE( proposal_idx.size() == 1 );
      auto proposal = proposal_idx.begin();
      auto proposal_id = proposal->id;

      // Multipliers should not change yet
      BOOST_REQUIRE( multiplier_obj.multipliers == initial_multipliers );

      // Witness2 and witness3 approve
      for( auto witness : { "witness2", "witness3" } )
      {
         reward_multiplier_approval_operation approve_op;
         approve_op.witness = witness;
         approve_op.proposal_id = proposal_id;
         approve_op.approve = true;

         tx.operations.clear();
         tx.operations.push_back( approve_op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         push_transaction( tx, generate_private_key( witness ) );
      }

      // Multipliers should now be changed
      BOOST_REQUIRE_EQUAL( multiplier_obj.multipliers[0], 10000 );
      BOOST_REQUIRE_EQUAL( multiplier_obj.multipliers[1], 13000 );
      BOOST_REQUIRE_EQUAL( multiplier_obj.multipliers[2], 16000 );
      BOOST_REQUIRE_EQUAL( multiplier_obj.multipliers[3], 25000 );

      // Proposal should be removed
      BOOST_REQUIRE( proposal_idx.size() == 0 );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( reward_level_multiplier_application )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: reward level multiplier application" );

      ACTORS( (alice)(bob) )
      fund( "alice", ASSET( "10.000 TESTS" ) );
      fund( "bob", ASSET( "10.000 TESTS" ) );
      op.reward_level = 0; // 100% rewards
      push_operation( op, alice_private_key );

      op.account = "bob";
      op.reward_level = 3; // 200% rewards
      push_operation( op, bob_private_key );

      // Create similar content and compare rewards
      // ... test implementation ...

      // Verify bob receives approximately 2x rewards compared to alice
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
```

## Authorization and Security

### Witness Consensus Requirements

Reward level changes require witness consensus to ensure:
- **Decentralized governance**: No single party can arbitrarily adjust rewards
- **Transparency**: All proposals and approvals are on-chain and auditable
- **Accountability**: Justifications are required and permanently recorded

### Consensus Threshold

- **Proposal**: Any active witness can propose a reward level change
- **Approval**: Requires 2/3 majority of active witnesses (top 21 witnesses)
- **Auto-approval**: Proposing witness automatically approves their own proposal
- **Revocation**: Witnesses can change their approval vote before consensus is reached

### Authority Requirements

- **Proposing**: Requires active authority of a witness account with valid signing key
- **Approving**: Requires active authority of a witness account with valid signing key
- **Target account**: No permission required from the account being affected

### Security Considerations

```cpp
// Witness validation in evaluator
const auto& witness = _db.get_witness( o.proposing_witness );
FC_ASSERT( witness.signing_key != public_key_type(), "Not an active witness" );

// Prevent duplicate proposals
const auto& proposal_idx = _db.get_index< reward_level_proposal_index >().indices().get< by_target_account >();
auto existing = proposal_idx.find( o.target_account );
FC_ASSERT( existing == proposal_idx.end(), "Proposal already exists for this account" );

// Calculate dynamic consensus threshold based on active witnesses
const uint32_t required_approvals = (total_active_witnesses * 2 / 3) + 1;
```

### Economic Considerations

1. **Inflation Impact**: Higher reward levels increase total reward pool distribution
2. **Level Distribution**: Monitor percentage of accounts at each level
3. **Gaming Prevention**: Implement criteria for level eligibility to prevent abuse
4. **Gradual Transitions**: Consider implementing cooldown periods between level changes

## API Integration

### Database API

```cpp
// Get single account reward level
DEFINE_API( database_api, get_reward_level )
{
   auto account = _db.find_account( args.account );
   FC_ASSERT( account != nullptr, "Account not found" );

   get_reward_level_return result;
   result.reward_level = account->reward_level;
   result.reward_multiplier = get_reward_multiplier( _db, account->reward_level );

   return result;
}

// Get current reward multipliers
DEFINE_API( database_api, get_reward_multipliers )
{
   const auto& multiplier_obj = _db.get< reward_multiplier_object >();

   get_reward_multipliers_return result;
   result.multipliers = multiplier_obj.multipliers;

   return result;
}

// Get reward level distribution statistics
struct get_reward_level_stats_return
{
   std::array<uint64_t, 4> account_counts; // Accounts per level
   std::array<share_type, 4> total_vests;  // Total VESTS per level
};

DEFINE_API( database_api, get_reward_level_stats )
{
   get_reward_level_stats_return result;
   // ... implementation ...
   return result;
}

// Get pending level proposals
DEFINE_API( database_api, get_reward_level_proposals )
{
   get_reward_level_proposals_return result;

   const auto& proposal_idx = _db.get_index< reward_level_proposal_index >().indices().get< by_created >();

   if( args.account.size() > 0 )
   {
      // Filter by specific account
      const auto& by_account_idx = _db.get_index< reward_level_proposal_index >().indices().get< by_target_account >();
      auto itr = by_account_idx.find( args.account );
      if( itr != by_account_idx.end() )
      {
         result.proposals.push_back( *itr );
      }
   }
   else
   {
      // Return all proposals
      for( auto itr = proposal_idx.begin(); itr != proposal_idx.end(); ++itr )
      {
         result.proposals.push_back( *itr );
      }
   }

   return result;
}

// Get pending multiplier proposals
DEFINE_API( database_api, get_reward_multiplier_proposals )
{
   get_reward_multiplier_proposals_return result;

   const auto& proposal_idx = _db.get_index< reward_multiplier_proposal_index >().indices().get< by_created >();

   for( auto itr = proposal_idx.begin(); itr != proposal_idx.end(); ++itr )
   {
      result.proposals.push_back( *itr );
   }

   return result;
}
```

### Condenser API

Add condenser API compatibility methods for existing applications:

```cpp
fc::variant get_account_reward_level( std::string account_name );
fc::variant get_reward_multipliers();
fc::variant get_reward_level_proposals( optional<std::string> account_name );
fc::variant get_reward_multiplier_proposals();
fc::variant get_reward_level_proposal_votes( uint64_t proposal_id );
fc::variant get_reward_multiplier_proposal_votes( uint64_t proposal_id );
```

## Configuration

Add configuration options to `config.ini`:

```ini
# Enable reward level proposal and approval logging
log-reward-level-changes = true

# Set default reward level for new accounts (0-3)
default-reward-level = 0

# Maximum age for proposals before auto-expiration (days)
reward-level-proposal-expiration = 30

# Minimum witness rank to propose changes (1-21, 0 = any active witness)
reward-level-min-witness-rank = 0
```

## Monitoring and Metrics

Implement metrics tracking:

- Distribution of accounts across levels
- Total rewards distributed per level
- Number of pending proposals
- Proposal approval rates and timing
- Witness participation in proposal voting
- Economic impact on inflation rate
- Level transition frequency and patterns

## Workflow Example

### Typical Reward Level Change Process

1. **Proposal Creation**
   - Witness identifies an account deserving level adjustment
   - Witness creates `reward_level_proposal_operation` with justification
   - Proposal is broadcast and included in a block
   - Proposing witness automatically approves (1 vote)

2. **Community Review**
   - Other witnesses review the proposal and justification
   - Community can discuss via social layer (not on-chain)
   - Witnesses make independent decisions

3. **Witness Voting**
   - Witnesses submit `reward_level_approval_operation` to approve/reject
   - Votes can be changed before consensus is reached
   - Progress visible via API queries

4. **Consensus and Execution**
   - When 2/3 majority reached, change is applied immediately
   - Account's reward level updated
   - Proposal automatically removed
   - Event logged for monitoring

5. **Effect**
   - Account's future rewards calculated using new multiplier
   - Historical rewards remain unchanged
   - Change is permanent until another proposal passes

### CLI Wallet Commands

```bash
# Propose a reward level change (witness only)
propose_reward_level witness1 alice 2 "Outstanding community contributor with 500+ quality posts"

# Approve a level proposal
approve_reward_level_proposal witness2 12345 true

# Propose multiplier change (witness only)
propose_reward_multipliers witness1 "10000,13000,16000,25000" "Economic adjustment for better distribution"

# Approve a multiplier proposal
approve_reward_multiplier_proposal witness2 67890 true

# Check current multipliers
get_reward_multipliers

# Check pending level proposals
list_reward_level_proposals

# Check pending multiplier proposals
list_reward_multiplier_proposals

# Check specific account's proposal
get_reward_level_proposal alice

# Query account's current reward level
get_account_reward_level alice
```

## Future Enhancements

1. **Proposal Expiration**: Auto-expire proposals after configurable period (e.g., 30 days)
2. **Proposal Amendments**: Allow proposer to update justification before consensus
3. **Batch Proposals**: Single proposal affecting multiple accounts
4. **Level Criteria Framework**: On-chain criteria for automatic level eligibility
5. **Temporary Boosts**: Time-limited reward level increases for events/promotions
6. **Level Decay**: Gradual level reduction for inactive accounts
7. **Category-Specific Levels**: Different levels for different reward types (author vs. curator)
8. **Category-Specific Multipliers**: Separate multipliers for author rewards, curation rewards, and witness rewards
9. **Proposal Comments**: Allow witnesses to attach on-chain comments to votes
10. **Historical Tracking**: API to query historical level and multiplier changes with their impacts
11. **Multiplier Range Limits**: Configurable min/max bounds for multiplier values per witness vote
12. **Gradual Multiplier Transitions**: Phase in multiplier changes over time instead of instant updates

## References

- [Creating New Operations](../create-operation.md)
- [Plugin Development Guide](../plugin.md)
- [Testing Guide](../testing.md)
- [Hardfork Procedures](../../operations/hardfork-procedure.md)
