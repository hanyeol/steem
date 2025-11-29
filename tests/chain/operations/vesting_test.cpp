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

BOOST_AUTO_TEST_CASE( transfer_to_vesting_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: transfer_to_vesting_validate" );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( transfer_to_vesting_authorities )
{
   try
   {
      ACTORS( (alice)(bob) )
      fund( "alice", 10000 );

      BOOST_TEST_MESSAGE( "Testing: transfer_to_vesting_authorities" );

      transfer_to_vesting_operation op;
      op.from = "alice";
      op.to = "bob";
      op.amount = ASSET( "2.500 TESTS" );

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );

      BOOST_TEST_MESSAGE( "--- Test failure when no signatures" );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by a signature not in the account's authority" );
      tx.sign( alice_post_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when duplicate signatures" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by an additional signature not in the creator's authority" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test success with from signature" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( transfer_to_vesting_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: transfer_to_vesting_apply" );

      ACTORS( (alice)(bob) )
      fund( "alice", 10000 );

      const auto& gpo = db->get_dynamic_global_properties();

      BOOST_REQUIRE( alice.balance == ASSET( "10.000 TESTS" ) );

      auto shares = asset( gpo.total_vesting_shares.amount, VESTS_SYMBOL );
      auto vests = asset( gpo.total_vesting_fund_steem.amount, STEEM_SYMBOL );
      auto alice_shares = alice.vesting_shares;
      auto bob_shares = bob.vesting_shares;

      transfer_to_vesting_operation op;
      op.from = "alice";
      op.to = "";
      op.amount = ASSET( "7.500 TESTS" );

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      auto new_vest = op.amount * ( shares / vests );
      shares += new_vest;
      vests += op.amount;
      alice_shares += new_vest;

      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "2.500 TESTS" ).amount.value );
      BOOST_REQUIRE( alice.vesting_shares.amount.value == alice_shares.amount.value );
      BOOST_REQUIRE( gpo.total_vesting_fund_steem.amount.value == vests.amount.value );
      BOOST_REQUIRE( gpo.total_vesting_shares.amount.value == shares.amount.value );
      validate_database();

      op.to = "bob";
      op.amount = asset( 2000, STEEM_SYMBOL );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      new_vest = asset( ( op.amount * ( shares / vests ) ).amount, VESTS_SYMBOL );
      shares += new_vest;
      vests += op.amount;
      bob_shares += new_vest;

      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "0.500 TESTS" ).amount.value );
      BOOST_REQUIRE( alice.vesting_shares.amount.value == alice_shares.amount.value );
      BOOST_REQUIRE( bob.balance.amount.value == ASSET( "0.000 TESTS" ).amount.value );
      BOOST_REQUIRE( bob.vesting_shares.amount.value == bob_shares.amount.value );
      BOOST_REQUIRE( gpo.total_vesting_fund_steem.amount.value == vests.amount.value );
      BOOST_REQUIRE( gpo.total_vesting_shares.amount.value == shares.amount.value );
      validate_database();

      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );

      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "0.500 TESTS" ).amount.value );
      BOOST_REQUIRE( alice.vesting_shares.amount.value == alice_shares.amount.value );
      BOOST_REQUIRE( bob.balance.amount.value == ASSET( "0.000 TESTS" ).amount.value );
      BOOST_REQUIRE( bob.vesting_shares.amount.value == bob_shares.amount.value );
      BOOST_REQUIRE( gpo.total_vesting_fund_steem.amount.value == vests.amount.value );
      BOOST_REQUIRE( gpo.total_vesting_shares.amount.value == shares.amount.value );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( withdraw_vesting_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: withdraw_vesting_validate" );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( withdraw_vesting_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: withdraw_vesting_authorities" );

      ACTORS( (alice)(bob) )
      fund( "alice", 10000 );
      vest( "alice", 10000 );

      withdraw_vesting_operation op;
      op.account = "alice";
      op.vesting_shares = ASSET( "0.001000 VESTS" );

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );

      BOOST_TEST_MESSAGE( "--- Test failure when no signature." );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test success with account signature" );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, database::skip_transaction_dupe_check );

      BOOST_TEST_MESSAGE( "--- Test failure with duplicate signature" );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure with additional incorrect signature" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test failure with incorrect signature" );
      tx.signatures.clear();
      tx.sign( alice_post_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( withdraw_vesting_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: withdraw_vesting_apply" );

      ACTORS( (alice)(bob) )
      generate_block();
      vest( "alice", ASSET( "10.000 TESTS" ) );

      BOOST_TEST_MESSAGE( "--- Test failure withdrawing negative VESTS" );

      {
      const auto& alice = db->get_account( "alice" );

      withdraw_vesting_operation op;
      op.account = "alice";
      op.vesting_shares = asset( -1, VESTS_SYMBOL );

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Test withdraw of existing VESTS" );
      op.vesting_shares = asset( alice.vesting_shares.amount / 2, VESTS_SYMBOL );

      auto old_vesting_shares = alice.vesting_shares;

      tx.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( alice.vesting_shares.amount.value == old_vesting_shares.amount.value );
      BOOST_REQUIRE( alice.vesting_withdraw_rate.amount.value == ( old_vesting_shares.amount / ( STEEM_VESTING_WITHDRAW_INTERVALS * 2 ) ).value );
      BOOST_REQUIRE( alice.to_withdraw.value == op.vesting_shares.amount.value );
      BOOST_REQUIRE( alice.next_vesting_withdrawal == db->head_block_time() + STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test changing vesting withdrawal" );
      tx.operations.clear();
      tx.signatures.clear();

      op.vesting_shares = asset( alice.vesting_shares.amount / 3, VESTS_SYMBOL );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( alice.vesting_shares.amount.value == old_vesting_shares.amount.value );
      BOOST_REQUIRE( alice.vesting_withdraw_rate.amount.value == ( old_vesting_shares.amount / ( STEEM_VESTING_WITHDRAW_INTERVALS * 3 ) ).value );
      BOOST_REQUIRE( alice.to_withdraw.value == op.vesting_shares.amount.value );
      BOOST_REQUIRE( alice.next_vesting_withdrawal == db->head_block_time() + STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test withdrawing more vests than available" );
      //auto old_withdraw_amount = alice.to_withdraw;
      tx.operations.clear();
      tx.signatures.clear();

      op.vesting_shares = asset( alice.vesting_shares.amount * 2, VESTS_SYMBOL );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( alice.vesting_shares.amount.value == old_vesting_shares.amount.value );
      BOOST_REQUIRE( alice.vesting_withdraw_rate.amount.value == ( old_vesting_shares.amount / ( STEEM_VESTING_WITHDRAW_INTERVALS * 3 ) ).value );
      BOOST_REQUIRE( alice.next_vesting_withdrawal == db->head_block_time() + STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test withdrawing 0 to reset vesting withdraw" );
      tx.operations.clear();
      tx.signatures.clear();

      op.vesting_shares = asset( 0, VESTS_SYMBOL );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( alice.vesting_shares.amount.value == old_vesting_shares.amount.value );
      BOOST_REQUIRE( alice.vesting_withdraw_rate.amount.value == 0 );
      BOOST_REQUIRE( alice.to_withdraw.value == 0 );
      BOOST_REQUIRE( alice.next_vesting_withdrawal == fc::time_point_sec::maximum() );


      BOOST_TEST_MESSAGE( "--- Test cancelling a withdraw when below the account creation fee" );
      op.vesting_shares = alice.vesting_shares;
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      generate_block();
      }

      db_plugin->debug_update( [=]( database& db )
      {
         auto& wso = db.get_witness_schedule_object();

         db.modify( wso, [&]( witness_schedule_object& w )
         {
            w.median_props.account_creation_fee = ASSET( "10.000 TESTS" );
         });

         db.modify( db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo )
         {
            gpo.current_supply += wso.median_props.account_creation_fee - ASSET( "0.001 TESTS" ) - gpo.total_vesting_fund_steem;
            gpo.total_vesting_fund_steem = wso.median_props.account_creation_fee - ASSET( "0.001 TESTS" );
         });

         db.update_virtual_supply();
      }, database::skip_witness_signature );

      withdraw_vesting_operation op;
      signed_transaction tx;
      op.account = "alice";
      op.vesting_shares = ASSET( "0.000000 VESTS" );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).vesting_withdraw_rate == ASSET( "0.000000 VESTS" ) );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test withdrawing minimal VESTS" );
      op.account = "bob";
      op.vesting_shares = db->get_account( "bob" ).vesting_shares;
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 ); // We do not need to test the result of this, simply that it works.
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( vesting_withdrawals )
{
   try
   {
      ACTORS( (alice) )
      fund( "alice", 100000 );
      vest( "alice", 100000 );

      const auto& new_alice = db->get_account( "alice" );

      BOOST_TEST_MESSAGE( "Setting up withdrawal" );

      signed_transaction tx;
      withdraw_vesting_operation op;
      op.account = "alice";
      op.vesting_shares = asset( new_alice.vesting_shares.amount / 2, VESTS_SYMBOL );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      auto next_withdrawal = db->head_block_time() + STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS;
      asset vesting_shares = new_alice.vesting_shares;
      asset to_withdraw = op.vesting_shares;
      asset original_vesting = vesting_shares;
      asset withdraw_rate = new_alice.vesting_withdraw_rate;

      BOOST_TEST_MESSAGE( "Generating block up to first withdrawal" );
      generate_blocks( next_withdrawal - ( STEEM_BLOCK_INTERVAL / 2 ), true);

      BOOST_REQUIRE( db->get_account( "alice" ).vesting_shares.amount.value == vesting_shares.amount.value );

      BOOST_TEST_MESSAGE( "Generating block to cause withdrawal" );
      generate_block();

      auto fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();
      auto gpo = db->get_dynamic_global_properties();

      BOOST_REQUIRE( db->get_account( "alice" ).vesting_shares.amount.value == ( vesting_shares - withdraw_rate ).amount.value );
      BOOST_REQUIRE( ( withdraw_rate * gpo.get_vesting_share_price() ).amount.value - db->get_account( "alice" ).balance.amount.value <= 1 ); // Check a range due to differences in the share price
      BOOST_REQUIRE( fill_op.from_account == "alice" );
      BOOST_REQUIRE( fill_op.to_account == "alice" );
      BOOST_REQUIRE( fill_op.withdrawn.amount.value == withdraw_rate.amount.value );
      BOOST_REQUIRE( std::abs( ( fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price() ).amount.value ) <= 1 );
      validate_database();

      BOOST_TEST_MESSAGE( "Generating the rest of the blocks in the withdrawal" );

      vesting_shares = db->get_account( "alice" ).vesting_shares;
      auto balance = db->get_account( "alice" ).balance;
      auto old_next_vesting = db->get_account( "alice" ).next_vesting_withdrawal;

      for( int i = 1; i < STEEM_VESTING_WITHDRAW_INTERVALS - 1; i++ )
      {
         generate_blocks( db->head_block_time() + STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS );

         const auto& alice = db->get_account( "alice" );

         gpo = db->get_dynamic_global_properties();
         fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();

         BOOST_REQUIRE( alice.vesting_shares.amount.value == ( vesting_shares - withdraw_rate ).amount.value );
         BOOST_REQUIRE( balance.amount.value + ( withdraw_rate * gpo.get_vesting_share_price() ).amount.value - alice.balance.amount.value <= 1 );
         BOOST_REQUIRE( fill_op.from_account == "alice" );
         BOOST_REQUIRE( fill_op.to_account == "alice" );
         BOOST_REQUIRE( fill_op.withdrawn.amount.value == withdraw_rate.amount.value );
         BOOST_REQUIRE( std::abs( ( fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price() ).amount.value ) <= 1 );

         if ( i == STEEM_VESTING_WITHDRAW_INTERVALS - 1 )
            BOOST_REQUIRE( alice.next_vesting_withdrawal == fc::time_point_sec::maximum() );
         else
            BOOST_REQUIRE( alice.next_vesting_withdrawal.sec_since_epoch() == ( old_next_vesting + STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS ).sec_since_epoch() );

         validate_database();

         vesting_shares = alice.vesting_shares;
         balance = alice.balance;
         old_next_vesting = alice.next_vesting_withdrawal;
      }

      if (  to_withdraw.amount.value % withdraw_rate.amount.value != 0 )
      {
         BOOST_TEST_MESSAGE( "Generating one more block to take care of remainder" );
         generate_blocks( db->head_block_time() + STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS, true );
         fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();
         gpo = db->get_dynamic_global_properties();

         BOOST_REQUIRE( db->get_account( "alice" ).next_vesting_withdrawal.sec_since_epoch() == ( old_next_vesting + STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS ).sec_since_epoch() );
         BOOST_REQUIRE( fill_op.from_account == "alice" );
         BOOST_REQUIRE( fill_op.to_account == "alice" );
         BOOST_REQUIRE( fill_op.withdrawn.amount.value == withdraw_rate.amount.value );
         BOOST_REQUIRE( std::abs( ( fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price() ).amount.value ) <= 1 );

         generate_blocks( db->head_block_time() + STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS, true );
         gpo = db->get_dynamic_global_properties();
         fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();

         BOOST_REQUIRE( db->get_account( "alice" ).next_vesting_withdrawal.sec_since_epoch() == fc::time_point_sec::maximum().sec_since_epoch() );
         BOOST_REQUIRE( fill_op.to_account == "alice" );
         BOOST_REQUIRE( fill_op.from_account == "alice" );
         BOOST_REQUIRE( fill_op.withdrawn.amount.value == to_withdraw.amount.value % withdraw_rate.amount.value );
         BOOST_REQUIRE( std::abs( ( fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price() ).amount.value ) <= 1 );

         validate_database();
      }
      else
      {
         generate_blocks( db->head_block_time() + STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS, true );

         BOOST_REQUIRE( db->get_account( "alice" ).next_vesting_withdrawal.sec_since_epoch() == fc::time_point_sec::maximum().sec_since_epoch() );

         fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();
         BOOST_REQUIRE( fill_op.from_account == "alice" );
         BOOST_REQUIRE( fill_op.to_account == "alice" );
         BOOST_REQUIRE( fill_op.withdrawn.amount.value == withdraw_rate.amount.value );
         BOOST_REQUIRE( std::abs( ( fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price() ).amount.value ) <= 1 );
      }

      BOOST_REQUIRE( db->get_account( "alice" ).vesting_shares.amount.value == ( original_vesting - op.vesting_shares ).amount.value );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( vesting_withdraw_route )
{
   try
   {
      ACTORS( (alice)(bob)(sam) )

      auto original_vesting = alice.vesting_shares;

      fund( "alice", 1040000 );
      vest( "alice", 1040000 );

      auto withdraw_amount = alice.vesting_shares - original_vesting;

      BOOST_TEST_MESSAGE( "Setup vesting withdraw" );
      withdraw_vesting_operation wv;
      wv.account = "alice";
      wv.vesting_shares = withdraw_amount;

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( wv );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      BOOST_TEST_MESSAGE( "Setting up bob destination" );
      set_withdraw_vesting_route_operation op;
      op.from_account = "alice";
      op.to_account = "bob";
      op.percent = STEEM_1_PERCENT * 50;
      op.auto_vest = true;
      tx.operations.push_back( op );

      BOOST_TEST_MESSAGE( "Setting up sam destination" );
      op.to_account = "sam";
      op.percent = STEEM_1_PERCENT * 30;
      op.auto_vest = false;
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "Setting up first withdraw" );

      auto vesting_withdraw_rate = alice.vesting_withdraw_rate;
      auto old_alice_balance = alice.balance;
      auto old_alice_vesting = alice.vesting_shares;
      auto old_bob_balance = bob.balance;
      auto old_bob_vesting = bob.vesting_shares;
      auto old_sam_balance = sam.balance;
      auto old_sam_vesting = sam.vesting_shares;
      generate_blocks( alice.next_vesting_withdrawal, true );

      {
         const auto& alice = db->get_account( "alice" );
         const auto& bob = db->get_account( "bob" );
         const auto& sam = db->get_account( "sam" );

         BOOST_REQUIRE( alice.vesting_shares == old_alice_vesting - vesting_withdraw_rate );
         BOOST_REQUIRE( alice.balance == old_alice_balance + asset( ( vesting_withdraw_rate.amount * STEEM_1_PERCENT * 20 ) / STEEM_100_PERCENT, VESTS_SYMBOL ) * db->get_dynamic_global_properties().get_vesting_share_price() );
         BOOST_REQUIRE( bob.vesting_shares == old_bob_vesting + asset( ( vesting_withdraw_rate.amount * STEEM_1_PERCENT * 50 ) / STEEM_100_PERCENT, VESTS_SYMBOL ) );
         BOOST_REQUIRE( bob.balance == old_bob_balance );
         BOOST_REQUIRE( sam.vesting_shares == old_sam_vesting );
         BOOST_REQUIRE( sam.balance ==  old_sam_balance + asset( ( vesting_withdraw_rate.amount * STEEM_1_PERCENT * 30 ) / STEEM_100_PERCENT, VESTS_SYMBOL ) * db->get_dynamic_global_properties().get_vesting_share_price() );

         old_alice_balance = alice.balance;
         old_alice_vesting = alice.vesting_shares;
         old_bob_balance = bob.balance;
         old_bob_vesting = bob.vesting_shares;
         old_sam_balance = sam.balance;
         old_sam_vesting = sam.vesting_shares;
      }

      BOOST_TEST_MESSAGE( "Test failure with greater than 100% destination assignment" );

      tx.operations.clear();
      tx.signatures.clear();

      op.to_account = "sam";
      op.percent = STEEM_1_PERCENT * 50 + 1;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      STEEM_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_TEST_MESSAGE( "Test from_account receiving no withdraw" );

      tx.operations.clear();
      tx.signatures.clear();

      op.to_account = "sam";
      op.percent = STEEM_1_PERCENT * 50;
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( db->get_account( "alice" ).next_vesting_withdrawal, true );
      {
         const auto& alice = db->get_account( "alice" );
         const auto& bob = db->get_account( "bob" );
         const auto& sam = db->get_account( "sam" );

         BOOST_REQUIRE( alice.vesting_shares == old_alice_vesting - vesting_withdraw_rate );
         BOOST_REQUIRE( alice.balance == old_alice_balance );
         BOOST_REQUIRE( bob.vesting_shares == old_bob_vesting + asset( ( vesting_withdraw_rate.amount * STEEM_1_PERCENT * 50 ) / STEEM_100_PERCENT, VESTS_SYMBOL ) );
         BOOST_REQUIRE( bob.balance == old_bob_balance );
         BOOST_REQUIRE( sam.vesting_shares == old_sam_vesting );
         BOOST_REQUIRE( sam.balance ==  old_sam_balance + asset( ( vesting_withdraw_rate.amount * STEEM_1_PERCENT * 50 ) / STEEM_100_PERCENT, VESTS_SYMBOL ) * db->get_dynamic_global_properties().get_vesting_share_price() );
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif
