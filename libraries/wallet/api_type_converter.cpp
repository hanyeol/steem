#include <steem/wallet/api_type_converter.hpp>

namespace steem { namespace wallet {

/////////////////////////////////////////////////////////////////////////////
// Legacy signed_transaction conversions
/////////////////////////////////////////////////////////////////////////////

protocol::signed_transaction api_type_converter::from_legacy_signed_transaction(
   const condenser_api::legacy_signed_transaction& legacy_trx
)
{
   protocol::signed_transaction trx;
   trx.ref_block_num = legacy_trx.ref_block_num;
   trx.ref_block_prefix = legacy_trx.ref_block_prefix;
   trx.expiration = legacy_trx.expiration;

   // Convert operations from legacy format
   for( const auto& op : legacy_trx.operations )
   {
      trx.operations.push_back( op );
   }

   trx.extensions = legacy_trx.extensions;
   trx.signatures = legacy_trx.signatures;

   return trx;
}

condenser_api::legacy_signed_transaction api_type_converter::to_legacy_signed_transaction(
   const protocol::signed_transaction& trx
)
{
   condenser_api::legacy_signed_transaction legacy_trx;
   legacy_trx.ref_block_num = trx.ref_block_num;
   legacy_trx.ref_block_prefix = trx.ref_block_prefix;
   legacy_trx.expiration = trx.expiration;

   // Convert operations to legacy format
   for( const auto& op : trx.operations )
   {
      legacy_trx.operations.push_back( op );
   }

   legacy_trx.extensions = trx.extensions;
   legacy_trx.signatures = trx.signatures;

   return legacy_trx;
}

condenser_api::legacy_signed_transaction api_type_converter::to_legacy_signed_transaction(
   const protocol::annotated_signed_transaction& trx
)
{
   condenser_api::legacy_signed_transaction legacy_trx;
   legacy_trx.ref_block_num = trx.ref_block_num;
   legacy_trx.ref_block_prefix = trx.ref_block_prefix;
   legacy_trx.expiration = trx.expiration;

   for( const auto& op : trx.operations )
   {
      legacy_trx.operations.push_back( op );
   }

   legacy_trx.extensions = trx.extensions;
   legacy_trx.signatures = trx.signatures;

   return legacy_trx;
}

/////////////////////////////////////////////////////////////////////////////
// Legacy signed_block conversions
/////////////////////////////////////////////////////////////////////////////

protocol::signed_block api_type_converter::from_legacy_signed_block(
   const condenser_api::legacy_signed_block& legacy_block
)
{
   protocol::signed_block block;
   block.previous = legacy_block.previous;
   block.timestamp = legacy_block.timestamp;
   block.witness = legacy_block.witness;
   block.transaction_merkle_root = legacy_block.transaction_merkle_root;
   block.extensions = legacy_block.extensions;
   block.witness_signature = legacy_block.witness_signature;

   for( const auto& legacy_trx : legacy_block.transactions )
   {
      block.transactions.push_back( from_legacy_signed_transaction( legacy_trx ) );
   }

   return block;
}

condenser_api::legacy_signed_block api_type_converter::to_legacy_signed_block(
   const protocol::signed_block& block
)
{
   condenser_api::legacy_signed_block legacy_block;
   legacy_block.previous = block.previous;
   legacy_block.timestamp = block.timestamp;
   legacy_block.witness = block.witness;
   legacy_block.transaction_merkle_root = block.transaction_merkle_root;
   legacy_block.extensions = block.extensions;
   legacy_block.witness_signature = block.witness_signature;

   for( const auto& trx : block.transactions )
   {
      legacy_block.transactions.push_back( to_legacy_signed_transaction( trx ) );
   }

   return legacy_block;
}

/////////////////////////////////////////////////////////////////////////////
// Price conversions
/////////////////////////////////////////////////////////////////////////////

protocol::price api_type_converter::from_legacy_price(
   const condenser_api::legacy_price& legacy_price
)
{
   protocol::price price;
   price.base = legacy_price.base;
   price.quote = legacy_price.quote;
   return price;
}

condenser_api::legacy_price api_type_converter::to_legacy_price(
   const protocol::price& price
)
{
   condenser_api::legacy_price legacy_price;
   legacy_price.base = price.base;
   legacy_price.quote = price.quote;
   return legacy_price;
}

/////////////////////////////////////////////////////////////////////////////
// Operation conversions
/////////////////////////////////////////////////////////////////////////////

condenser_api::api_operation_object api_type_converter::to_condenser_operation(
   const account_history::api_operation_object& op
)
{
   condenser_api::api_operation_object result;
   result.trx_id = op.trx_id;
   result.block = op.block;
   result.trx_in_block = op.trx_in_block;
   result.op_in_trx = op.op_in_trx;
   result.virtual_op = op.virtual_op;
   result.timestamp = op.timestamp;
   result.op = op.op;
   return result;
}

std::map<uint32_t, condenser_api::api_operation_object> api_type_converter::to_condenser_operation_map(
   const std::map<uint32_t, account_history::api_operation_object>& history
)
{
   std::map<uint32_t, condenser_api::api_operation_object> result;
   for( const auto& item : history )
   {
      result[item.first] = to_condenser_operation( item.second );
   }
   return result;
}

std::vector<condenser_api::api_operation_object> api_type_converter::to_condenser_operation_vector(
   const std::multiset<account_history::api_operation_object>& ops
)
{
   std::vector<condenser_api::api_operation_object> result;
   for( const auto& op : ops )
   {
      result.push_back( to_condenser_operation( op ) );
   }
   return result;
}

/////////////////////////////////////////////////////////////////////////////
// Account conversions
/////////////////////////////////////////////////////////////////////////////

condenser_api::api_account_object api_type_converter::to_condenser_account(
   const database_api::api_account_object& account
)
{
   return condenser_api::api_account_object( account );
}

condenser_api::extended_account api_type_converter::to_condenser_extended_account(
   const database_api::api_account_object& account
)
{
   return condenser_api::extended_account( account );
}

std::vector<condenser_api::extended_account> api_type_converter::to_condenser_extended_accounts(
   const std::vector<database_api::api_account_object>& accounts
)
{
   std::vector<condenser_api::extended_account> result;
   for( const auto& account : accounts )
   {
      result.push_back( to_condenser_extended_account( account ) );
   }
   return result;
}

/////////////////////////////////////////////////////////////////////////////
// Witness conversions
/////////////////////////////////////////////////////////////////////////////

condenser_api::api_witness_object api_type_converter::to_condenser_witness(
   const database_api::api_witness_object& witness
)
{
   return condenser_api::api_witness_object( witness );
}

std::vector<condenser_api::api_witness_object> api_type_converter::to_condenser_witnesses(
   const std::vector<database_api::api_witness_object>& witnesses
)
{
   std::vector<condenser_api::api_witness_object> result;
   for( const auto& witness : witnesses )
   {
      result.push_back( to_condenser_witness( witness ) );
   }
   return result;
}

/////////////////////////////////////////////////////////////////////////////
// Tag/Discussion conversions
/////////////////////////////////////////////////////////////////////////////

condenser_api::api_tag_object api_type_converter::to_condenser_tag(
   const tags::api_tag_object& tag
)
{
   condenser_api::api_tag_object result;
   result.name = tag.name;
   result.total_payouts = tag.total_payouts;
   result.net_votes = tag.net_votes;
   result.top_posts = tag.top_posts;
   result.comments = tag.comments;
   result.trending = tag.trending;
   return result;
}

std::vector<condenser_api::api_tag_object> api_type_converter::to_condenser_tags(
   const std::vector<tags::api_tag_object>& tags_vec
)
{
   std::vector<condenser_api::api_tag_object> result;
   for( const auto& tag : tags_vec )
   {
      result.push_back( to_condenser_tag( tag ) );
   }
   return result;
}

condenser_api::discussion api_type_converter::to_condenser_discussion(
   const tags::discussion& disc
)
{
   return condenser_api::discussion( disc );
}

std::vector<condenser_api::discussion> api_type_converter::to_condenser_discussions(
   const std::vector<tags::discussion>& discussions
)
{
   std::vector<condenser_api::discussion> result;
   for( const auto& disc : discussions )
   {
      result.push_back( to_condenser_discussion( disc ) );
   }
   return result;
}

/////////////////////////////////////////////////////////////////////////////
// Market conversions
/////////////////////////////////////////////////////////////////////////////

condenser_api::get_ticker_return api_type_converter::to_condenser_ticker(
   const market_history::get_ticker_return& ticker
)
{
   condenser_api::get_ticker_return result;
   result.latest = ticker.latest;
   result.lowest_ask = ticker.lowest_ask;
   result.highest_bid = ticker.highest_bid;
   result.percent_change = ticker.percent_change;
   result.steem_volume = ticker.steem_volume;
   result.sbd_volume = ticker.sbd_volume;
   return result;
}

condenser_api::get_volume_return api_type_converter::to_condenser_volume(
   const market_history::get_volume_return& volume
)
{
   condenser_api::get_volume_return result;
   result.steem_volume = volume.steem_volume;
   result.sbd_volume = volume.sbd_volume;
   return result;
}

condenser_api::get_order_book_return api_type_converter::to_condenser_order_book(
   const market_history::get_order_book_return& order_book
)
{
   condenser_api::get_order_book_return result;
   // TODO: Convert order structures if needed
   result.bids = order_book.bids;
   result.asks = order_book.asks;
   return result;
}

condenser_api::market_trade api_type_converter::to_condenser_trade(
   const market_history::market_trade& trade
)
{
   condenser_api::market_trade result;
   result.date = trade.date;
   result.current_pays = trade.current_pays;
   result.open_pays = trade.open_pays;
   return result;
}

std::vector<condenser_api::market_trade> api_type_converter::to_condenser_trades(
   const std::vector<market_history::market_trade>& trades
)
{
   std::vector<condenser_api::market_trade> result;
   for( const auto& trade : trades )
   {
      result.push_back( to_condenser_trade( trade ) );
   }
   return result;
}

/////////////////////////////////////////////////////////////////////////////
// Feed/Price history conversions
/////////////////////////////////////////////////////////////////////////////

condenser_api::api_feed_history_object api_type_converter::to_condenser_feed_history(
   const database_api::api_feed_history_object& feed_history
)
{
   return condenser_api::api_feed_history_object( feed_history );
}

condenser_api::api_witness_schedule_object api_type_converter::to_condenser_witness_schedule(
   const database_api::api_witness_schedule_object& schedule
)
{
   return condenser_api::api_witness_schedule_object( schedule );
}

condenser_api::extended_dynamic_global_properties api_type_converter::to_condenser_dynamic_global_props(
   const database_api::api_dynamic_global_property_object& props
)
{
   return condenser_api::extended_dynamic_global_properties( props );
}

/////////////////////////////////////////////////////////////////////////////
// Limit order conversions
/////////////////////////////////////////////////////////////////////////////

condenser_api::api_limit_order_object api_type_converter::to_condenser_limit_order(
   const database_api::api_limit_order_object& order
)
{
   return condenser_api::api_limit_order_object( order );
}

std::vector<condenser_api::api_limit_order_object> api_type_converter::to_condenser_limit_orders(
   const std::vector<database_api::api_limit_order_object>& orders
)
{
   std::vector<condenser_api::api_limit_order_object> result;
   for( const auto& order : orders )
   {
      result.push_back( to_condenser_limit_order( order ) );
   }
   return result;
}

/////////////////////////////////////////////////////////////////////////////
// Conversion request conversions
/////////////////////////////////////////////////////////////////////////////

condenser_api::api_convert_request_object api_type_converter::to_condenser_convert_request(
   const database_api::api_convert_request_object& request
)
{
   return condenser_api::api_convert_request_object( request );
}

std::vector<condenser_api::api_convert_request_object> api_type_converter::to_condenser_convert_requests(
   const std::vector<database_api::api_convert_request_object>& requests
)
{
   std::vector<condenser_api::api_convert_request_object> result;
   for( const auto& request : requests )
   {
      result.push_back( to_condenser_convert_request( request ) );
   }
   return result;
}

/////////////////////////////////////////////////////////////////////////////
// Escrow conversions
/////////////////////////////////////////////////////////////////////////////

condenser_api::api_escrow_object api_type_converter::to_condenser_escrow(
   const database_api::api_escrow_object& escrow
)
{
   return condenser_api::api_escrow_object( escrow );
}

/////////////////////////////////////////////////////////////////////////////
// Savings withdrawal conversions
/////////////////////////////////////////////////////////////////////////////

condenser_api::api_savings_withdraw_object api_type_converter::to_condenser_savings_withdraw(
   const database_api::api_savings_withdraw_object& withdraw
)
{
   return condenser_api::api_savings_withdraw_object( withdraw );
}

std::vector<condenser_api::api_savings_withdraw_object> api_type_converter::to_condenser_savings_withdrawals(
   const std::vector<database_api::api_savings_withdraw_object>& withdrawals
)
{
   std::vector<condenser_api::api_savings_withdraw_object> result;
   for( const auto& withdraw : withdrawals )
   {
      result.push_back( to_condenser_savings_withdraw( withdraw ) );
   }
   return result;
}

/////////////////////////////////////////////////////////////////////////////
// Vesting delegation conversions
/////////////////////////////////////////////////////////////////////////////

condenser_api::api_vesting_delegation_object api_type_converter::to_condenser_vesting_delegation(
   const database_api::api_vesting_delegation_object& delegation
)
{
   return condenser_api::api_vesting_delegation_object( delegation );
}

std::vector<condenser_api::api_vesting_delegation_object> api_type_converter::to_condenser_vesting_delegations(
   const std::vector<database_api::api_vesting_delegation_object>& delegations
)
{
   std::vector<condenser_api::api_vesting_delegation_object> result;
   for( const auto& delegation : delegations )
   {
      result.push_back( to_condenser_vesting_delegation( delegation ) );
   }
   return result;
}

condenser_api::api_vesting_delegation_expiration_object api_type_converter::to_condenser_vesting_delegation_expiration(
   const database_api::api_vesting_delegation_expiration_object& expiration
)
{
   return condenser_api::api_vesting_delegation_expiration_object( expiration );
}

std::vector<condenser_api::api_vesting_delegation_expiration_object> api_type_converter::to_condenser_vesting_delegation_expirations(
   const std::vector<database_api::api_vesting_delegation_expiration_object>& expirations
)
{
   std::vector<condenser_api::api_vesting_delegation_expiration_object> result;
   for( const auto& expiration : expirations )
   {
      result.push_back( to_condenser_vesting_delegation_expiration( expiration ) );
   }
   return result;
}

/////////////////////////////////////////////////////////////////////////////
// Reward fund conversions
/////////////////////////////////////////////////////////////////////////////

condenser_api::api_reward_fund_object api_type_converter::to_condenser_reward_fund(
   const database_api::api_reward_fund_object& fund
)
{
   return condenser_api::api_reward_fund_object( fund );
}

} } // steem::wallet
