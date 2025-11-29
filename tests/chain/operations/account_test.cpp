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

BOOST_AUTO_TEST_CASE( account_create_validate )
{
   try
   {

   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_create_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_create_authorities" );

      account_create_operation op;
      op.creator = "alice";
      op.new_account_name = "bob";

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      BOOST_TEST_MESSAGE( "--- Testing owner authority" );
      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      BOOST_TEST_MESSAGE( "--- Testing active authority" );
      expected.insert( "alice" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      BOOST_TEST_MESSAGE( "--- Testing posting authority" );
      expected.clear();
      auths.clear();
      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_create_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_create_apply" );

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );

      signed_transaction tx;
      private_key_type priv_key = generate_private_key( "alice" );

      const account_object& init = db->get_account( STEEM_GENESIS_WITNESS_NAME );
      asset init_starting_balance = init.balance;

      const auto& gpo = db->get_dynamic_global_properties();

      account_create_operation op;

      op.fee = asset( 100, STEEM_SYMBOL );
      op.new_account_name = "alice";
      op.creator = STEEM_GENESIS_WITNESS_NAME;
      op.owner = authority( 1, priv_key.get_public_key(), 1 );
      op.active = authority( 2, priv_key.get_public_key(), 2 );
      op.memo_key = priv_key.get_public_key();
      op.json_metadata = "{\"foo\":\"bar\"}";

      BOOST_TEST_MESSAGE( "--- Test normal account creation" );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( init_account_priv_key, db->get_chain_id() );
      tx.validate();
      db->push_transaction( tx, 0 );

      const account_object& acct = db->get_account( "alice" );
      const account_authority_object& acct_auth = db->get< account_authority_object, by_account >( "alice" );

      auto vest_shares = gpo.total_vesting_shares;
      auto vests = gpo.total_vesting_fund_steem;

      BOOST_REQUIRE( acct.name == "alice" );
      BOOST_REQUIRE( acct_auth.owner == authority( 1, priv_key.get_public_key(), 1 ) );
      BOOST_REQUIRE( acct_auth.active == authority( 2, priv_key.get_public_key(), 2 ) );
      BOOST_REQUIRE( acct.memo_key == priv_key.get_public_key() );
      BOOST_REQUIRE( acct.proxy == "" );
      BOOST_REQUIRE( acct.created == db->head_block_time() );
      BOOST_REQUIRE( acct.balance.amount.value == ASSET( "0.000 TESTS" ).amount.value );
      BOOST_REQUIRE( acct.sbd_balance.amount.value == ASSET( "0.000 TBD" ).amount.value );
      BOOST_REQUIRE( acct.id._id == acct_auth.id._id );

      /// because init_witness has created vesting shares and blocks have been produced, 100 STEEM is worth less than 100 vesting shares due to rounding
      BOOST_REQUIRE( acct.vesting_shares.amount.value == ( op.fee * ( vest_shares / vests ) ).amount.value );
      BOOST_REQUIRE( acct.vesting_withdraw_rate.amount.value == ASSET( "0.000000 VESTS" ).amount.value );
      BOOST_REQUIRE( acct.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( ( init_starting_balance - ASSET( "0.100 TESTS" ) ).amount.value == init.balance.amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure of duplicate account creation" );
      BOOST_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );

      BOOST_REQUIRE( acct.name == "alice" );
      BOOST_REQUIRE( acct_auth.owner == authority( 1, priv_key.get_public_key(), 1 ) );
      BOOST_REQUIRE( acct_auth.active == authority( 2, priv_key.get_public_key(), 2 ) );
      BOOST_REQUIRE( acct.memo_key == priv_key.get_public_key() );
      BOOST_REQUIRE( acct.proxy == "" );
      BOOST_REQUIRE( acct.created == db->head_block_time() );
      BOOST_REQUIRE( acct.balance.amount.value == ASSET( "0.000 TESTS " ).amount.value );
      BOOST_REQUIRE( acct.sbd_balance.amount.value == ASSET( "0.000 TBD" ).amount.value );
      BOOST_REQUIRE( acct.vesting_shares.amount.value == ( op.fee * ( vest_shares / vests ) ).amount.value );
      BOOST_REQUIRE( acct.vesting_withdraw_rate.amount.value == ASSET( "0.000000 VESTS" ).amount.value );
      BOOST_REQUIRE( acct.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( ( init_starting_balance - ASSET( "0.100 TESTS" ) ).amount.value == init.balance.amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when creator cannot cover fee" );
      tx.signatures.clear();
      tx.operations.clear();
      op.fee = asset( db->get_account( STEEM_GENESIS_WITNESS_NAME ).balance.amount + 1, STEEM_SYMBOL );
      op.new_account_name = "bob";
      tx.operations.push_back( op );
      tx.sign( init_account_priv_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure covering witness fee" );
      generate_block();
      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_witness_schedule_object(), [&]( witness_schedule_object& wso )
         {
            wso.median_props.account_creation_fee = ASSET( "10.000 TESTS" );
         });
      });
      generate_block();

      tx.clear();
      op.fee = ASSET( "1.000 TESTS" );
      tx.operations.push_back( op );
      tx.sign( init_account_priv_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test account creation with temp account does not set recovery account" );
      fund( STEEM_TEMP_ACCOUNT, ASSET( "310.000 TESTS" ) );
      vest( STEEM_TEMP_ACCOUNT, ASSET( "10.000 TESTS" ) );
      op.creator = STEEM_TEMP_ACCOUNT;
      op.fee = ASSET( "300.000 TESTS" );
      op.new_account_name = "bob";
      tx.clear();
      tx.operations.push_back( op );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "bob" ).recovery_account == account_name_type() );
      validate_database();

   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_update_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_update_validate" );

      ACTORS( (alice) )

      account_update_operation op;
      op.account = "alice";
      op.posting = authority();
      op.posting->weight_threshold = 1;
      op.posting->add_authorities( "abcdefghijklmnopq", 1 );

      try
      {
         op.validate();

         signed_transaction tx;
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         tx.operations.push_back( op );
         tx.sign( alice_private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );

         BOOST_FAIL( "An exception was not thrown for an invalid account name" );
      }
      catch( fc::exception& ) {}

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_update_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_update_authorities" );

      ACTORS( (alice)(bob) )
      private_key_type active_key = generate_private_key( "new_key" );

      db->modify( db->get< account_authority_object, by_account >( "alice" ), [&]( account_authority_object& a )
      {
         a.active = authority( 1, active_key.get_public_key(), 1 );
      });

      account_update_operation op;
      op.account = "alice";
      op.json_metadata = "{\"success\":true}";

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );

      BOOST_TEST_MESSAGE( "  Tests when owner authority is not updated ---" );
      BOOST_TEST_MESSAGE( "--- Test failure when no signature" );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when wrong signature" );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when containing additional incorrect signature" );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when containing duplicate signatures" );
      tx.signatures.clear();
      tx.sign( active_key, db->get_chain_id() );
      tx.sign( active_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test success on active key" );
      tx.signatures.clear();
      tx.sign( active_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- Test success on owner key alone" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, database::skip_transaction_dupe_check );

      BOOST_TEST_MESSAGE( "  Tests when owner authority is updated ---" );
      BOOST_TEST_MESSAGE( "--- Test failure when updating the owner authority with an active key" );
      tx.signatures.clear();
      tx.operations.clear();
      op.owner = authority( 1, active_key.get_public_key(), 1 );
      tx.operations.push_back( op );
      tx.sign( active_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_owner_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when owner key and active key are present" );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when incorrect signature" );
      tx.signatures.clear();
      tx.sign( alice_post_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0), tx_missing_owner_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when duplicate owner keys are present" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test success when updating the owner authority with an owner key" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_update_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_update_apply" );

      ACTORS( (alice) )
      private_key_type new_private_key = generate_private_key( "new_key" );

      BOOST_TEST_MESSAGE( "--- Test normal update" );

      account_update_operation op;
      op.account = "alice";
      op.owner = authority( 1, new_private_key.get_public_key(), 1 );
      op.active = authority( 2, new_private_key.get_public_key(), 2 );
      op.memo_key = new_private_key.get_public_key();
      op.json_metadata = "{\"bar\":\"foo\"}";

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      const account_object& acct = db->get_account( "alice" );
      const account_authority_object& acct_auth = db->get< account_authority_object, by_account >( "alice" );

      BOOST_REQUIRE( acct.name == "alice" );
      BOOST_REQUIRE( acct_auth.owner == authority( 1, new_private_key.get_public_key(), 1 ) );
      BOOST_REQUIRE( acct_auth.active == authority( 2, new_private_key.get_public_key(), 2 ) );
      BOOST_REQUIRE( acct.memo_key == new_private_key.get_public_key() );

      /* This is being moved out of consensus
      #ifndef IS_LOW_MEM
         BOOST_REQUIRE( acct.json_metadata == "{\"bar\":\"foo\"}" );
      #else
         BOOST_REQUIRE( acct.json_metadata == "" );
      #endif
      */

      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when updating a non-existent account" );
      tx.operations.clear();
      tx.signatures.clear();
      op.account = "bob";
      tx.operations.push_back( op );
      tx.sign( new_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception )
      validate_database();


      BOOST_TEST_MESSAGE( "--- Test failure when account authority does not exist" );
      tx.clear();
      op = account_update_operation();
      op.account = "alice";
      op.posting = authority();
      op.posting->weight_threshold = 1;
      op.posting->add_authorities( "dave", 1 );
      tx.operations.push_back( op );
      tx.sign( new_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_witness_vote_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_witness_vote_validate" );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_witness_vote_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_witness_vote_authorities" );

      ACTORS( (alice)(bob)(sam) )

      fund( "alice", 1000 );
      private_key_type alice_witness_key = generate_private_key( "alice_witness" );
      witness_create( "alice", alice_private_key, "foo.bar", alice_witness_key.get_public_key(), 1000 );

      account_witness_vote_operation op;
      op.account = "bob";
      op.witness = "alice";

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );

      BOOST_TEST_MESSAGE( "--- Test failure when no signatures" );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by a signature not in the account's authority" );
      tx.sign( bob_post_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when duplicate signatures" );
      tx.signatures.clear();
      tx.sign( bob_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by an additional signature not in the creator's authority" );
      tx.signatures.clear();
      tx.sign( bob_private_key, db->get_chain_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test success with witness signature" );
      tx.signatures.clear();
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- Test failure with proxy signature" );
      proxy( "bob", "sam" );
      tx.signatures.clear();
      tx.sign( sam_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_witness_vote_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_witness_vote_apply" );

      ACTORS( (alice)(bob)(sam) )
      fund( "alice" , 5000 );
      vest( "alice", 5000 );
      fund( "sam", 1000 );

      private_key_type sam_witness_key = generate_private_key( "sam_key" );
      witness_create( "sam", sam_private_key, "foo.bar", sam_witness_key.get_public_key(), 1000 );
      const witness_object& sam_witness = db->get_witness( "sam" );

      const auto& witness_vote_idx = db->get_index< witness_vote_index >().indices().get< by_witness_account >();

      BOOST_TEST_MESSAGE( "--- Test normal vote" );
      account_witness_vote_operation op;
      op.account = "alice";
      op.witness = "sam";
      op.approve = true;

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( sam_witness.votes == alice.vesting_shares.amount );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, alice.name ) ) != witness_vote_idx.end() );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test revoke vote" );
      op.approve = false;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );
      BOOST_REQUIRE( sam_witness.votes.value == 0 );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, alice.name ) ) == witness_vote_idx.end() );

      BOOST_TEST_MESSAGE( "--- Test failure when attempting to revoke a non-existent vote" );

      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );
      BOOST_REQUIRE( sam_witness.votes.value == 0 );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, alice.name ) ) == witness_vote_idx.end() );

      BOOST_TEST_MESSAGE( "--- Test proxied vote" );
      proxy( "alice", "bob" );
      tx.operations.clear();
      tx.signatures.clear();
      op.approve = true;
      op.account = "bob";
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( sam_witness.votes == ( bob.proxied_vsf_votes_total() + bob.vesting_shares.amount ) );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, bob.name ) ) != witness_vote_idx.end() );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, alice.name ) ) == witness_vote_idx.end() );

      BOOST_TEST_MESSAGE( "--- Test vote from a proxied account" );
      tx.operations.clear();
      tx.signatures.clear();
      op.account = "alice";
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );

      BOOST_REQUIRE( sam_witness.votes == ( bob.proxied_vsf_votes_total() + bob.vesting_shares.amount ) );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, bob.name ) ) != witness_vote_idx.end() );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, alice.name ) ) == witness_vote_idx.end() );

      BOOST_TEST_MESSAGE( "--- Test revoke proxied vote" );
      tx.operations.clear();
      tx.signatures.clear();
      op.account = "bob";
      op.approve = false;
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( sam_witness.votes.value == 0 );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, bob.name ) ) == witness_vote_idx.end() );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, alice.name ) ) == witness_vote_idx.end() );

      BOOST_TEST_MESSAGE( "--- Test failure when voting for a non-existent account" );
      tx.operations.clear();
      tx.signatures.clear();
      op.witness = "dave";
      op.approve = true;
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );

      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when voting for an account that is not a witness" );
      tx.operations.clear();
      tx.signatures.clear();
      op.witness = "alice";
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );

      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_witness_proxy_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_witness_proxy_validate" );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_witness_proxy_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_witness_proxy_authorities" );

      ACTORS( (alice)(bob) )

      account_witness_proxy_operation op;
      op.account = "bob";
      op.proxy = "alice";

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );

      BOOST_TEST_MESSAGE( "--- Test failure when no signatures" );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by a signature not in the account's authority" );
      tx.sign( bob_post_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when duplicate signatures" );
      tx.signatures.clear();
      tx.sign( bob_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by an additional signature not in the creator's authority" );
      tx.signatures.clear();
      tx.sign( bob_private_key, db->get_chain_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test success with witness signature" );
      tx.signatures.clear();
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- Test failure with proxy signature" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_witness_proxy_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_witness_proxy_apply" );

      ACTORS( (alice)(bob)(sam)(dave) )
      fund( "alice", 1000 );
      vest( "alice", 1000 );
      fund( "bob", 3000 );
      vest( "bob", 3000 );
      fund( "sam", 5000 );
      vest( "sam", 5000 );
      fund( "dave", 7000 );
      vest( "dave", 7000 );

      BOOST_TEST_MESSAGE( "--- Test setting proxy to another account from self." );
      // bob -> alice

      account_witness_proxy_operation op;
      op.account = "bob";
      op.proxy = "alice";

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( bob_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( bob.proxy == "alice" );
      BOOST_REQUIRE( bob.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( alice.proxy == STEEM_PROXY_TO_SELF_ACCOUNT );
      BOOST_REQUIRE( alice.proxied_vsf_votes_total() == bob.vesting_shares.amount );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test changing proxy" );
      // bob->sam

      tx.operations.clear();
      tx.signatures.clear();
      op.proxy = "sam";
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( bob.proxy == "sam" );
      BOOST_REQUIRE( bob.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( alice.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( sam.proxy == STEEM_PROXY_TO_SELF_ACCOUNT );
      BOOST_REQUIRE( sam.proxied_vsf_votes_total().value == bob.vesting_shares.amount );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when changing proxy to existing proxy" );

      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );

      BOOST_REQUIRE( bob.proxy == "sam" );
      BOOST_REQUIRE( bob.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( sam.proxy == STEEM_PROXY_TO_SELF_ACCOUNT );
      BOOST_REQUIRE( sam.proxied_vsf_votes_total() == bob.vesting_shares.amount );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test adding a grandparent proxy" );
      // bob->sam->dave

      tx.operations.clear();
      tx.signatures.clear();
      op.proxy = "dave";
      op.account = "sam";
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( bob.proxy == "sam" );
      BOOST_REQUIRE( bob.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( sam.proxy == "dave" );
      BOOST_REQUIRE( sam.proxied_vsf_votes_total() == bob.vesting_shares.amount );
      BOOST_REQUIRE( dave.proxy == STEEM_PROXY_TO_SELF_ACCOUNT );
      BOOST_REQUIRE( dave.proxied_vsf_votes_total() == ( sam.vesting_shares + bob.vesting_shares ).amount );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test adding a grandchild proxy" );
      //       alice
      //         |
      // bob->  sam->dave

      tx.operations.clear();
      tx.signatures.clear();
      op.proxy = "sam";
      op.account = "alice";
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( alice.proxy == "sam" );
      BOOST_REQUIRE( alice.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( bob.proxy == "sam" );
      BOOST_REQUIRE( bob.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( sam.proxy == "dave" );
      BOOST_REQUIRE( sam.proxied_vsf_votes_total() == ( bob.vesting_shares + alice.vesting_shares ).amount );
      BOOST_REQUIRE( dave.proxy == STEEM_PROXY_TO_SELF_ACCOUNT );
      BOOST_REQUIRE( dave.proxied_vsf_votes_total() == ( sam.vesting_shares + bob.vesting_shares + alice.vesting_shares ).amount );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test removing a grandchild proxy" );
      // alice->sam->dave

      tx.operations.clear();
      tx.signatures.clear();
      op.proxy = STEEM_PROXY_TO_SELF_ACCOUNT;
      op.account = "bob";
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( alice.proxy == "sam" );
      BOOST_REQUIRE( alice.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( bob.proxy == STEEM_PROXY_TO_SELF_ACCOUNT );
      BOOST_REQUIRE( bob.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( sam.proxy == "dave" );
      BOOST_REQUIRE( sam.proxied_vsf_votes_total() == alice.vesting_shares.amount );
      BOOST_REQUIRE( dave.proxy == STEEM_PROXY_TO_SELF_ACCOUNT );
      BOOST_REQUIRE( dave.proxied_vsf_votes_total() == ( sam.vesting_shares + alice.vesting_shares ).amount );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test votes are transferred when a proxy is added" );
      account_witness_vote_operation vote;
      vote.account= "bob";
      vote.witness = STEEM_GENESIS_WITNESS_NAME;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( vote );
      tx.sign( bob_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      op.account = "alice";
      op.proxy = "bob";
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_witness( STEEM_GENESIS_WITNESS_NAME ).votes == ( alice.vesting_shares + bob.vesting_shares ).amount );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test votes are removed when a proxy is removed" );
      op.proxy = STEEM_PROXY_TO_SELF_ACCOUNT;
      tx.signatures.clear();
      tx.operations.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_witness( STEEM_GENESIS_WITNESS_NAME ).votes == bob.vesting_shares.amount );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( account_create_with_delegation_authorities )
{
   try
   {
     BOOST_TEST_MESSAGE( "Testing: account_create_with_delegation_authorities" );

      account_create_with_delegation_operation op;
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

BOOST_AUTO_TEST_CASE( account_create_with_delegation_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_create_with_delegation_apply" );
      signed_transaction tx;
      ACTORS( (alice) );
      // 150 * fee = ( 5 * STEEM ) + SP
      //auto gpo = db->get_dynamic_global_properties();
      generate_blocks(1);
      fund( "alice", ASSET("1510.000 TESTS") );
      vest( "alice", ASSET("1000.000 TESTS") );

      private_key_type priv_key = generate_private_key( "temp_key" );

      generate_block();

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_witness_schedule_object(), [&]( witness_schedule_object& w )
         {
            w.median_props.account_creation_fee = ASSET( "1.000 TESTS" );
         });
      });

      generate_block();

      // This test passed pre HF20
      BOOST_TEST_MESSAGE( "--- Test deprecation. " );
      account_create_with_delegation_operation op;
      op.fee = ASSET( "10.000 TESTS" );
      op.delegation = ASSET( "100000000.000000 VESTS" );
      op.creator = "alice";
      op.new_account_name = "bob";
      op.owner = authority( 1, priv_key.get_public_key(), 1 );
      op.active = authority( 2, priv_key.get_public_key(), 2 );
      op.memo_key = priv_key.get_public_key();
      op.json_metadata = "{\"foo\":\"bar\"}";
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( post_rate_limit )
{
   try
   {
      ACTORS( (alice) )

      fund( "alice", 10000 );
      vest( "alice", 10000 );

      comment_operation op;
      op.author = "alice";
      op.permlink = "test1";
      op.parent_author = "";
      op.parent_permlink = "test";
      op.body = "test";

      signed_transaction tx;

      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_comment( "alice", string( "test1" ) ).reward_weight == STEEM_100_PERCENT );

      tx.operations.clear();
      tx.signatures.clear();

      generate_blocks( db->head_block_time() + STEEM_MIN_ROOT_COMMENT_INTERVAL + fc::seconds( STEEM_BLOCK_INTERVAL ), true );

      op.permlink = "test2";

      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_comment( "alice", string( "test2" ) ).reward_weight == STEEM_100_PERCENT );

      generate_blocks( db->head_block_time() + STEEM_MIN_ROOT_COMMENT_INTERVAL + fc::seconds( STEEM_BLOCK_INTERVAL ), true );

      tx.operations.clear();
      tx.signatures.clear();

      op.permlink = "test3";

      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_comment( "alice", string( "test3" ) ).reward_weight == STEEM_100_PERCENT );

      generate_blocks( db->head_block_time() + STEEM_MIN_ROOT_COMMENT_INTERVAL + fc::seconds( STEEM_BLOCK_INTERVAL ), true );

      tx.operations.clear();
      tx.signatures.clear();

      op.permlink = "test4";

      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_comment( "alice", string( "test4" ) ).reward_weight == STEEM_100_PERCENT );

      generate_blocks( db->head_block_time() + STEEM_MIN_ROOT_COMMENT_INTERVAL + fc::seconds( STEEM_BLOCK_INTERVAL ), true );

      tx.operations.clear();
      tx.signatures.clear();

      op.permlink = "test5";

      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_comment( "alice", string( "test5" ) ).reward_weight == STEEM_100_PERCENT );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( clear_null_account )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing clearing the null account's balances on block" );

      ACTORS( (alice) );
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TESTS" ) ) );

      fund( "alice", ASSET( "10.000 TESTS" ) );
      fund( "alice", ASSET( "10.000 TBD" ) );

      transfer_operation transfer1;
      transfer1.from = "alice";
      transfer1.to = STEEM_NULL_ACCOUNT;
      transfer1.amount = ASSET( "1.000 TESTS" );

      transfer_operation transfer2;
      transfer2.from = "alice";
      transfer2.to = STEEM_NULL_ACCOUNT;
      transfer2.amount = ASSET( "2.000 TBD" );

      transfer_to_vesting_operation vest;
      vest.from = "alice";
      vest.to = STEEM_NULL_ACCOUNT;
      vest.amount = ASSET( "3.000 TESTS" );

      transfer_to_savings_operation save1;
      save1.from = "alice";
      save1.to = STEEM_NULL_ACCOUNT;
      save1.amount = ASSET( "4.000 TESTS" );

      transfer_to_savings_operation save2;
      save2.from = "alice";
      save2.to = STEEM_NULL_ACCOUNT;
      save2.amount = ASSET( "5.000 TBD" );

      BOOST_TEST_MESSAGE( "--- Transferring to NULL Account" );

      signed_transaction tx;
      tx.operations.push_back( transfer1 );
      tx.operations.push_back( transfer2 );
      tx.operations.push_back( vest );
      tx.operations.push_back( save1);
      tx.operations.push_back( save2 );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      validate_database();

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_account( STEEM_NULL_ACCOUNT ), [&]( account_object& a )
         {
            a.reward_steem_balance = ASSET( "1.000 TESTS" );
            a.reward_sbd_balance = ASSET( "1.000 TBD" );
            a.reward_vesting_balance = ASSET( "1.000000 VESTS" );
            a.reward_vesting_steem = ASSET( "1.000 TESTS" );
         });

         db.modify( db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo )
         {
            gpo.current_supply += ASSET( "2.000 TESTS" );
            gpo.virtual_supply += ASSET( "3.000 TESTS" );
            gpo.current_sbd_supply += ASSET( "1.000 TBD" );
            gpo.pending_rewarded_vesting_shares += ASSET( "1.000000 VESTS" );
            gpo.pending_rewarded_vesting_steem += ASSET( "1.000 TESTS" );
         });
      });

      validate_database();

      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).balance == ASSET( "1.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).sbd_balance == ASSET( "2.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).vesting_shares > ASSET( "0.000000 VESTS" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).savings_balance == ASSET( "4.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).savings_sbd_balance == ASSET( "5.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).reward_sbd_balance == ASSET( "1.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).reward_steem_balance == ASSET( "1.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).reward_vesting_balance == ASSET( "1.000000 VESTS" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).reward_vesting_steem == ASSET( "1.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "2.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).sbd_balance == ASSET( "3.000 TBD" ) );

      BOOST_TEST_MESSAGE( "--- Generating block to clear balances" );
      generate_block();
      validate_database();

      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).balance == ASSET( "0.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).sbd_balance == ASSET( "0.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).vesting_shares == ASSET( "0.000000 VESTS" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).savings_balance == ASSET( "0.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).savings_sbd_balance == ASSET( "0.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).reward_sbd_balance == ASSET( "0.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).reward_steem_balance == ASSET( "0.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).reward_vesting_balance == ASSET( "0.000000 VESTS" ) );
      BOOST_REQUIRE( db->get_account( STEEM_NULL_ACCOUNT ).reward_vesting_steem == ASSET( "0.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "2.000 TESTS" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).sbd_balance == ASSET( "3.000 TBD" ) );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif
