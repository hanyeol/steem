#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <steem/protocol/exceptions.hpp>
#include <steem/protocol/hardfork.hpp>

#include <steem/chain/block_summary_object.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/database_exceptions.hpp>
#include <steem/chain/history_object.hpp>
#include <steem/chain/steem_objects.hpp>

#include <steem/chain/utils/reward.hpp>

#include <steem/plugins/debug_node/debug_node_plugin.hpp>
#include <steem/plugins/witness/witness_objects.hpp>

#include <fc/macros.hpp>
#include <fc/crypto/digest.hpp>

#include "../../fixtures/database_fixture.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace steem;
using namespace steem::chain;
using namespace steem::protocol;
using fc::string;

BOOST_FIXTURE_TEST_SUITE( operation_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( pow_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: pow_validate" );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( pow_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: pow_authorities" );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( pow_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: pow_apply" );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_recovery )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account recovery" );

      ACTORS( (alice) );
      fund( "alice", 1000000 );

      BOOST_TEST_MESSAGE( "Creating account bob with alice" );

      account_create_operation acc_create;
      acc_create.fee = ASSET( "10.000 TESTS" );
      acc_create.creator = "alice";
      acc_create.new_account_name = "bob";
      acc_create.owner = authority( 1, generate_private_key( "bob_owner" ).get_public_key(), 1 );
      acc_create.active = authority( 1, generate_private_key( "bob_active" ).get_public_key(), 1 );
      acc_create.posting = authority( 1, generate_private_key( "bob_posting" ).get_public_key(), 1 );
      acc_create.memo_key = generate_private_key( "bob_memo" ).get_public_key();
      acc_create.json_metadata = "";


      signed_transaction tx;
      tx.operations.push_back( acc_create );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      const auto& bob_auth = db->get< account_authority_object, by_account >( "bob" );
      BOOST_REQUIRE( bob_auth.owner == acc_create.owner );


      BOOST_TEST_MESSAGE( "Changing bob's owner authority" );

      account_update_operation acc_update;
      acc_update.account = "bob";
      acc_update.owner = authority( 1, generate_private_key( "bad_key" ).get_public_key(), 1 );
      acc_update.memo_key = acc_create.memo_key;
      acc_update.json_metadata = "";

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( acc_update );
      tx.sign( generate_private_key( "bob_owner" ), db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( bob_auth.owner == *acc_update.owner );


      BOOST_TEST_MESSAGE( "Creating recover request for bob with alice" );

      request_account_recovery_operation request;
      request.recovery_account = "alice";
      request.account_to_recover = "bob";
      request.new_owner_authority = authority( 1, generate_private_key( "new_key" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( request );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( bob_auth.owner == *acc_update.owner );


      BOOST_TEST_MESSAGE( "Recovering bob's account with original owner auth and new secret" );

      generate_blocks( db->head_block_time() + STEEM_OWNER_UPDATE_LIMIT );

      recover_account_operation recover;
      recover.account_to_recover = "bob";
      recover.new_owner_authority = request.new_owner_authority;
      recover.recent_owner_authority = acc_create.owner;

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( recover );
      tx.sign( generate_private_key( "bob_owner" ), db->get_chain_id() );
      tx.sign( generate_private_key( "new_key" ), db->get_chain_id() );
      db->push_transaction( tx, 0 );
      const auto& owner1 = db->get< account_authority_object, by_account >("bob").owner;

      BOOST_REQUIRE( owner1 == recover.new_owner_authority );


      BOOST_TEST_MESSAGE( "Creating new recover request for a bogus key" );

      request.new_owner_authority = authority( 1, generate_private_key( "foo bar" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( request );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );


      BOOST_TEST_MESSAGE( "Testing failure when bob does not have new authority" );

      generate_blocks( db->head_block_time() + STEEM_OWNER_UPDATE_LIMIT + fc::seconds( STEEM_BLOCK_INTERVAL ) );

      recover.new_owner_authority = authority( 1, generate_private_key( "idontknow" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( recover );
      tx.sign( generate_private_key( "bob_owner" ), db->get_chain_id() );
      tx.sign( generate_private_key( "idontknow" ), db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      const auto& owner2 = db->get< account_authority_object, by_account >("bob").owner;
      BOOST_REQUIRE( owner2 == authority( 1, generate_private_key( "new_key" ).get_public_key(), 1 ) );


      BOOST_TEST_MESSAGE( "Testing failure when bob does not have old authority" );

      recover.recent_owner_authority = authority( 1, generate_private_key( "idontknow" ).get_public_key(), 1 );
      recover.new_owner_authority = authority( 1, generate_private_key( "foo bar" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( recover );
      tx.sign( generate_private_key( "foo bar" ), db->get_chain_id() );
      tx.sign( generate_private_key( "idontknow" ), db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      const auto& owner3 = db->get< account_authority_object, by_account >("bob").owner;
      BOOST_REQUIRE( owner3 == authority( 1, generate_private_key( "new_key" ).get_public_key(), 1 ) );


      BOOST_TEST_MESSAGE( "Testing using the same old owner auth again for recovery" );

      recover.recent_owner_authority = authority( 1, generate_private_key( "bob_owner" ).get_public_key(), 1 );
      recover.new_owner_authority = authority( 1, generate_private_key( "foo bar" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( recover );
      tx.sign( generate_private_key( "bob_owner" ), db->get_chain_id() );
      tx.sign( generate_private_key( "foo bar" ), db->get_chain_id() );
      db->push_transaction( tx, 0 );

      const auto& owner4 = db->get< account_authority_object, by_account >("bob").owner;
      BOOST_REQUIRE( owner4 == recover.new_owner_authority );

      BOOST_TEST_MESSAGE( "Creating a recovery request that will expire" );

      request.new_owner_authority = authority( 1, generate_private_key( "expire" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( request );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      const auto& request_idx = db->get_index< account_recovery_request_index >().indices();
      auto req_itr = request_idx.begin();

      BOOST_REQUIRE( req_itr->account_to_recover == "bob" );
      BOOST_REQUIRE( req_itr->new_owner_authority == authority( 1, generate_private_key( "expire" ).get_public_key(), 1 ) );
      BOOST_REQUIRE( req_itr->expires == db->head_block_time() + STEEM_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD );
      auto expires = req_itr->expires;
      ++req_itr;
      BOOST_REQUIRE( req_itr == request_idx.end() );

      generate_blocks( time_point_sec( expires - STEEM_BLOCK_INTERVAL ), true );

      const auto& new_request_idx = db->get_index< account_recovery_request_index >().indices();
      BOOST_REQUIRE( new_request_idx.begin() != new_request_idx.end() );

      generate_block();

      BOOST_REQUIRE( new_request_idx.begin() == new_request_idx.end() );

      recover.new_owner_authority = authority( 1, generate_private_key( "expire" ).get_public_key(), 1 );
      recover.recent_owner_authority = authority( 1, generate_private_key( "bob_owner" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( recover );
      tx.set_expiration( db->head_block_time() );
      tx.sign( generate_private_key( "expire" ), db->get_chain_id() );
      tx.sign( generate_private_key( "bob_owner" ), db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      const auto& owner5 = db->get< account_authority_object, by_account >("bob").owner;
      BOOST_REQUIRE( owner5 == authority( 1, generate_private_key( "foo bar" ).get_public_key(), 1 ) );

      BOOST_TEST_MESSAGE( "Expiring owner authority history" );

      acc_update.owner = authority( 1, generate_private_key( "new_key" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( acc_update );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( generate_private_key( "foo bar" ), db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( db->head_block_time() + ( STEEM_OWNER_AUTH_RECOVERY_PERIOD - STEEM_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD ) );
      generate_block();

      request.new_owner_authority = authority( 1, generate_private_key( "last key" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( request );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      recover.new_owner_authority = request.new_owner_authority;
      recover.recent_owner_authority = authority( 1, generate_private_key( "bob_owner" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( recover );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( generate_private_key( "bob_owner" ), db->get_chain_id() );
      tx.sign( generate_private_key( "last key" ), db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      const auto& owner6 = db->get< account_authority_object, by_account >("bob").owner;
      BOOST_REQUIRE( owner6 == authority( 1, generate_private_key( "new_key" ).get_public_key(), 1 ) );

      recover.recent_owner_authority = authority( 1, generate_private_key( "foo bar" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( recover );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( generate_private_key( "foo bar" ), db->get_chain_id() );
      tx.sign( generate_private_key( "last key" ), db->get_chain_id() );
      db->push_transaction( tx, 0 );
      const auto& owner7 = db->get< account_authority_object, by_account >("bob").owner;
      BOOST_REQUIRE( owner7 == authority( 1, generate_private_key( "last key" ).get_public_key(), 1 ) );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( change_recovery_account )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing change_recovery_account_operation" );

      ACTORS( (alice)(bob)(sam)(tyler) )

      auto change_recovery_account = [&]( const std::string& account_to_recover, const std::string& new_recovery_account )
      {
         change_recovery_account_operation op;
         op.account_to_recover = account_to_recover;
         op.new_recovery_account = new_recovery_account;

         signed_transaction tx;
         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         tx.sign( alice_private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );
      };

      auto recover_account = [&]( const std::string& account_to_recover, const fc::ecc::private_key& new_owner_key, const fc::ecc::private_key& recent_owner_key )
      {
         recover_account_operation op;
         op.account_to_recover = account_to_recover;
         op.new_owner_authority = authority( 1, public_key_type( new_owner_key.get_public_key() ), 1 );
         op.recent_owner_authority = authority( 1, public_key_type( recent_owner_key.get_public_key() ), 1 );

         signed_transaction tx;
         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         tx.sign( recent_owner_key, db->get_chain_id() );
         // only Alice -> throw
         STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
         tx.signatures.clear();
         tx.sign( new_owner_key, db->get_chain_id() );
         // only Sam -> throw
         STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
         tx.sign( recent_owner_key, db->get_chain_id() );
         // Alice+Sam -> OK
         db->push_transaction( tx, 0 );
      };

      auto request_account_recovery = [&]( const std::string& recovery_account, const fc::ecc::private_key& recovery_account_key, const std::string& account_to_recover, const public_key_type& new_owner_key )
      {
         request_account_recovery_operation op;
         op.recovery_account    = recovery_account;
         op.account_to_recover  = account_to_recover;
         op.new_owner_authority = authority( 1, new_owner_key, 1 );

         signed_transaction tx;
         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         tx.sign( recovery_account_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );
      };

      auto change_owner = [&]( const std::string& account, const fc::ecc::private_key& old_private_key, const public_key_type& new_public_key )
      {
         account_update_operation op;
         op.account = account;
         op.owner = authority( 1, new_public_key, 1 );

         signed_transaction tx;
         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         tx.sign( old_private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );
      };

      // if either/both users do not exist, we shouldn't allow it
      STEEM_REQUIRE_THROW( change_recovery_account("alice", "nobody"), fc::exception );
      STEEM_REQUIRE_THROW( change_recovery_account("haxer", "sam"   ), fc::exception );
      STEEM_REQUIRE_THROW( change_recovery_account("haxer", "nobody"), fc::exception );
      change_recovery_account("alice", "sam");

      fc::ecc::private_key alice_priv1 = fc::ecc::private_key::regenerate( fc::sha256::hash( "alice_k1" ) );
      fc::ecc::private_key alice_priv2 = fc::ecc::private_key::regenerate( fc::sha256::hash( "alice_k2" ) );
      public_key_type alice_pub1 = public_key_type( alice_priv1.get_public_key() );

      generate_blocks( db->head_block_time() + STEEM_OWNER_AUTH_RECOVERY_PERIOD - fc::seconds( STEEM_BLOCK_INTERVAL ), true );
      // cannot request account recovery until recovery account is approved
      STEEM_REQUIRE_THROW( request_account_recovery( "sam", sam_private_key, "alice", alice_pub1 ), fc::exception );
      generate_blocks(1);
      // cannot finish account recovery until requested
      STEEM_REQUIRE_THROW( recover_account( "alice", alice_priv1, alice_private_key ), fc::exception );
      // do the request
      request_account_recovery( "sam", sam_private_key, "alice", alice_pub1 );
      // can't recover with the current owner key
      STEEM_REQUIRE_THROW( recover_account( "alice", alice_priv1, alice_private_key ), fc::exception );
      // unless we change it!
      change_owner( "alice", alice_private_key, public_key_type( alice_priv2.get_public_key() ) );
      recover_account( "alice", alice_priv1, alice_private_key );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_bandwidth )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_bandwidth" );
      ACTORS( (alice)(bob) )
      generate_block();
      vest( "alice", ASSET( "10.000 TESTS" ) );
      fund( "alice", ASSET( "10.000 TESTS" ) );
      vest( "bob", ASSET( "10.000 TESTS" ) );

      generate_block();
      db->skip_transaction_delta_check = false;

      BOOST_TEST_MESSAGE( "--- Test first tx in block" );

      signed_transaction tx;
      transfer_operation op;

      op.from = "alice";
      op.to = "bob";
      op.amount = ASSET( "1.000 TESTS" );

      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      auto last_bandwidth_update = db->get< plugins::witness::account_bandwidth_object, plugins::witness::by_account_bandwidth_type >( boost::make_tuple( "alice", plugins::witness::bandwidth_type::market ) ).last_bandwidth_update;
      auto average_bandwidth = db->get< plugins::witness::account_bandwidth_object, plugins::witness::by_account_bandwidth_type >( boost::make_tuple( "alice", plugins::witness::bandwidth_type::market ) ).average_bandwidth;
      BOOST_REQUIRE( last_bandwidth_update == db->head_block_time() );
      BOOST_REQUIRE( average_bandwidth == fc::raw::pack_size( tx ) * 10 * STEEM_BANDWIDTH_PRECISION );
      auto total_bandwidth = average_bandwidth;

      BOOST_TEST_MESSAGE( "--- Test second tx in block" );

      op.amount = ASSET( "0.100 TESTS" );
      tx.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      last_bandwidth_update = db->get< plugins::witness::account_bandwidth_object, plugins::witness::by_account_bandwidth_type >( boost::make_tuple( "alice", plugins::witness::bandwidth_type::market ) ).last_bandwidth_update;
      average_bandwidth = db->get< plugins::witness::account_bandwidth_object, plugins::witness::by_account_bandwidth_type >( boost::make_tuple( "alice", plugins::witness::bandwidth_type::market ) ).average_bandwidth;
      BOOST_REQUIRE( last_bandwidth_update == db->head_block_time() );
      BOOST_REQUIRE( average_bandwidth == total_bandwidth + fc::raw::pack_size( tx ) * 10 * STEEM_BANDWIDTH_PRECISION );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( delegate_vesting_shares_validate )
{
   try
   {
      delegate_vesting_shares_operation op;

      op.delegator = "alice";
      op.delegatee = "bob";
      op.vesting_shares = asset( -1, VESTS_SYMBOL );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( delegate_vesting_shares_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: delegate_vesting_shares_authorities" );
      signed_transaction tx;
      ACTORS( (alice)(bob) )
      fund( "alice", 500000 );
      vest( "alice", 500000 );

      delegate_vesting_shares_operation op;
      op.vesting_shares = ASSET( "300.000000 VESTS");
      op.delegator = "alice";
      op.delegatee = "bob";

      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );

      BOOST_TEST_MESSAGE( "--- Test failure when no signatures" );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test success with witness signature" );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- Test failure when duplicate signatures" );
      tx.operations.clear();
      tx.signatures.clear();
      op.delegatee = "sam";
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by an additional signature not in the creator's authority" );
      tx.signatures.clear();
      tx.sign( init_account_priv_key, db->get_chain_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by a signature not in the creator's authority" );
      tx.signatures.clear();
      tx.sign( init_account_priv_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( delegate_vesting_shares_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: delegate_vesting_shares_apply" );
      signed_transaction tx;
      ACTORS( (alice)(bob) )
      generate_block();

      fund( "alice", ASSET( "40000000.000 TESTS" ) );
      vest( "alice", ASSET( "40000000.000 TESTS" ) );

      generate_block();

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_witness_schedule_object(), [&]( witness_schedule_object& w )
         {
            w.median_props.account_creation_fee = ASSET( "1.000 TESTS" );
         });
      });

      generate_block();

      delegate_vesting_shares_operation op;
      op.vesting_shares = ASSET( "10000000.000000 VESTS");
      op.delegator = "alice";
      op.delegatee = "bob";

      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      generate_blocks( 1 );
      const account_object& alice_acc = db->get_account( "alice" );
      const account_object& bob_acc = db->get_account( "bob" );

      BOOST_REQUIRE( alice_acc.delegated_vesting_shares == ASSET( "10000000.000000 VESTS"));
      BOOST_REQUIRE( bob_acc.received_vesting_shares == ASSET( "10000000.000000 VESTS"));

      BOOST_TEST_MESSAGE( "--- Test that the delegation object is correct. " );
      auto delegation = db->find< vesting_delegation_object, by_delegation >( boost::make_tuple( op.delegator, op.delegatee ) );

      BOOST_REQUIRE( delegation != nullptr );
      BOOST_REQUIRE( delegation->delegator == op.delegator);
      BOOST_REQUIRE( delegation->vesting_shares  == ASSET( "10000000.000000 VESTS"));

      validate_database();
      tx.clear();
      op.vesting_shares = ASSET( "20000000.000000 VESTS");
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      generate_blocks(1);

      BOOST_REQUIRE( delegation != nullptr );
      BOOST_REQUIRE( delegation->delegator == op.delegator);
      BOOST_REQUIRE( delegation->vesting_shares == ASSET( "20000000.000000 VESTS"));
      BOOST_REQUIRE( alice_acc.delegated_vesting_shares == ASSET( "20000000.000000 VESTS"));
      BOOST_REQUIRE( bob_acc.received_vesting_shares == ASSET( "20000000.000000 VESTS"));

      BOOST_TEST_MESSAGE( "--- Test that effective vesting shares is accurate and being applied." );
      tx.operations.clear();
      tx.signatures.clear();

      comment_operation comment_op;
      comment_op.author = "alice";
      comment_op.permlink = "foo";
      comment_op.parent_permlink = "test";
      comment_op.title = "bar";
      comment_op.body = "foo bar";
      tx.operations.push_back( comment_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      tx.signatures.clear();
      tx.operations.clear();
      vote_operation vote_op;
      vote_op.voter = "bob";
      vote_op.author = "alice";
      vote_op.permlink = "foo";
      vote_op.weight = STEEM_100_PERCENT;
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( vote_op );
      tx.sign( bob_private_key, db->get_chain_id() );
      auto old_voting_power = bob_acc.voting_power;

      db->push_transaction( tx, 0 );
      generate_blocks(1);

      const auto& vote_idx = db->get_index< comment_vote_index >().indices().get< by_comment_voter >();

      auto& alice_comment = db->get_comment( "alice", string( "foo" ) );
      auto itr = vote_idx.find( std::make_tuple( alice_comment.id, bob_acc.id ) );
      BOOST_REQUIRE( alice_comment.net_rshares.value == db->get_effective_vesting_shares(bob_acc, VESTS_SYMBOL).amount.value * ( old_voting_power - bob_acc.voting_power ) / STEEM_100_PERCENT - STEEM_VOTE_DUST_THRESHOLD);
      BOOST_REQUIRE( itr->rshares == db->get_effective_vesting_shares(bob_acc, VESTS_SYMBOL).amount.value * ( old_voting_power - bob_acc.voting_power ) / STEEM_100_PERCENT - STEEM_VOTE_DUST_THRESHOLD );


      generate_block();
      ACTORS( (sam)(dave) )
      generate_block();

      vest( "sam", ASSET( "1000.000 TESTS" ) );

      generate_block();

      auto sam_vest = db->get_account( "sam" ).vesting_shares;

      BOOST_TEST_MESSAGE( "--- Test failure when delegating 0 VESTS" );
      tx.clear();
      op.delegator = "sam";
      op.delegatee = "dave";
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( sam_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx ), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Testing failure delegating more vesting shares than account has." );
      tx.clear();
      op.vesting_shares = asset( sam_vest.amount + 1, VESTS_SYMBOL );
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx ), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Test failure delegating vesting shares that are part of a power down" );
      tx.clear();
      sam_vest = asset( sam_vest.amount / 2, VESTS_SYMBOL );
      withdraw_vesting_operation withdraw;
      withdraw.account = "sam";
      withdraw.vesting_shares = sam_vest;
      tx.operations.push_back( withdraw );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.clear();
      op.vesting_shares = asset( sam_vest.amount + 2, VESTS_SYMBOL );
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx ), fc::assert_exception );

      tx.clear();
      withdraw.vesting_shares = ASSET( "0.000000 VESTS" );
      tx.operations.push_back( withdraw );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );


      BOOST_TEST_MESSAGE( "--- Test failure powering down vesting shares that are delegated" );
      sam_vest.amount += 1000;
      op.vesting_shares = sam_vest;
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.clear();
      withdraw.vesting_shares = asset( sam_vest.amount, VESTS_SYMBOL );
      tx.operations.push_back( withdraw );
      tx.sign( sam_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx ), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Remove a delegation and ensure it is returned after 1 week" );
      tx.clear();
      op.vesting_shares = ASSET( "0.000000 VESTS" );
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      auto exp_obj = db->get_index< vesting_delegation_expiration_index, by_id >().begin();
      auto end = db->get_index< vesting_delegation_expiration_index, by_id >().end();
      auto gpo = db->get_dynamic_global_properties();

      BOOST_REQUIRE( gpo.delegation_return_period == STEEM_DELEGATION_RETURN_PERIOD );

      BOOST_REQUIRE( exp_obj != end );
      BOOST_REQUIRE( exp_obj->delegator == "sam" );
      BOOST_REQUIRE( exp_obj->vesting_shares == sam_vest );
      BOOST_REQUIRE( exp_obj->expiration == db->head_block_time() + gpo.delegation_return_period );
      BOOST_REQUIRE( db->get_account( "sam" ).delegated_vesting_shares == sam_vest );
      BOOST_REQUIRE( db->get_account( "dave" ).received_vesting_shares == ASSET( "0.000000 VESTS" ) );
      delegation = db->find< vesting_delegation_object, by_delegation >( boost::make_tuple( op.delegator, op.delegatee ) );
      BOOST_REQUIRE( delegation == nullptr );

      generate_blocks( exp_obj->expiration + STEEM_BLOCK_INTERVAL );

      exp_obj = db->get_index< vesting_delegation_expiration_index, by_id >().begin();
      end = db->get_index< vesting_delegation_expiration_index, by_id >().end();

      BOOST_REQUIRE( exp_obj == end );
      BOOST_REQUIRE( db->get_account( "sam" ).delegated_vesting_shares == ASSET( "0.000000 VESTS" ) );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( issue_971_vesting_removal )
{
   // This is a regression test specifically for issue #971
   try
   {
      BOOST_TEST_MESSAGE( "Test Issue 971 Vesting Removal" );
      ACTORS( (alice)(bob) )
      generate_block();

      fund( "alice", ASSET( "20000000.000 TESTS" ) );
      vest( "alice", ASSET( "20000000.000 TESTS" ) );

      generate_block();

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_witness_schedule_object(), [&]( witness_schedule_object& w )
         {
            w.median_props.account_creation_fee = ASSET( "1.000 TESTS" );
         });
      });

      generate_block();

      signed_transaction tx;
      delegate_vesting_shares_operation op;
      op.vesting_shares = ASSET( "10000000.000000 VESTS");
      op.delegator = "alice";
      op.delegatee = "bob";

      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      generate_block();
      const account_object& alice_acc = db->get_account( "alice" );
      const account_object& bob_acc = db->get_account( "bob" );

      BOOST_REQUIRE( alice_acc.delegated_vesting_shares == ASSET( "10000000.000000 VESTS"));
      BOOST_REQUIRE( bob_acc.received_vesting_shares == ASSET( "10000000.000000 VESTS"));

      generate_block();

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_witness_schedule_object(), [&]( witness_schedule_object& w )
         {
            w.median_props.account_creation_fee = ASSET( "100.000 TESTS" );
         });
      });

      generate_block();

      op.vesting_shares = ASSET( "0.000000 VESTS" );

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      generate_block();

      BOOST_REQUIRE( alice_acc.delegated_vesting_shares == ASSET( "10000000.000000 VESTS"));
      BOOST_REQUIRE( bob_acc.received_vesting_shares == ASSET( "0.000000 VESTS"));
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( comment_beneficiaries_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Test Comment Beneficiaries Validate" );
      comment_options_operation op;

      op.author = "alice";
      op.permlink = "test";

      BOOST_TEST_MESSAGE( "--- Testing more than 100% weight on a single route" );
      comment_payout_beneficiaries b;
      b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "bob" ), STEEM_100_PERCENT + 1 ) );
      op.extensions.insert( b );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Testing more than 100% total weight" );
      b.beneficiaries.clear();
      b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "bob" ), STEEM_1_PERCENT * 75 ) );
      b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "sam" ), STEEM_1_PERCENT * 75 ) );
      op.extensions.clear();
      op.extensions.insert( b );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Testing maximum number of routes" );
      b.beneficiaries.clear();
      for( size_t i = 0; i < 127; i++ )
      {
         b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "foo" + fc::to_string( i ) ), 1 ) );
      }

      op.extensions.clear();
      std::sort( b.beneficiaries.begin(), b.beneficiaries.end() );
      op.extensions.insert( b );
      op.validate();

      BOOST_TEST_MESSAGE( "--- Testing one too many routes" );
      b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "bar" ), 1 ) );
      std::sort( b.beneficiaries.begin(), b.beneficiaries.end() );
      op.extensions.clear();
      op.extensions.insert( b );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Testing duplicate accounts" );
      b.beneficiaries.clear();
      b.beneficiaries.push_back( beneficiary_route_type( "bob", STEEM_1_PERCENT * 2 ) );
      b.beneficiaries.push_back( beneficiary_route_type( "bob", STEEM_1_PERCENT ) );
      op.extensions.clear();
      op.extensions.insert( b );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Testing incorrect account sort order" );
      b.beneficiaries.clear();
      b.beneficiaries.push_back( beneficiary_route_type( "bob", STEEM_1_PERCENT ) );
      b.beneficiaries.push_back( beneficiary_route_type( "alice", STEEM_1_PERCENT ) );
      op.extensions.clear();
      op.extensions.insert( b );
      STEEM_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Testing correct account sort order" );
      b.beneficiaries.clear();
      b.beneficiaries.push_back( beneficiary_route_type( "alice", STEEM_1_PERCENT ) );
      b.beneficiaries.push_back( beneficiary_route_type( "bob", STEEM_1_PERCENT ) );
      op.extensions.clear();
      op.extensions.insert( b );
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( comment_beneficiaries_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Test Comment Beneficiaries" );
      ACTORS( (alice)(bob)(sam)(dave) )
      fund( "bob", 10000 );
      vest( "bob", 10000 );
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );

      comment_operation comment;
      vote_operation vote;
      comment_options_operation op;
      comment_payout_beneficiaries b;
      signed_transaction tx;

      comment.author = "alice";
      comment.permlink = "test";
      comment.parent_permlink = "test";
      comment.title = "test";
      comment.body = "foobar";

      tx.operations.push_back( comment );
      tx.set_expiration( db->head_block_time() + STEEM_MIN_TRANSACTION_EXPIRATION_LIMIT );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx );

      BOOST_TEST_MESSAGE( "--- Test failure on more than 8 benefactors" );
      b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "bob" ), STEEM_1_PERCENT ) );

      for( size_t i = 0; i < 8; i++ )
      {
         b.beneficiaries.push_back( beneficiary_route_type( account_name_type( STEEM_GENESIS_WITNESS_NAME + fc::to_string( i ) ), STEEM_1_PERCENT ) );
      }

      op.author = "alice";
      op.permlink = "test";
      op.allow_curation_rewards = false;
      op.extensions.insert( b );
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx ), chain::plugin_exception );


      BOOST_TEST_MESSAGE( "--- Test specifying a non-existent benefactor" );
      b.beneficiaries.clear();
      b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "doug" ), STEEM_1_PERCENT ) );
      op.extensions.clear();
      op.extensions.insert( b );
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx ), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Test setting when comment has been voted on" );
      vote.author = "alice";
      vote.permlink = "test";
      vote.voter = "bob";
      vote.weight = STEEM_100_PERCENT;

      const auto& bob_account = db->get_account( "bob" );
      idump( (bob_account.vesting_shares) );
      idump( (bob_account.reward_vesting_steem) );
      idump( (bob_account.reward_vesting_balance) );

      b.beneficiaries.clear();
      b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "bob" ), 25 * STEEM_1_PERCENT ) );
      b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "sam" ), 50 * STEEM_1_PERCENT ) );
      op.extensions.clear();
      op.extensions.insert( b );

      tx.clear();
      tx.operations.push_back( vote );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx ), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Test success" );
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx );


      BOOST_TEST_MESSAGE( "--- Test setting when there are already beneficiaries" );
      b.beneficiaries.clear();
      b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "dave" ), 25 * STEEM_1_PERCENT ) );
      op.extensions.clear();
      op.extensions.insert( b );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx ), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Payout and verify rewards were split properly" );
      tx.clear();
      tx.operations.push_back( vote );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( db->get_comment( "alice", string( "test" ) ).cashout_time - STEEM_BLOCK_INTERVAL );

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_dynamic_global_properties(), [=]( dynamic_global_property_object& gpo )
         {
            gpo.current_supply -= gpo.total_reward_fund_steem;
            gpo.total_reward_fund_steem = ASSET( "100.000 TESTS" );
            gpo.current_supply += gpo.total_reward_fund_steem;
         });
      });

      generate_block();

      BOOST_REQUIRE( db->get_account( "bob" ).reward_steem_balance == ASSET( "0.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( "bob" ).reward_sbd_balance == ASSET( "0.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( "bob" ).reward_vesting_steem.amount + db->get_account( "sam" ).reward_vesting_steem.amount == db->get_comment( "alice", string( "test" ) ).beneficiary_payout_value.amount );
      BOOST_REQUIRE( ( db->get_account( "alice" ).reward_sbd_balance.amount + db->get_account( "alice" ).reward_vesting_steem.amount ) == db->get_account( "bob" ).reward_vesting_steem.amount + 1 );
      BOOST_REQUIRE( ( db->get_account( "alice" ).reward_sbd_balance.amount + db->get_account( "alice" ).reward_vesting_steem.amount ) * 2 == db->get_account( "sam" ).reward_vesting_steem.amount + 1 );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( claim_account_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: claim_account_validate" );

      claim_account_operation op;
      op.creator = "alice";
      op.fee = ASSET( "1.000 TESTS" );

      BOOST_TEST_MESSAGE( "--- Test failure with invalid account name" );
      op.creator = "aA0";
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Test failure with invalid fee symbol" );
      op.creator = "alice";
      op.fee = ASSET( "1.000 TBD" );
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Test failure with negative fee" );
      op.fee = ASSET( "-1.000 TESTS" );
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Test failure with non-zero extensions" );
      op.fee = ASSET( "1.000 TESTS" );
      op.extensions.insert( future_extensions( void_t() ) );
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Test success" );
      op.extensions.clear();
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( claim_account_authorities )
{
   try
   {
     BOOST_TEST_MESSAGE( "Testing: claim_account_authorities" );

      claim_account_operation op;
      op.creator = "alice";

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.insert( "alice" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.clear();
      auths.clear();
      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( claim_account_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: claim_account_apply" );

      ACTORS( (alice) )
      generate_block();

      fund( "alice", ASSET( "15.000 TESTS" ) );
      generate_block();

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_witness_schedule_object(), [&](witness_schedule_object& wso )
         {
            wso.median_props.account_creation_fee = ASSET( "20.000 TESTS" );
         });
      });
      generate_block();

      signed_transaction tx;
      claim_account_operation op;

      BOOST_TEST_MESSAGE( "--- Test failure when creator cannot cover fee" );
      op.creator = "alice";
      op.fee = ASSET( "20.000 TESTS" );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
      validate_database();


      // This test will be removed when soft forking for discount creation is implemented
      BOOST_TEST_MESSAGE( "--- Test failure covering witness fee" );

      generate_block();
      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_witness_schedule_object(), [&]( witness_schedule_object& wso )
         {
            wso.median_props.account_creation_fee = ASSET( "5.000 TESTS" );
         });
      });
      generate_block();

      op.fee = ASSET( "1.000 TESTS" );
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
      validate_database();


      BOOST_TEST_MESSAGE( "--- Test success claiming an account" );
      op.fee = ASSET( "5.000 TESTS" );
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      BOOST_REQUIRE( db->get_account( "alice" ).pending_claimed_accounts == 1 );
      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "10.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).balance == ASSET( "5.000 TESTS" ) );
      validate_database();


      BOOST_TEST_MESSAGE( "--- Test claiming from a non-existent account" );
      op.creator = "bob";
      tx.clear();
      tx.operations.push_back( op );
      BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();


      BOOST_TEST_MESSAGE( "--- Test success claiming a second account" );
      generate_block();
      op.creator = "alice";
      tx.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      BOOST_REQUIRE( db->get_account( "alice" ).pending_claimed_accounts == 2 );
      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "5.000 TESTS" ) );
      validate_database();


      BOOST_TEST_MESSAGE( "--- Test failure on claim overflow" );
      generate_block();
      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_account( "alice" ), [&]( account_object& a )
         {
            a.pending_claimed_accounts = std::numeric_limits< int64_t >::max();
         });
      });
      generate_block();

      tx.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( create_claimed_account_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: create_claimed_account_validate" );

      private_key_type priv_key = generate_private_key( "alice" );

      create_claimed_account_operation op;
      op.creator = "alice";
      op.new_account_name = "bob";
      op.owner = authority( 1, priv_key.get_public_key(), 1 );
      op.active = authority( 1, priv_key.get_public_key(), 1 );
      op.posting = authority( 1, priv_key.get_public_key(), 1 );
      op.memo_key = priv_key.get_public_key();

      BOOST_TEST_MESSAGE( "--- Test invalid creator name" );
      op.creator = "aA0";
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Test invalid new account name" );
      op.creator = "alice";
      op.new_account_name = "aA0";
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Test invalid account name in owner authority" );
      op.new_account_name = "bob";
      op.owner = authority( 1, "aA0", 1 );
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Test invalid account name in active authority" );
      op.owner = authority( 1, priv_key.get_public_key(), 1 );
      op.active = authority( 1, "aA0", 1 );
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Test invalid account name in posting authority" );
      op.active = authority( 1, priv_key.get_public_key(), 1 );
      op.posting = authority( 1, "aA0", 1 );
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Test invalid JSON metadata" );
      op.posting = authority( 1, priv_key.get_public_key(), 1 );
      op.json_metadata = "{\"foo\",\"bar\"}";
      BOOST_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "--- Test non UTF-8 JSON metadata" );
      op.json_metadata = "{\"foo\":\"\xa0\xa1\"}";
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Test failure with non-zero extensions" );
      op.json_metadata = "";
      op.extensions.insert( future_extensions( void_t() ) );
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Test success" );
      op.extensions.clear();
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( create_claimed_account_authorities )
{
   try
   {
     BOOST_TEST_MESSAGE( "Testing: create_claimed_account_authorities" );

      create_claimed_account_operation op;
      op.creator = "alice";

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.insert( "alice" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.clear();
      auths.clear();
      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( create_claimed_account_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: create_claimed_account_apply" );

      ACTORS( (alice) )
      vest( STEEM_TEMP_ACCOUNT, ASSET( "10.000 TESTS" ) );
      generate_block();

      signed_transaction tx;
      create_claimed_account_operation op;
      private_key_type priv_key = generate_private_key( "bob" );

      BOOST_TEST_MESSAGE( "--- Test failure when creator has not claimed an account" );
      op.creator = "alice";
      op.new_account_name = "bob";
      op.owner = authority( 1, priv_key.get_public_key(), 1 );
      op.active = authority( 2, priv_key.get_public_key(), 2 );
      op.posting = authority( 3, priv_key.get_public_key(), 3 );
      op.memo_key = priv_key.get_public_key();
      op.json_metadata = "{\"foo\":\"bar\"}";
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure creating account with non-existent account auth" );
      generate_block();
      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_account( "alice" ), [&]( account_object& a )
         {
            a.pending_claimed_accounts = 2;
         });
      });
      generate_block();
      op.owner = authority( 1, "bob", 1 );
      tx.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();


      BOOST_TEST_MESSAGE( "--- Test success creating claimed account" );
      op.owner = authority( 1, priv_key.get_public_key(), 1 );
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      const auto& bob = db->get_account( "bob" );
      const auto& bob_auth = db->get< account_authority_object, by_account >( "bob" );

      BOOST_REQUIRE( bob.name == "bob" );
      BOOST_REQUIRE( bob_auth.owner == authority( 1, priv_key.get_public_key(), 1 ) );
      BOOST_REQUIRE( bob_auth.active == authority( 2, priv_key.get_public_key(), 2 ) );
      BOOST_REQUIRE( bob_auth.posting == authority( 3, priv_key.get_public_key(), 3 ) );
      BOOST_REQUIRE( bob.memo_key == priv_key.get_public_key() );
#ifndef IS_LOW_MEM // json_metadata is not stored on low memory nodes
      BOOST_REQUIRE( bob.json_metadata == "{\"foo\":\"bar\"}" );
#endif
      BOOST_REQUIRE( bob.proxy == "" );
      BOOST_REQUIRE( bob.recovery_account == "alice" );
      BOOST_REQUIRE( bob.created == db->head_block_time() );
      BOOST_REQUIRE( bob.balance.amount.value == ASSET( "0.000 TESTS" ).amount.value );
      BOOST_REQUIRE( bob.sbd_balance.amount.value == ASSET( "0.000 TBD" ).amount.value );
      BOOST_REQUIRE( bob.vesting_shares.amount.value == ASSET( "0.000000 VESTS" ).amount.value );
      BOOST_REQUIRE( bob.id._id == bob_auth.id._id );

      BOOST_REQUIRE( db->get_account( "alice" ).pending_claimed_accounts == 1 );
      validate_database();


      BOOST_TEST_MESSAGE( "--- Test failure creating duplicate account name" );
      tx.signatures.clear();
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();


      BOOST_TEST_MESSAGE( "--- Test account creation with temp account does not set recovery account" );
      generate_block();
      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_account( STEEM_TEMP_ACCOUNT ), [&]( account_object& a )
         {
            a.pending_claimed_accounts = 1;
         });
      });
      generate_block();
      op.creator = STEEM_TEMP_ACCOUNT;
      op.new_account_name = "charlie";
      tx.clear();
      tx.operations.push_back( op );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "charlie" ).recovery_account == account_name_type() );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif
