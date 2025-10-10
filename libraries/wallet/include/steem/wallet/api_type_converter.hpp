#pragma once

#include <steem/plugins/condenser_api/condenser_api.hpp>
#include <steem/plugins/database_api/database_api.hpp>
#include <steem/plugins/block_api/block_api.hpp>
#include <steem/plugins/account_history_api/account_history_api.hpp>
#include <steem/plugins/tags_api/tags_api.hpp>
#include <steem/plugins/market_history_api/market_history_api.hpp>

namespace steem { namespace wallet {

/**
 * Type conversion utilities for converting between condenser_api legacy types
 * and module-specific native types.
 */
class api_type_converter
{
public:
   /////////////////////////////////////////////////////////////////////////////
   // Legacy signed_transaction conversions
   /////////////////////////////////////////////////////////////////////////////

   /**
    * Convert legacy_signed_transaction to signed_transaction
    */
   static protocol::signed_transaction from_legacy_signed_transaction(
      const condenser_api::legacy_signed_transaction& legacy_trx
   );

   /**
    * Convert signed_transaction to legacy_signed_transaction
    */
   static condenser_api::legacy_signed_transaction to_legacy_signed_transaction(
      const protocol::signed_transaction& trx
   );

   /**
    * Convert annotated_signed_transaction to legacy_signed_transaction
    */
   static condenser_api::legacy_signed_transaction to_legacy_signed_transaction(
      const protocol::annotated_signed_transaction& trx
   );

   /////////////////////////////////////////////////////////////////////////////
   // Legacy signed_block conversions
   /////////////////////////////////////////////////////////////////////////////

   /**
    * Convert legacy_signed_block to signed_block
    */
   static protocol::signed_block from_legacy_signed_block(
      const condenser_api::legacy_signed_block& legacy_block
   );

   /**
    * Convert signed_block to legacy_signed_block
    */
   static condenser_api::legacy_signed_block to_legacy_signed_block(
      const protocol::signed_block& block
   );

   /////////////////////////////////////////////////////////////////////////////
   // Price conversions
   /////////////////////////////////////////////////////////////////////////////

   /**
    * Convert legacy_price to price
    */
   static protocol::price from_legacy_price(
      const condenser_api::legacy_price& legacy_price
   );

   /**
    * Convert price to legacy_price
    */
   static condenser_api::legacy_price to_legacy_price(
      const protocol::price& price
   );

   /////////////////////////////////////////////////////////////////////////////
   // Operation conversions
   /////////////////////////////////////////////////////////////////////////////

   /**
    * Convert account_history::api_operation_object to condenser_api::api_operation_object
    */
   static condenser_api::api_operation_object to_condenser_operation(
      const account_history::api_operation_object& op
   );

   /**
    * Convert map of account_history operations to condenser format
    */
   static std::map<uint32_t, condenser_api::api_operation_object> to_condenser_operation_map(
      const std::map<uint32_t, account_history::api_operation_object>& history
   );

   /**
    * Convert multiset of account_history operations to condenser vector
    */
   static std::vector<condenser_api::api_operation_object> to_condenser_operation_vector(
      const std::multiset<account_history::api_operation_object>& ops
   );

   /////////////////////////////////////////////////////////////////////////////
   // Account conversions
   /////////////////////////////////////////////////////////////////////////////

   /**
    * Convert database_api::api_account_object to condenser_api::api_account_object
    */
   static condenser_api::api_account_object to_condenser_account(
      const database_api::api_account_object& account
   );

   /**
    * Convert database_api::api_account_object to condenser_api::extended_account
    */
   static condenser_api::extended_account to_condenser_extended_account(
      const database_api::api_account_object& account
   );

   /**
    * Convert vector of database_api accounts to condenser extended_accounts
    */
   static std::vector<condenser_api::extended_account> to_condenser_extended_accounts(
      const std::vector<database_api::api_account_object>& accounts
   );

   /////////////////////////////////////////////////////////////////////////////
   // Witness conversions
   /////////////////////////////////////////////////////////////////////////////

   /**
    * Convert database_api::api_witness_object to condenser_api::api_witness_object
    */
   static condenser_api::api_witness_object to_condenser_witness(
      const database_api::api_witness_object& witness
   );

   /**
    * Convert vector of database_api witnesses to condenser format
    */
   static std::vector<condenser_api::api_witness_object> to_condenser_witnesses(
      const std::vector<database_api::api_witness_object>& witnesses
   );

   /////////////////////////////////////////////////////////////////////////////
   // Tag/Discussion conversions
   /////////////////////////////////////////////////////////////////////////////

   /**
    * Convert tags::api_tag_object to condenser_api::api_tag_object
    */
   static condenser_api::api_tag_object to_condenser_tag(
      const tags::api_tag_object& tag
   );

   /**
    * Convert vector of tags to condenser format
    */
   static std::vector<condenser_api::api_tag_object> to_condenser_tags(
      const std::vector<tags::api_tag_object>& tags
   );

   /**
    * Convert tags::discussion to condenser_api::discussion
    */
   static condenser_api::discussion to_condenser_discussion(
      const tags::discussion& disc
   );

   /**
    * Convert vector of discussions to condenser format
    */
   static std::vector<condenser_api::discussion> to_condenser_discussions(
      const std::vector<tags::discussion>& discussions
   );

   /////////////////////////////////////////////////////////////////////////////
   // Market conversions
   /////////////////////////////////////////////////////////////////////////////

   /**
    * Convert market_history::get_ticker_return to condenser_api::get_ticker_return
    */
   static condenser_api::get_ticker_return to_condenser_ticker(
      const market_history::get_ticker_return& ticker
   );

   /**
    * Convert market_history::get_volume_return to condenser_api::get_volume_return
    */
   static condenser_api::get_volume_return to_condenser_volume(
      const market_history::get_volume_return& volume
   );

   /**
    * Convert market_history::get_order_book_return to condenser_api::get_order_book_return
    */
   static condenser_api::get_order_book_return to_condenser_order_book(
      const market_history::get_order_book_return& order_book
   );

   /**
    * Convert market_history::market_trade to condenser_api::market_trade
    */
   static condenser_api::market_trade to_condenser_trade(
      const market_history::market_trade& trade
   );

   /**
    * Convert vector of market_history trades to condenser format
    */
   static std::vector<condenser_api::market_trade> to_condenser_trades(
      const std::vector<market_history::market_trade>& trades
   );

   /////////////////////////////////////////////////////////////////////////////
   // Feed/Price history conversions
   /////////////////////////////////////////////////////////////////////////////

   /**
    * Convert database_api feed history to condenser format
    */
   static condenser_api::api_feed_history_object to_condenser_feed_history(
      const database_api::api_feed_history_object& feed_history
   );

   /**
    * Convert database_api witness schedule to condenser format
    */
   static condenser_api::api_witness_schedule_object to_condenser_witness_schedule(
      const database_api::api_witness_schedule_object& schedule
   );

   /**
    * Convert database_api dynamic global properties to condenser format
    */
   static condenser_api::extended_dynamic_global_properties to_condenser_dynamic_global_props(
      const database_api::api_dynamic_global_property_object& props
   );

   /////////////////////////////////////////////////////////////////////////////
   // Limit order conversions
   /////////////////////////////////////////////////////////////////////////////

   /**
    * Convert database_api::api_limit_order_object to condenser_api::api_limit_order_object
    */
   static condenser_api::api_limit_order_object to_condenser_limit_order(
      const database_api::api_limit_order_object& order
   );

   /**
    * Convert vector of limit orders to condenser format
    */
   static std::vector<condenser_api::api_limit_order_object> to_condenser_limit_orders(
      const std::vector<database_api::api_limit_order_object>& orders
   );

   /////////////////////////////////////////////////////////////////////////////
   // Conversion request conversions
   /////////////////////////////////////////////////////////////////////////////

   /**
    * Convert database_api convert request to condenser format
    */
   static condenser_api::api_convert_request_object to_condenser_convert_request(
      const database_api::api_convert_request_object& request
   );

   /**
    * Convert vector of convert requests to condenser format
    */
   static std::vector<condenser_api::api_convert_request_object> to_condenser_convert_requests(
      const std::vector<database_api::api_convert_request_object>& requests
   );

   /////////////////////////////////////////////////////////////////////////////
   // Escrow conversions
   /////////////////////////////////////////////////////////////////////////////

   /**
    * Convert database_api escrow to condenser format
    */
   static condenser_api::api_escrow_object to_condenser_escrow(
      const database_api::api_escrow_object& escrow
   );

   /////////////////////////////////////////////////////////////////////////////
   // Savings withdrawal conversions
   /////////////////////////////////////////////////////////////////////////////

   /**
    * Convert database_api savings withdrawal to condenser format
    */
   static condenser_api::api_savings_withdraw_object to_condenser_savings_withdraw(
      const database_api::api_savings_withdraw_object& withdraw
   );

   /**
    * Convert vector of savings withdrawals to condenser format
    */
   static std::vector<condenser_api::api_savings_withdraw_object> to_condenser_savings_withdrawals(
      const std::vector<database_api::api_savings_withdraw_object>& withdrawals
   );

   /////////////////////////////////////////////////////////////////////////////
   // Vesting delegation conversions
   /////////////////////////////////////////////////////////////////////////////

   /**
    * Convert database_api vesting delegation to condenser format
    */
   static condenser_api::api_vesting_delegation_object to_condenser_vesting_delegation(
      const database_api::api_vesting_delegation_object& delegation
   );

   /**
    * Convert vector of vesting delegations to condenser format
    */
   static std::vector<condenser_api::api_vesting_delegation_object> to_condenser_vesting_delegations(
      const std::vector<database_api::api_vesting_delegation_object>& delegations
   );

   /**
    * Convert database_api vesting delegation expiration to condenser format
    */
   static condenser_api::api_vesting_delegation_expiration_object to_condenser_vesting_delegation_expiration(
      const database_api::api_vesting_delegation_expiration_object& expiration
   );

   /**
    * Convert vector of vesting delegation expirations to condenser format
    */
   static std::vector<condenser_api::api_vesting_delegation_expiration_object> to_condenser_vesting_delegation_expirations(
      const std::vector<database_api::api_vesting_delegation_expiration_object>& expirations
   );

   /////////////////////////////////////////////////////////////////////////////
   // Reward fund conversions
   /////////////////////////////////////////////////////////////////////////////

   /**
    * Convert database_api reward fund to condenser format
    */
   static condenser_api::api_reward_fund_object to_condenser_reward_fund(
      const database_api::api_reward_fund_object& fund
   );
};

} } // steem::wallet
