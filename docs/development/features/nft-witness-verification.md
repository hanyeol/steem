# NFT-Based Witness Verification Feature

## Overview

This document describes the implementation of a feature that requires witnesses to prove ownership of ERC-721 NFTs on Ethereum-compatible chains before they can confirm blocks. This adds an additional layer of authorization and enables NFT-based governance for block production.

## Feature Description

Witnesses must demonstrate ownership of specific ERC-721 NFTs to be eligible for block confirmation. Key features:

1. **Multiple NFT Collection Support** - Support multiple ERC-721 contract addresses
2. **Cross-Chain Verification** - Verify NFT ownership on Ethereum-compatible chains (Ethereum, Polygon, BSC, etc.)
3. **Ownership Proof** - Witnesses submit cryptographic proofs of NFT ownership
4. **Dynamic Updates** - NFT requirements can be updated via witness consensus
5. **Backward Compatibility** - Hardfork-gated to maintain compatibility with existing witnesses

## Architecture

### High-Level Flow

```
┌─────────────────┐
│   Witness Node  │
│                 │
│  1. Generate    │
│     ownership   │
│     proof       │
└────────┬────────┘
         │
         ▼
┌─────────────────┐      ┌──────────────────┐
│  Steem Chain    │◄────►│  Oracle Service  │
│                 │      │                  │
│  2. Verify      │      │  3. Query NFT    │
│     signature   │      │     ownership    │
│                 │      │     on EVM chain │
│  4. Validate    │      └──────────────────┘
│     witness     │
│     schedule    │
└─────────────────┘
```

### Components

1. **NFT Verification Oracle** - Off-chain service that queries EVM chains
2. **Witness Verification Plugin** - Plugin that validates NFT ownership proofs
3. **NFT Registry** - On-chain registry of approved NFT contracts
4. **Proof Mechanism** - Cryptographic proof system linking Steem keys to Ethereum addresses

## Implementation Design

### 1. Protocol Changes

#### New Operations

Add new operations in [libraries/protocol/include/steem/protocol/steem_operations.hpp](../../../libraries/protocol/include/steem/protocol/steem_operations.hpp):

```cpp
/**
 * Witness submits proof of NFT ownership
 * Must be submitted periodically (e.g., every 24 hours)
 */
struct witness_nft_proof_operation : public base_operation
{
   account_name_type witness_account;

   // Ethereum-compatible address that owns the NFT
   string eth_address;

   // NFT contract address
   string contract_address;

   // Token ID owned by the address
   string token_id;

   // Chain ID (1 = Ethereum, 137 = Polygon, 56 = BSC, etc.)
   uint32_t chain_id;

   // Signature proving control of eth_address using Steem private key
   // Sign message: "steem:<witness_account>:nft:<contract_address>:<token_id>:<timestamp>"
   string eth_signature;

   // Timestamp of the proof (must be recent)
   fc::time_point_sec timestamp;

   // Signature of the operation with witness signing key
   extensions_type extensions;

   void validate() const;
   void get_required_active_authorities( flat_set<account_name_type>& a ) const
   {
      a.insert( witness_account );
   }
};

/**
 * Register a new NFT collection that can be used for witness verification
 * Requires majority witness approval
 */
struct nft_collection_register_operation : public base_operation
{
   account_name_type proposer;

   // Human-readable name for the collection
   string collection_name;

   // NFT contract address on the target chain
   string contract_address;

   // Chain ID (1 = Ethereum, 137 = Polygon, 56 = BSC, etc.)
   uint32_t chain_id;

   // Minimum number of NFTs required from this collection
   uint32_t min_nft_count = 1;

   // Whether this collection is currently active
   bool active = true;

   extensions_type extensions;

   void validate() const;
   void get_required_active_authorities( flat_set<account_name_type>& a ) const
   {
      a.insert( proposer );
   }
};

/**
 * Approve or reject an NFT collection registration
 * Must be signed by active witness
 */
struct nft_collection_approve_operation : public base_operation
{
   account_name_type witness_account;

   // ID of the collection registration to approve/reject
   uint64_t collection_id;

   // true = approve, false = reject
   bool approve;

   extensions_type extensions;

   void validate() const;
   void get_required_active_authorities( flat_set<account_name_type>& a ) const
   {
      a.insert( witness_account );
   }
};
```

Add `FC_REFLECT` for serialization:

```cpp
FC_REFLECT( steem::protocol::witness_nft_proof_operation,
            (witness_account)
            (eth_address)
            (contract_address)
            (token_id)
            (chain_id)
            (eth_signature)
            (timestamp)
            (extensions)
          )

FC_REFLECT( steem::protocol::nft_collection_register_operation,
            (proposer)
            (collection_name)
            (contract_address)
            (chain_id)
            (min_nft_count)
            (active)
            (extensions)
          )

FC_REFLECT( steem::protocol::nft_collection_approve_operation,
            (witness_account)
            (collection_id)
            (approve)
            (extensions)
          )
```

#### Validation Implementation

```cpp
void witness_nft_proof_operation::validate() const
{
   FC_ASSERT( is_valid_account_name( witness_account ),
              "Invalid witness account name" );

   // Validate Ethereum address format (0x + 40 hex chars)
   FC_ASSERT( eth_address.size() == 42,
              "Invalid Ethereum address length" );
   FC_ASSERT( eth_address.substr(0, 2) == "0x",
              "Ethereum address must start with 0x" );

   // Validate contract address format
   FC_ASSERT( contract_address.size() == 42,
              "Invalid contract address length" );
   FC_ASSERT( contract_address.substr(0, 2) == "0x",
              "Contract address must start with 0x" );

   // Validate token ID is not empty
   FC_ASSERT( !token_id.empty(),
              "Token ID cannot be empty" );

   // Validate signature is not empty
   FC_ASSERT( !eth_signature.empty(),
              "Ethereum signature cannot be empty" );

   // Validate timestamp is not too old or in the future
   auto now = fc::time_point::now();
   FC_ASSERT( timestamp <= now,
              "Timestamp cannot be in the future" );
   FC_ASSERT( now - timestamp <= fc::days(1),
              "Proof timestamp too old (must be within 24 hours)" );
}

void nft_collection_register_operation::validate() const
{
   FC_ASSERT( is_valid_account_name( proposer ),
              "Invalid proposer account name" );

   FC_ASSERT( !collection_name.empty(),
              "Collection name cannot be empty" );
   FC_ASSERT( collection_name.size() <= 100,
              "Collection name too long (max 100 chars)" );

   FC_ASSERT( contract_address.size() == 42,
              "Invalid contract address" );
   FC_ASSERT( contract_address.substr(0, 2) == "0x",
              "Contract address must start with 0x" );

   FC_ASSERT( min_nft_count > 0 && min_nft_count <= 100,
              "Minimum NFT count must be between 1 and 100" );
}

void nft_collection_approve_operation::validate() const
{
   FC_ASSERT( is_valid_account_name( witness_account ),
              "Invalid witness account name" );
}
```

### 2. Chain State Changes

#### Database Objects

Add new objects in [libraries/chain/include/steem/chain/](../../../libraries/chain/include/steem/chain/):

Create `nft_verification_objects.hpp`:

```cpp
#pragma once

#include <steem/chain/steem_object_types.hpp>
#include <steem/protocol/types.hpp>

namespace steem { namespace chain {

using namespace steem::protocol;

/**
 * Tracks registered NFT collections that can be used for witness verification
 */
class nft_collection_object : public object< nft_collection_object_type, nft_collection_object >
{
   public:
      template< typename Constructor, typename Allocator >
      nft_collection_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      id_type           id;

      string            collection_name;
      string            contract_address;
      uint32_t          chain_id;
      uint32_t          min_nft_count;
      bool              active;

      // Witness approval tracking
      flat_set<account_name_type> approved_by;
      flat_set<account_name_type> rejected_by;

      fc::time_point_sec created;
      fc::time_point_sec last_updated;
};

/**
 * Tracks NFT ownership proofs submitted by witnesses
 */
class witness_nft_proof_object : public object< witness_nft_proof_object_type, witness_nft_proof_object >
{
   public:
      template< typename Constructor, typename Allocator >
      witness_nft_proof_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      id_type           id;

      account_name_type witness_account;
      string            eth_address;
      string            contract_address;
      string            token_id;
      uint32_t          chain_id;

      fc::time_point_sec proof_timestamp;
      fc::time_point_sec verification_timestamp;

      // Verification status
      enum verification_status_type {
         pending,        // Waiting for oracle verification
         verified,       // Oracle confirmed ownership
         failed,         // Oracle verification failed
         expired         // Proof too old
      };

      uint8_t           verification_status;
      string            verification_details; // Error message or oracle transaction hash
};

struct by_collection_name;
struct by_contract_address;
struct by_witness;
struct by_token_identity;
struct by_verification_status;

typedef multi_index_container<
   nft_collection_object,
   indexed_by<
      ordered_unique< tag< by_id >,
         member< nft_collection_object, nft_collection_object::id_type, &nft_collection_object::id >
      >,
      ordered_unique< tag< by_collection_name >,
         member< nft_collection_object, string, &nft_collection_object::collection_name >
      >,
      ordered_non_unique< tag< by_contract_address >,
         composite_key< nft_collection_object,
            member< nft_collection_object, string, &nft_collection_object::contract_address >,
            member< nft_collection_object, uint32_t, &nft_collection_object::chain_id >
         >
      >
   >,
   allocator< nft_collection_object >
> nft_collection_index;

typedef multi_index_container<
   witness_nft_proof_object,
   indexed_by<
      ordered_unique< tag< by_id >,
         member< witness_nft_proof_object, witness_nft_proof_object::id_type, &witness_nft_proof_object::id >
      >,
      ordered_non_unique< tag< by_witness >,
         member< witness_nft_proof_object, account_name_type, &witness_nft_proof_object::witness_account >
      >,
      ordered_unique< tag< by_token_identity >,
         composite_key< witness_nft_proof_object,
            member< witness_nft_proof_object, string, &witness_nft_proof_object::contract_address >,
            member< witness_nft_proof_object, string, &witness_nft_proof_object::token_id >,
            member< witness_nft_proof_object, uint32_t, &witness_nft_proof_object::chain_id >
         >
      >,
      ordered_non_unique< tag< by_verification_status >,
         composite_key< witness_nft_proof_object,
            member< witness_nft_proof_object, uint8_t, &witness_nft_proof_object::verification_status >,
            member< witness_nft_proof_object, fc::time_point_sec, &witness_nft_proof_object::proof_timestamp >
         >
      >
   >,
   allocator< witness_nft_proof_object >
> witness_nft_proof_index;

} } // steem::chain

FC_REFLECT( steem::chain::nft_collection_object,
            (id)
            (collection_name)
            (contract_address)
            (chain_id)
            (min_nft_count)
            (active)
            (approved_by)
            (rejected_by)
            (created)
            (last_updated)
          )

FC_REFLECT_ENUM( steem::chain::witness_nft_proof_object::verification_status_type,
                 (pending)
                 (verified)
                 (failed)
                 (expired)
               )

FC_REFLECT( steem::chain::witness_nft_proof_object,
            (id)
            (witness_account)
            (eth_address)
            (contract_address)
            (token_id)
            (chain_id)
            (proof_timestamp)
            (verification_timestamp)
            (verification_status)
            (verification_details)
          )

CHAINBASE_SET_INDEX_TYPE( steem::chain::nft_collection_object, steem::chain::nft_collection_index )
CHAINBASE_SET_INDEX_TYPE( steem::chain::witness_nft_proof_object, steem::chain::witness_nft_proof_index )
```

#### Update Object Type Enumeration

Add to [libraries/chain/include/steem/chain/steem_object_types.hpp](../../../libraries/chain/include/steem/chain/steem_object_types.hpp):

```cpp
enum object_type
{
   // ... existing types ...
   nft_collection_object_type,
   witness_nft_proof_object_type
};
```

### 3. Evaluator Implementation

Create evaluators in [libraries/chain/](../../../libraries/chain/):

Create `nft_evaluator.cpp`:

```cpp
#include <steem/chain/steem_evaluator.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/nft_verification_objects.hpp>
#include <steem/protocol/steem_operations.hpp>

namespace steem { namespace chain {

void witness_nft_proof_evaluator::do_apply( const witness_nft_proof_operation& o )
{
   const auto& witness = _db.get_witness( o.witness_account );

   // Verify the witness exists and is active
   FC_ASSERT( witness.signing_key != public_key_type(),
              "Witness is not active" );

   // Check if an NFT collection is registered for this contract
   const auto& collection_idx = _db.get_index< nft_collection_index >().indices().get< by_contract_address >();
   auto collection_itr = collection_idx.find( boost::make_tuple( o.contract_address, o.chain_id ) );

   FC_ASSERT( collection_itr != collection_idx.end(),
              "NFT collection not registered: ${contract} on chain ${chain}",
              ("contract", o.contract_address)("chain", o.chain_id) );

   FC_ASSERT( collection_itr->active,
              "NFT collection is not active" );

   // Verify the Ethereum signature
   // Message format: "steem:<witness_account>:nft:<contract_address>:<token_id>:<timestamp>"
   std::string message = "steem:" + o.witness_account.operator string() +
                         ":nft:" + o.contract_address +
                         ":" + o.token_id +
                         ":" + std::to_string( o.timestamp.sec_since_epoch() );

   // Recover Ethereum address from signature
   string recovered_address = recover_eth_address( message, o.eth_signature );

   FC_ASSERT( recovered_address == o.eth_address,
              "Signature verification failed: recovered address ${recovered} does not match ${provided}",
              ("recovered", recovered_address)("provided", o.eth_address) );

   // Create or update proof object
   const auto& proof_idx = _db.get_index< witness_nft_proof_index >().indices().get< by_witness >();
   auto proof_itr = proof_idx.find( o.witness_account );

   if( proof_itr == proof_idx.end() )
   {
      _db.create< witness_nft_proof_object >( [&]( witness_nft_proof_object& proof )
      {
         proof.witness_account = o.witness_account;
         proof.eth_address = o.eth_address;
         proof.contract_address = o.contract_address;
         proof.token_id = o.token_id;
         proof.chain_id = o.chain_id;
         proof.proof_timestamp = o.timestamp;
         proof.verification_status = witness_nft_proof_object::pending;
         proof.verification_details = "Pending oracle verification";
      });
   }
   else
   {
      _db.modify( *proof_itr, [&]( witness_nft_proof_object& proof )
      {
         proof.eth_address = o.eth_address;
         proof.contract_address = o.contract_address;
         proof.token_id = o.token_id;
         proof.chain_id = o.chain_id;
         proof.proof_timestamp = o.timestamp;
         proof.verification_status = witness_nft_proof_object::pending;
         proof.verification_details = "Pending oracle verification";
      });
   }

   // Emit signal for oracle service to pick up
   _db.notify_nft_proof_submitted( o );
}

void nft_collection_register_evaluator::do_apply( const nft_collection_register_operation& o )
{
   // Verify proposer is an active witness
   const auto* witness = _db.find_witness( o.proposer );
   FC_ASSERT( witness != nullptr,
              "Proposer must be an active witness" );
   FC_ASSERT( witness->signing_key != public_key_type(),
              "Proposer witness is not active" );

   // Check if collection already exists
   const auto& collection_idx = _db.get_index< nft_collection_index >().indices().get< by_contract_address >();
   auto collection_itr = collection_idx.find( boost::make_tuple( o.contract_address, o.chain_id ) );

   FC_ASSERT( collection_itr == collection_idx.end(),
              "NFT collection already registered for this contract and chain" );

   // Create collection registration (pending approval)
   _db.create< nft_collection_object >( [&]( nft_collection_object& collection )
   {
      collection.collection_name = o.collection_name;
      collection.contract_address = o.contract_address;
      collection.chain_id = o.chain_id;
      collection.min_nft_count = o.min_nft_count;
      collection.active = false; // Starts inactive until approved
      collection.approved_by.insert( o.proposer ); // Proposer auto-approves
      collection.created = _db.head_block_time();
      collection.last_updated = _db.head_block_time();
   });
}

void nft_collection_approve_evaluator::do_apply( const nft_collection_approve_operation& o )
{
   // Verify approver is an active witness
   const auto& witness = _db.get_witness( o.witness_account );
   FC_ASSERT( witness.signing_key != public_key_type(),
              "Approver must be an active witness" );

   // Get collection
   const auto& collection = _db.get< nft_collection_object >( o.collection_id );

   // Update approval status
   _db.modify( collection, [&]( nft_collection_object& c )
   {
      if( o.approve )
      {
         c.approved_by.insert( o.witness_account );
         c.rejected_by.erase( o.witness_account );
      }
      else
      {
         c.rejected_by.insert( o.witness_account );
         c.approved_by.erase( o.witness_account );
      }

      c.last_updated = _db.head_block_time();

      // Activate if majority of top witnesses approve
      // Get top 21 witnesses
      const auto& witness_idx = _db.get_index< witness_index >().indices().get< by_vote_name >();
      size_t top_witness_count = 0;
      size_t approvals_from_top = 0;

      for( auto wit_itr = witness_idx.begin();
           wit_itr != witness_idx.end() && top_witness_count < 21;
           ++wit_itr, ++top_witness_count )
      {
         if( c.approved_by.count( wit_itr->owner ) > 0 )
            approvals_from_top++;
      }

      // Require majority (11 out of 21)
      if( approvals_from_top >= 11 )
         c.active = true;
      else
         c.active = false;
   });
}

// Helper function to recover Ethereum address from signature
string witness_nft_proof_evaluator::recover_eth_address(
   const string& message,
   const string& signature )
{
   // This is a simplified version - actual implementation would use
   // secp256k1 recovery and keccak256 hashing

   // 1. Prefix message with "\x19Ethereum Signed Message:\n" + length
   string prefixed_message = "\x19Ethereum Signed Message:\n" +
                             std::to_string( message.length() ) +
                             message;

   // 2. Hash with Keccak256
   fc::sha256 message_hash = fc::keccak256( prefixed_message );

   // 3. Recover public key from signature
   // Parse signature (r, s, v)
   // signature format: 0x + 130 hex chars (65 bytes: r(32) + s(32) + v(1))
   FC_ASSERT( signature.size() == 132, "Invalid signature length" );

   // Extract r, s, v from signature
   // ... (implementation details with fc::ecc or secp256k1 library)

   // 4. Derive Ethereum address from public key
   // Take last 20 bytes of keccak256(public_key)
   // ... (implementation details)

   // Placeholder return - actual implementation needed
   return signature; // TODO: Implement proper recovery
}

} } // steem::chain
```

### 4. Witness Schedule Validation

Modify [libraries/chain/database.cpp](../../../libraries/chain/database.cpp) to validate NFT ownership before allowing block production:

```cpp
bool database::validate_witness_nft_ownership( const account_name_type& witness ) const
{
   // Get witness NFT proof
   const auto& proof_idx = get_index< witness_nft_proof_index >().indices().get< by_witness >();
   auto proof_itr = proof_idx.find( witness );

   if( proof_itr == proof_idx.end() )
      return false; // No proof submitted

   // Check verification status
   if( proof_itr->verification_status != witness_nft_proof_object::verified )
      return false; // Not verified

   // Check if proof is still valid (not expired)
   auto now = head_block_time();
   if( now - proof_itr->proof_timestamp > fc::days(7) )
      return false; // Proof expired (require refresh every 7 days)

   return true;
}

void database::update_witness_schedule()
{
   // ... existing witness schedule logic ...

   // Filter out witnesses without valid NFT ownership
   // Remove witnesses without valid NFT proofs from schedule
   auto& active_witnesses = get_witness_schedule_object().current_shuffled_witnesses;

   active_witnesses.erase(
      std::remove_if( active_witnesses.begin(), active_witnesses.end(),
         [this]( const account_name_type& witness ) {
            return !validate_witness_nft_ownership( witness );
         }
      ),
      active_witnesses.end()
   );

   FC_ASSERT( active_witnesses.size() > 0,
              "No witnesses with valid NFT ownership available for block production" );

   // ... rest of scheduling logic ...
}
```

### 5. NFT Verification Oracle Plugin

Create a new plugin [libraries/plugins/nft_oracle/](../../../libraries/plugins/nft_oracle/) to handle off-chain verification:

Create `nft_oracle_plugin.hpp`:

```cpp
#pragma once

#include <appbase/application.hpp>
#include <steem/plugins/chain/chain_plugin.hpp>

namespace steem { namespace plugins { namespace nft_oracle {

namespace detail { class nft_oracle_plugin_impl; }

class nft_oracle_plugin : public appbase::plugin< nft_oracle_plugin >
{
   public:
      APPBASE_PLUGIN_REQUIRES( (steem::plugins::chain::chain_plugin) )

      nft_oracle_plugin();
      virtual ~nft_oracle_plugin();

      static const std::string& name() { static std::string name = "nft_oracle"; return name; }

      virtual void set_program_options(
         boost::program_options::options_description& cli,
         boost::program_options::options_description& cfg ) override;

      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;

   private:
      std::unique_ptr< detail::nft_oracle_plugin_impl > my;
};

} } } // steem::plugins::nft_oracle
```

Create `nft_oracle_plugin.cpp`:

```cpp
#include <steem/plugins/nft_oracle/nft_oracle_plugin.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/nft_verification_objects.hpp>

namespace steem { namespace plugins { namespace nft_oracle {

namespace detail {

class nft_oracle_plugin_impl
{
   public:
      nft_oracle_plugin_impl( nft_oracle_plugin& _plugin )
         : _self( _plugin ) {}

      void on_nft_proof_submitted( const witness_nft_proof_operation& op );
      void verify_nft_ownership( const witness_nft_proof_object& proof );

      // EVM chain RPC endpoints
      std::map< uint32_t, std::string > rpc_endpoints;

      // Thread pool for async verification
      boost::asio::io_service io_service;
      std::unique_ptr< boost::asio::io_service::work > work;
      boost::thread_group thread_pool;

   private:
      nft_oracle_plugin& _self;
      chain::database* _db = nullptr;
};

void nft_oracle_plugin_impl::on_nft_proof_submitted( const witness_nft_proof_operation& op )
{
   // Queue verification task
   io_service.post( [this, op]() {
      // Get proof object from database
      auto& db = _self.chain_plugin().db();

      db.with_read_lock( [&]() {
         const auto& proof_idx = db.get_index< witness_nft_proof_index >().indices().get< by_witness >();
         auto proof_itr = proof_idx.find( op.witness_account );

         if( proof_itr != proof_idx.end() )
         {
            verify_nft_ownership( *proof_itr );
         }
      });
   });
}

void nft_oracle_plugin_impl::verify_nft_ownership( const witness_nft_proof_object& proof )
{
   try
   {
      // Get RPC endpoint for the chain
      auto rpc_itr = rpc_endpoints.find( proof.chain_id );
      if( rpc_itr == rpc_endpoints.end() )
      {
         elog( "No RPC endpoint configured for chain ${chain}", ("chain", proof.chain_id) );
         return;
      }

      // Call ERC-721 ownerOf(tokenId) function
      // eth_call to contract address with function selector: 0x6352211e + padded token_id

      std::string method = "eth_call";
      std::string to = proof.contract_address;
      std::string data = "0x6352211e" + pad_hex( proof.token_id, 64 ); // ownerOf function selector

      // Make JSON-RPC request to EVM node
      fc::variant result = make_rpc_call( rpc_itr->second, method,
                                          fc::variants{ to, data, "latest" } );

      // Parse result (owner address)
      std::string owner_address = result.as_string();

      // Compare with claimed address
      bool verified = ( to_lowercase( owner_address ) == to_lowercase( proof.eth_address ) );

      // Update proof object
      auto& db = _self.chain_plugin().db();
      db.with_write_lock( [&]() {
         const auto& proof_obj = db.get< witness_nft_proof_object >( proof.id );
         db.modify( proof_obj, [&]( witness_nft_proof_object& p ) {
            p.verification_status = verified ?
               witness_nft_proof_object::verified :
               witness_nft_proof_object::failed;
            p.verification_timestamp = db.head_block_time();
            p.verification_details = verified ?
               "Ownership verified on-chain" :
               "Address " + owner_address + " owns the NFT, not " + proof.eth_address;
         });
      });

      ilog( "NFT verification ${result} for witness ${witness}: ${contract}:${token}",
            ("result", verified ? "SUCCESS" : "FAILED")
            ("witness", proof.witness_account)
            ("contract", proof.contract_address)
            ("token", proof.token_id) );
   }
   catch( const fc::exception& e )
   {
      elog( "NFT verification error: ${e}", ("e", e.to_detail_string()) );

      // Mark as failed
      auto& db = _self.chain_plugin().db();
      db.with_write_lock( [&]() {
         const auto& proof_obj = db.get< witness_nft_proof_object >( proof.id );
         db.modify( proof_obj, [&]( witness_nft_proof_object& p ) {
            p.verification_status = witness_nft_proof_object::failed;
            p.verification_timestamp = db.head_block_time();
            p.verification_details = "Verification error: " + e.to_string();
         });
      });
   }
}

} // detail

nft_oracle_plugin::nft_oracle_plugin() {}
nft_oracle_plugin::~nft_oracle_plugin() {}

void nft_oracle_plugin::set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg )
{
   cfg.add_options()
      ("nft-oracle-rpc-endpoint", boost::program_options::value< std::vector<std::string> >()->composing(),
       "EVM RPC endpoint in format chain_id:url (e.g., 1:https://eth.llamarpc.com)")
      ("nft-oracle-threads", boost::program_options::value<uint32_t>()->default_value(4),
       "Number of threads for NFT verification")
      ;
}

void nft_oracle_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   my = std::make_unique< detail::nft_oracle_plugin_impl >( *this );

   // Parse RPC endpoints
   if( options.count("nft-oracle-rpc-endpoint") )
   {
      auto endpoints = options["nft-oracle-rpc-endpoint"].as< std::vector<std::string> >();
      for( const auto& endpoint : endpoints )
      {
         auto colon_pos = endpoint.find(':');
         FC_ASSERT( colon_pos != std::string::npos,
                    "Invalid RPC endpoint format: ${ep}", ("ep", endpoint) );

         uint32_t chain_id = std::stoi( endpoint.substr(0, colon_pos) );
         std::string url = endpoint.substr( colon_pos + 1 );

         my->rpc_endpoints[chain_id] = url;
         ilog( "Configured NFT oracle for chain ${chain}: ${url}",
               ("chain", chain_id)("url", url) );
      }
   }

   // Initialize thread pool
   uint32_t num_threads = options["nft-oracle-threads"].as<uint32_t>();
   my->work = std::make_unique< boost::asio::io_service::work >( my->io_service );

   for( uint32_t i = 0; i < num_threads; ++i )
   {
      my->thread_pool.create_thread( [this]() {
         my->io_service.run();
      });
   }

   // Register signal handler
   chain_plugin().db().notify_nft_proof_submitted.connect(
      [this]( const witness_nft_proof_operation& op ) {
         my->on_nft_proof_submitted( op );
      }
   );
}

void nft_oracle_plugin::plugin_startup()
{
   ilog( "NFT Oracle Plugin started" );
}

void nft_oracle_plugin::plugin_shutdown()
{
   my->work.reset();
   my->thread_pool.join_all();
   ilog( "NFT Oracle Plugin shutdown" );
}

} } } // steem::plugins::nft_oracle
```

### 6. API Support

Add API methods to query NFT verification status in [libraries/plugins/apis/database_api/database_api.cpp](../../../libraries/plugins/apis/database_api/database_api.cpp):

```cpp
struct get_witness_nft_proof_return
{
   bool has_proof = false;
   optional< witness_nft_proof_object > proof;
};

struct list_nft_collections_return
{
   vector< nft_collection_object > collections;
};

get_witness_nft_proof_return database_api::get_witness_nft_proof(
   string witness_account ) const
{
   return my->_db.with_read_lock( [&]()
   {
      get_witness_nft_proof_return result;

      const auto& proof_idx = my->_db.get_index< witness_nft_proof_index >()
                                     .indices().get< by_witness >();
      auto proof_itr = proof_idx.find( witness_account );

      if( proof_itr != proof_idx.end() )
      {
         result.has_proof = true;
         result.proof = *proof_itr;
      }

      return result;
   });
}

list_nft_collections_return database_api::list_nft_collections(
   bool active_only ) const
{
   return my->_db.with_read_lock( [&]()
   {
      list_nft_collections_return result;

      const auto& collection_idx = my->_db.get_index< nft_collection_index >()
                                          .indices().get< by_id >();

      for( auto itr = collection_idx.begin(); itr != collection_idx.end(); ++itr )
      {
         if( !active_only || itr->active )
         {
            result.collections.push_back( *itr );
         }
      }

      return result;
   });
}
```

## Usage Examples

### 1. Register an NFT Collection

Witnesses propose and vote on NFT collections:

```javascript
// Witness proposes a new NFT collection
const registerOp = {
  type: 'nft_collection_register_operation',
  value: {
    proposer: 'witness-alice',
    collection_name: 'Bored Ape Yacht Club',
    contract_address: '0xBC4CA0EdA7647A8aB7C2061c2E118A18a936f13D',
    chain_id: 1, // Ethereum Mainnet
    min_nft_count: 1,
    active: true,
    extensions: []
  }
};

// Other witnesses approve
const approveOp = {
  type: 'nft_collection_approve_operation',
  value: {
    witness_account: 'witness-bob',
    collection_id: 1,
    approve: true,
    extensions: []
  }
};
```

### 2. Submit NFT Ownership Proof

Witnesses must periodically prove NFT ownership:

```javascript
// Witness proves ownership of NFT
// 1. Generate signature with Ethereum private key
const ethPrivateKey = '0x...'; // Witness's Ethereum private key
const witnessAccount = 'witness-alice';
const nftContract = '0xBC4CA0EdA7647A8aB7C2061c2E118A18a936f13D';
const tokenId = '1234';
const timestamp = Math.floor(Date.now() / 1000);

// Message format: "steem:<witness_account>:nft:<contract>:<token_id>:<timestamp>"
const message = `steem:${witnessAccount}:nft:${nftContract}:${tokenId}:${timestamp}`;

// Sign with eth_sign (using ethers.js or web3.js)
const signature = await wallet.signMessage(message);

// 2. Submit proof operation
const proofOp = {
  type: 'witness_nft_proof_operation',
  value: {
    witness_account: 'witness-alice',
    eth_address: '0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb',
    contract_address: nftContract,
    token_id: tokenId,
    chain_id: 1,
    eth_signature: signature,
    timestamp: timestamp,
    extensions: []
  }
};
```

### 3. Query NFT Verification Status

Check if a witness has valid NFT proof:

```javascript
// Check witness NFT proof status
const proof = await api.call('database_api', 'get_witness_nft_proof', {
  witness_account: 'witness-alice'
});

if (proof.has_proof) {
  console.log('Verification status:', proof.proof.verification_status);
  console.log('Proof timestamp:', proof.proof.proof_timestamp);
  console.log('Details:', proof.proof.verification_details);
} else {
  console.log('No NFT proof submitted');
}

// List all registered NFT collections
const collections = await api.call('database_api', 'list_nft_collections', {
  active_only: true
});

console.log('Active NFT collections:', collections.collections);
```

## Testing

### Unit Tests

Add tests to [tests/tests/operation_tests.cpp](../../../tests/tests/operation_tests.cpp):

```cpp
BOOST_AUTO_TEST_SUITE( nft_witness_tests )

BOOST_AUTO_TEST_CASE( nft_collection_register_validation )
{ try {
   BOOST_TEST_MESSAGE( "Testing NFT collection registration validation" );

   nft_collection_register_operation op;
   op.proposer = "alice";
   op.collection_name = "Test Collection";
   op.contract_address = "0x1234567890123456789012345678901234567890";
   op.chain_id = 1;
   op.min_nft_count = 1;

   REQUIRE_OP_VALIDATION_SUCCESS( op, nft_collection_register_operation );

   // Invalid contract address
   op.contract_address = "invalid";
   REQUIRE_OP_VALIDATION_FAILURE( op, nft_collection_register_operation );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( witness_nft_proof_validation )
{ try {
   BOOST_TEST_MESSAGE( "Testing witness NFT proof validation" );

   witness_nft_proof_operation op;
   op.witness_account = "alice";
   op.eth_address = "0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb";
   op.contract_address = "0xBC4CA0EdA7647A8aB7C2061c2E118A18a936f13D";
   op.token_id = "1234";
   op.chain_id = 1;
   op.eth_signature = "0x1234..."; // Valid signature
   op.timestamp = fc::time_point::now();

   REQUIRE_OP_VALIDATION_SUCCESS( op, witness_nft_proof_operation );

   // Invalid Ethereum address
   op.eth_address = "invalid";
   REQUIRE_OP_VALIDATION_FAILURE( op, witness_nft_proof_operation );

   // Timestamp too old
   op.eth_address = "0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb";
   op.timestamp = fc::time_point::now() - fc::days(2);
   REQUIRE_OP_VALIDATION_FAILURE( op, witness_nft_proof_operation );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( nft_collection_approval_process )
{ try {
   BOOST_TEST_MESSAGE( "Testing NFT collection approval process" );

   // Create 21 witness accounts
   ACTORS( (alice)(bob)(charlie) /* ... create 21 witnesses ... */ )

   // Register collection
   nft_collection_register_operation register_op;
   register_op.proposer = "alice";
   register_op.collection_name = "Test NFT";
   register_op.contract_address = "0x1234567890123456789012345678901234567890";
   register_op.chain_id = 1;

   push_transaction( register_op, alice_private_key );
   generate_block();

   // Get collection ID
   const auto& collection_idx = db->get_index< nft_collection_index >()
                                    .indices().get< by_collection_name >();
   auto collection_itr = collection_idx.find( "Test NFT" );
   BOOST_REQUIRE( collection_itr != collection_idx.end() );
   BOOST_REQUIRE( !collection_itr->active ); // Not yet active

   uint64_t collection_id = collection_itr->id._id;

   // 10 more witnesses approve (total 11 including proposer)
   for( int i = 0; i < 10; i++ )
   {
      nft_collection_approve_operation approve_op;
      approve_op.witness_account = /* witness accounts */;
      approve_op.collection_id = collection_id;
      approve_op.approve = true;

      push_transaction( approve_op, /* witness keys */ );
   }

   generate_block();

   // Check if collection is now active (majority approved)
   collection_itr = collection_idx.find( "Test NFT" );
   BOOST_REQUIRE( collection_itr->active );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( witness_block_production_requires_nft )
{ try {
   BOOST_TEST_MESSAGE( "Testing witness block production requires NFT proof" );

   ACTORS( (alice)(bob) )

   // Enable hardfork
   generate_blocks( STEEM_HARDFORK_0_XX_TIME );

   // Alice becomes witness but has no NFT proof
   // Should not be able to produce blocks

   // Bob becomes witness and submits NFT proof
   // Should be able to produce blocks after verification

   // TODO: Complete test implementation

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
```

## Security Considerations

### 1. Signature Verification

- Ethereum signature must be verified to prove control of the eth_address
- Use EIP-191 standard for message signing
- Prevent replay attacks with timestamp validation

### 2. Oracle Trust

- Oracle service is a trusted component that can be compromised
- Consider multi-oracle verification for critical operations
- Implement oracle reputation system
- Add fallback mechanisms if oracle fails

### 3. NFT Transfer Attacks

- Attacker could briefly acquire NFT, submit proof, then transfer it
- Require proofs to be recent (< 24 hours old)
- Consider requiring proof of ownership for extended periods
- Monitor for rapid NFT transfers after proof submission

### 4. Chain Reorganization

- Handle blockchain reorgs properly
- Proofs should reference recent block hashes
- Oracle should wait for sufficient confirmations

### 5. Witness Collusion

- Malicious witnesses could approve fake NFT collections
- Require supermajority (e.g., 15/21) for controversial collections
- Implement governance mechanism for collection removal

### 6. DoS Attacks

- Limit rate of proof submissions per witness
- Prevent spam NFT collection registrations
- Implement fees for registration and proofs

## Performance Considerations

### 1. Oracle Latency

- NFT verification requires external RPC calls (can be slow)
- Use caching for recently verified proofs
- Implement async verification to not block chain operations
- Consider proof pre-submission before witness scheduling

### 2. Storage Costs

- Each proof object stores Ethereum addresses and signatures
- Prune old expired proofs periodically
- Limit number of active NFT collections

### 3. Network Bandwidth

- Ethereum RPC calls add external network dependency
- Use local Ethereum nodes when possible
- Implement connection pooling and request batching

## Future Enhancements

### 1. Multi-NFT Requirements

Support requiring multiple NFTs from different collections:

```cpp
struct multi_nft_requirement {
   vector< nft_collection_id_type > required_collections;
   uint32_t min_collections_required; // "X out of Y" requirement
};
```

### 2. NFT Staking

Allow witnesses to "stake" NFTs by locking them in a smart contract:

- Prevents rapid transfer after proof
- Provides economic security guarantees
- Can slash staked NFTs for misbehavior

### 3. Delegated NFT Ownership

Allow NFT owners to delegate witness rights to operators:

```cpp
struct nft_delegation_operation {
   string nft_owner_eth_address;
   account_name_type delegated_witness;
   fc::time_point_sec expiration;
};
```

### 4. Dynamic NFT Requirements

Adjust NFT requirements based on chain activity:

- Require more valuable NFTs during high-stakes periods
- Allow fallback to traditional witness voting if NFT pool is too small

### 5. Cross-Chain Bridges

Integrate with cross-chain bridges to verify NFTs on non-EVM chains:

- Bitcoin Ordinals
- Solana NFTs
- Cosmos NFTs

### 6. Reputation-Based NFT Values

Weight NFTs differently based on:

- Collection floor price
- Rarity traits
- Historical ownership

## Deployment Strategy

### Phase 1: Collection Registration (Pre-Hardfork)

1. Deploy nft_oracle plugin to all witness nodes
2. Configure EVM RPC endpoints
3. Witnesses register and approve initial NFT collections
4. Test oracle verification on testnet

### Phase 2: Soft Deployment (Post-Hardfork)

1. Activate hardfork with NFT operations
2. Witnesses submit proofs voluntarily
3. Monitor proof verification success rates
4. Do NOT enforce NFT requirements for block production yet

### Phase 3: Hard Enforcement (After Stabilization)

1. After 30 days of successful proof submissions
2. Enable witness schedule filtering based on NFT ownership
3. Witnesses without valid proofs are excluded from block production

### Rollback Plan

If critical issues arise:

1. Emergency hardfork to disable NFT requirements
2. Revert to traditional witness scheduling
3. Investigate and fix issues
4. Re-enable in future hardfork

## Configuration

### Witness Node Config

Add to `config.ini`:

```ini
# Enable NFT oracle plugin
plugin = nft_oracle

# Configure EVM RPC endpoints
nft-oracle-rpc-endpoint = 1:https://eth.llamarpc.com
nft-oracle-rpc-endpoint = 137:https://polygon-rpc.com
nft-oracle-rpc-endpoint = 56:https://bsc-dataseed.binance.org

# Number of verification threads
nft-oracle-threads = 4
```

### Seed Node Config

Seed nodes don't need oracle plugin, but should track NFT objects:

```ini
# Minimal NFT support (no oracle)
# NFT objects will be tracked but not verified
```

## Related Documentation

- [Plugin Development Guide](../plugin.md)
- [Create Operation Guide](../create-operation.md)
- [Hardfork Procedure](../../operations/hardfork-procedure.md)
- [Witness Operations](../../operations/witness-guide.md)

## References

- **ERC-721 Standard**: https://eips.ethereum.org/EIPS/eip-721
- **EIP-191 Signed Data**: https://eips.ethereum.org/EIPS/eip-191
- **Ethereum JSON-RPC**: https://ethereum.org/en/developers/docs/apis/json-rpc/
- **Protocol Operations**: [libraries/protocol/include/steem/protocol/steem_operations.hpp](../../../libraries/protocol/include/steem/protocol/steem_operations.hpp)
- **Chain Database**: [libraries/chain/database.cpp](../../../libraries/chain/database.cpp)
- **Plugin Framework**: [libraries/appbase/](../../../libraries/appbase/)
