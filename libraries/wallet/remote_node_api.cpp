#include <steem/wallet/remote_node_api.hpp>

namespace steem { namespace wallet {

/////////////////////////////////////////////////////////////////////////////
// Core blockchain queries - routed to database_api
/////////////////////////////////////////////////////////////////////////////

fc::variant_object remote_node_api::get_config()
{
   return _api_mgr->get_database_api()->call( "get_config", fc::variants() ).as< fc::variant_object >();
}

database_api::api_dynamic_global_property_object remote_node_api::get_dynamic_global_properties()
{
   auto result = _api_mgr->get_database_api()->get_dynamic_global_properties( {} );
   return result;
}

protocol::legacy_chain_properties remote_node_api::get_chain_properties()
{
   database_api::get_witness_schedule_args args;
   auto result = _api_mgr->get_database_api()->get_witness_schedule( args );
   return result.median_props;
}

protocol::price remote_node_api::get_current_median_history_price()
{
   database_api::get_current_price_feed_args args;
   auto result = _api_mgr->get_database_api()->get_current_price_feed( args );
   return result.price_feed.current_median_history;
}

database_api::api_feed_history_object remote_node_api::get_feed_history()
{
   database_api::find_feed_history_args args;
   auto result = _api_mgr->get_database_api()->find_feed_history( args );
   FC_ASSERT( result.feed_history, "Feed history not found" );
   return *result.feed_history;
}

database_api::api_witness_schedule_object remote_node_api::get_witness_schedule()
{
   database_api::get_witness_schedule_args args;
   auto result = _api_mgr->get_database_api()->get_witness_schedule( args );
   return result;
}

hardfork_version remote_node_api::get_hardfork_version()
{
   database_api::get_hardfork_properties_args args;
   auto result = _api_mgr->get_database_api()->get_hardfork_properties( args );
   return result.current_hardfork_version;
}

remote_node_api::scheduled_hardfork remote_node_api::get_next_scheduled_hardfork()
{
   database_api::get_hardfork_properties_args args;
   auto result = _api_mgr->get_database_api()->get_hardfork_properties( args );

   scheduled_hardfork hf;
   hf.hf_version = result.next_hardfork;
   hf.live_time = result.next_hardfork_time;
   return hf;
}

database_api::api_reward_fund_object remote_node_api::get_reward_fund( string name )
{
   database_api::find_reward_funds_args args;
   args.fund_names.push_back( name );
   auto result = _api_mgr->get_database_api()->find_reward_funds( args );
   FC_ASSERT( result.funds.size() > 0, "Reward fund not found" );
   return result.funds[0];
}

/////////////////////////////////////////////////////////////////////////////
// Block queries - routed to block_api
/////////////////////////////////////////////////////////////////////////////

optional< block_header > remote_node_api::get_block_header( uint32_t block_num )
{
   auto api = _api_mgr->get_block_api();
   if( !api ) {
      FC_ASSERT( false, "block_api not available" );
   }

   block_api::get_block_header_args args;
   args.block_num = block_num;
   auto result = (*api)->get_block_header( args );
   return result.header;
}

optional< protocol::signed_block > remote_node_api::get_block( uint32_t block_num )
{
   auto api = _api_mgr->get_block_api();
   if( !api ) {
      FC_ASSERT( false, "block_api not available" );
   }

   block_api::get_block_args args;
   args.block_num = block_num;
   auto result = (*api)->get_block( args );
   return result.block;
}

/////////////////////////////////////////////////////////////////////////////
// Account queries - routed to database_api
/////////////////////////////////////////////////////////////////////////////

vector< database_api::api_account_object > remote_node_api::get_accounts( vector< account_name_type > names )
{
   database_api::find_accounts_args args;
   args.accounts = names;
   auto result = _api_mgr->get_database_api()->find_accounts( args );
   return result.accounts;
}

vector< account_id_type > remote_node_api::get_account_references( account_id_type account_id )
{
   // This functionality is not available in the modern API structure
   FC_ASSERT( false, "get_account_references is not supported - deprecated API" );
}

vector< optional< database_api::api_account_object > > remote_node_api::lookup_account_names( vector< account_name_type > names )
{
   database_api::find_accounts_args args;
   args.accounts = names;
   auto result = _api_mgr->get_database_api()->find_accounts( args );

   // Convert to vector of optionals, matching order of input names
   vector< optional< database_api::api_account_object > > ret;
   ret.reserve( names.size() );

   for( const auto& name : names )
   {
      bool found = false;
      for( const auto& account : result.accounts )
      {
         if( account.name == name )
         {
            ret.push_back( account );
            found = true;
            break;
         }
      }
      if( !found )
         ret.push_back( optional< database_api::api_account_object >() );
   }

   return ret;
}

vector< account_name_type > remote_node_api::lookup_accounts( account_name_type lower_bound_name, uint32_t limit )
{
   database_api::list_accounts_args args;
   args.start = lower_bound_name;
   args.limit = limit;
   args.order = database_api::by_name;
   auto result = _api_mgr->get_database_api()->list_accounts( args );

   vector< account_name_type > names;
   names.reserve( result.accounts.size() );
   for( const auto& account : result.accounts )
      names.push_back( account.name );

   return names;
}

uint64_t remote_node_api::get_account_count()
{
   database_api::find_account_count_args args;
   auto result = _api_mgr->get_database_api()->find_account_count( args );
   return result.count;
}

vector< database_api::api_owner_authority_history_object > remote_node_api::get_owner_history( account_name_type account )
{
   database_api::find_owner_histories_args args;
   args.owner = account;
   auto result = _api_mgr->get_database_api()->find_owner_histories( args );
   return result.owner_auths;
}

optional< database_api::api_account_recovery_request_object > remote_node_api::get_recovery_request( account_name_type account )
{
   database_api::find_account_recovery_requests_args args;
   args.accounts.push_back( account );
   auto result = _api_mgr->get_database_api()->find_account_recovery_requests( args );

   if( result.requests.empty() )
      return optional< database_api::api_account_recovery_request_object >();

   return result.requests[0];
}

optional< database_api::api_escrow_object > remote_node_api::get_escrow( account_name_type from, uint32_t escrow_id )
{
   database_api::find_escrows_args args;
   args.from = from;
   auto result = _api_mgr->get_database_api()->find_escrows( args );

   for( const auto& escrow : result.escrows )
   {
      if( escrow.escrow_id == escrow_id )
         return escrow;
   }

   return optional< database_api::api_escrow_object >();
}

vector< database_api::api_withdraw_vesting_route_object > remote_node_api::get_withdraw_routes( account_name_type account, withdraw_route_type type )
{
   database_api::find_withdraw_vesting_routes_args args;
   args.account = account;

   switch( type )
   {
      case incoming:
         args.order = database_api::by_withdraw_route;
         break;
      case outgoing:
         args.order = database_api::by_destination;
         break;
      case all:
         args.order = database_api::by_withdraw_route;
         break;
   }

   auto result = _api_mgr->get_database_api()->find_withdraw_vesting_routes( args );
   return result.routes;
}

vector< database_api::api_savings_withdraw_object > remote_node_api::get_savings_withdraw_from( account_name_type account )
{
   database_api::find_savings_withdrawals_args args;
   args.account = account;
   auto result = _api_mgr->get_database_api()->find_savings_withdrawals( args );
   return result.withdrawals;
}

vector< database_api::api_savings_withdraw_object > remote_node_api::get_savings_withdraw_to( account_name_type account )
{
   // The database_api doesn't separate "from" and "to" - it returns all withdrawals for an account
   // For "to" withdrawals, we need to filter by the 'to' field
   database_api::find_savings_withdrawals_args args;
   args.account = account;
   auto result = _api_mgr->get_database_api()->find_savings_withdrawals( args );

   vector< database_api::api_savings_withdraw_object > to_withdrawals;
   for( const auto& w : result.withdrawals )
   {
      if( w.to == account )
         to_withdrawals.push_back( w );
   }

   return to_withdrawals;
}

vector< database_api::api_vesting_delegation_object > remote_node_api::get_vesting_delegations( account_name_type account, account_name_type start, uint32_t limit )
{
   database_api::list_vesting_delegations_args args;
   args.start = fc::variant( std::make_pair( account, start ) ).as< fc::variant_object >();
   args.limit = limit;
   args.order = database_api::by_delegation;
   auto result = _api_mgr->get_database_api()->list_vesting_delegations( args );

   // Filter to only include delegations from the specified account
   vector< database_api::api_vesting_delegation_object > filtered;
   for( const auto& delegation : result.delegations )
   {
      if( delegation.delegator == account )
         filtered.push_back( delegation );
   }

   return filtered;
}

vector< database_api::api_vesting_delegation_expiration_object > remote_node_api::get_expiring_vesting_delegations( account_name_type account, time_point_sec start, uint32_t limit )
{
   database_api::list_vesting_delegation_expirations_args args;
   args.start = fc::variant( std::make_pair( account, start ) ).as< fc::variant_object >();
   args.limit = limit;
   args.order = database_api::by_expiration;
   auto result = _api_mgr->get_database_api()->list_vesting_delegation_expirations( args );

   // Filter to only include expirations for the specified account
   vector< database_api::api_vesting_delegation_expiration_object > filtered;
   for( const auto& expiration : result.delegations )
   {
      if( expiration.delegator == account )
         filtered.push_back( expiration );
   }

   return filtered;
}

/////////////////////////////////////////////////////////////////////////////
// Key to account mapping - routed to account_by_key_api
/////////////////////////////////////////////////////////////////////////////

vector< vector< account_name_type > > remote_node_api::get_key_references( vector< public_key_type > keys )
{
   account_by_key::get_key_references_args args;
   args.keys = keys;
   auto result = _api_mgr->get_account_by_key_api()->get_key_references( args );
   return result.accounts;
}

/////////////////////////////////////////////////////////////////////////////
// Account history - routed to account_history_api
/////////////////////////////////////////////////////////////////////////////

map< uint32_t, account_history::api_operation_object > remote_node_api::get_account_history( account_name_type account, uint64_t start, uint32_t limit )
{
   auto api = _api_mgr->get_account_history_api();
   if( !api ) {
      FC_ASSERT( false, "account_history_api not available" );
   }

   account_history::get_account_history_args args;
   args.account = account;
   args.start = start;
   args.limit = limit;
   auto result = (*api)->get_account_history( args );
   return result.history;
}

vector< account_history::api_operation_object > remote_node_api::get_ops_in_block( uint32_t block_num, bool only_virtual )
{
   auto api = _api_mgr->get_account_history_api();
   if( !api ) {
      FC_ASSERT( false, "account_history_api not available" );
   }

   account_history::get_ops_in_block_args args;
   args.block_num = block_num;
   args.only_virtual = only_virtual;
   auto result = (*api)->get_ops_in_block( args );
   return result.ops;
}

protocol::signed_transaction remote_node_api::get_transaction( transaction_id_type id )
{
   auto api = _api_mgr->get_account_history_api();
   if( !api ) {
      FC_ASSERT( false, "account_history_api not available" );
   }

   account_history::get_transaction_args args;
   args.id = id;
   auto result = (*api)->get_transaction( args );

   FC_ASSERT( result.transaction, "Transaction not found" );
   return *result.transaction;
}

/////////////////////////////////////////////////////////////////////////////
// Witness queries - routed to database_api
/////////////////////////////////////////////////////////////////////////////

vector< account_name_type > remote_node_api::get_active_witnesses()
{
   database_api::get_active_witnesses_args args;
   auto result = _api_mgr->get_database_api()->get_active_witnesses( args );
   return result.witnesses;
}

vector< optional< database_api::api_witness_object > > remote_node_api::get_witnesses( vector< witness_id_type > witness_ids )
{
   database_api::find_witnesses_args args;

   // Convert witness_ids to owner names (we need to look up by name)
   // This is a limitation - the modern API uses names, not IDs
   // For now, we'll have to iterate and find by ID
   FC_ASSERT( false, "get_witnesses by ID is not directly supported - use get_witness_by_account instead" );
}

vector< database_api::api_convert_request_object > remote_node_api::get_conversion_requests( account_name_type account )
{
   database_api::find_sbd_conversion_requests_args args;
   args.account = account;
   auto result = _api_mgr->get_database_api()->find_sbd_conversion_requests( args );
   return result.requests;
}

optional< database_api::api_witness_object > remote_node_api::get_witness_by_account( account_name_type account )
{
   database_api::find_witnesses_args args;
   args.owners.push_back( account );
   auto result = _api_mgr->get_database_api()->find_witnesses( args );

   if( result.witnesses.empty() )
      return optional< database_api::api_witness_object >();

   return result.witnesses[0];
}

vector< database_api::api_witness_object > remote_node_api::get_witnesses_by_vote( account_name_type from, uint32_t limit )
{
   database_api::list_witnesses_args args;
   args.start = fc::variant( from ).as< fc::variant_object >();
   args.limit = limit;
   args.order = database_api::by_vote_name;
   auto result = _api_mgr->get_database_api()->list_witnesses( args );
   return result.witnesses;
}

vector< account_name_type > remote_node_api::lookup_witness_accounts( string lower_bound_name, uint32_t limit )
{
   database_api::list_witnesses_args args;
   args.start = fc::variant( lower_bound_name ).as< fc::variant_object >();
   args.limit = limit;
   args.order = database_api::by_name;
   auto result = _api_mgr->get_database_api()->list_witnesses( args );

   vector< account_name_type > names;
   names.reserve( result.witnesses.size() );
   for( const auto& witness : result.witnesses )
      names.push_back( witness.owner );

   return names;
}

uint64_t remote_node_api::get_witness_count()
{
   database_api::find_witness_count_args args;
   auto result = _api_mgr->get_database_api()->find_witness_count( args );
   return result.count;
}

optional< witness::api_account_bandwidth_object > remote_node_api::get_account_bandwidth( account_name_type account, witness::bandwidth_type type )
{
   // This requires witness plugin API which may not be available
   FC_ASSERT( false, "get_account_bandwidth requires witness plugin API - not yet implemented" );
}

/////////////////////////////////////////////////////////////////////////////
// Market queries
/////////////////////////////////////////////////////////////////////////////

vector< database_api::api_limit_order_object > remote_node_api::get_open_orders( account_name_type account )
{
   database_api::find_limit_orders_args args;
   args.account = account;
   auto result = _api_mgr->get_database_api()->find_limit_orders( args );
   return result.orders;
}

market_history::get_ticker_return remote_node_api::get_ticker()
{
   auto api = _api_mgr->get_market_history_api();
   if( !api ) {
      FC_ASSERT( false, "market_history_api not available" );
   }

   return (*api)->get_ticker();
}

market_history::get_volume_return remote_node_api::get_volume()
{
   auto api = _api_mgr->get_market_history_api();
   if( !api ) {
      FC_ASSERT( false, "market_history_api not available" );
   }

   return (*api)->get_volume();
}

market_history::get_order_book_return remote_node_api::get_order_book( uint32_t limit )
{
   auto api = _api_mgr->get_market_history_api();
   if( !api ) {
      FC_ASSERT( false, "market_history_api not available" );
   }

   market_history::get_order_book_args args;
   args.limit = limit;
   return (*api)->get_order_book( args );
}

vector< market_history::market_trade > remote_node_api::get_trade_history( time_point_sec start, time_point_sec end, uint32_t limit )
{
   auto api = _api_mgr->get_market_history_api();
   if( !api ) {
      FC_ASSERT( false, "market_history_api not available" );
   }

   market_history::get_trade_history_args args;
   args.start = start;
   args.end = end;
   args.limit = limit;
   auto result = (*api)->get_trade_history( args );
   return result.trades;
}

vector< market_history::market_trade > remote_node_api::get_recent_trades( uint32_t limit )
{
   auto api = _api_mgr->get_market_history_api();
   if( !api ) {
      FC_ASSERT( false, "market_history_api not available" );
   }

   market_history::get_recent_trades_args args;
   args.limit = limit;
   auto result = (*api)->get_recent_trades( args );
   return result.trades;
}

vector< market_history::bucket_object > remote_node_api::get_market_history( uint32_t bucket_seconds, time_point_sec start, time_point_sec end )
{
   auto api = _api_mgr->get_market_history_api();
   if( !api ) {
      FC_ASSERT( false, "market_history_api not available" );
   }

   market_history::get_market_history_args args;
   args.bucket_seconds = bucket_seconds;
   args.start = start;
   args.end = end;
   auto result = (*api)->get_market_history( args );
   return result.buckets;
}

flat_set< uint32_t > remote_node_api::get_market_history_buckets()
{
   auto api = _api_mgr->get_market_history_api();
   if( !api ) {
      FC_ASSERT( false, "market_history_api not available" );
   }

   auto result = (*api)->get_market_history_buckets();
   return result.bucket_sizes;
}

/////////////////////////////////////////////////////////////////////////////
// Transaction utilities - routed to database_api
/////////////////////////////////////////////////////////////////////////////

string remote_node_api::get_transaction_hex( protocol::signed_transaction trx )
{
   database_api::get_transaction_hex_args args;
   args.trx = trx;
   auto result = _api_mgr->get_database_api()->get_transaction_hex( args );
   return result.hex;
}

set< public_key_type > remote_node_api::get_required_signatures( protocol::signed_transaction trx, flat_set< public_key_type > available_keys )
{
   database_api::get_required_signatures_args args;
   args.trx = trx;
   args.available_keys = available_keys;
   auto result = _api_mgr->get_database_api()->get_required_signatures( args );
   return result.keys;
}

set< public_key_type > remote_node_api::get_potential_signatures( protocol::signed_transaction trx )
{
   database_api::get_potential_signatures_args args;
   args.trx = trx;
   auto result = _api_mgr->get_database_api()->get_potential_signatures( args );
   return result.keys;
}

bool remote_node_api::verify_authority( protocol::signed_transaction trx )
{
   database_api::verify_authority_args args;
   args.trx = trx;
   auto result = _api_mgr->get_database_api()->verify_authority( args );
   return result.valid;
}

bool remote_node_api::verify_account_authority( string account, flat_set< public_key_type > signers )
{
   database_api::verify_account_authority_args args;
   args.account = account;
   args.signers = signers;
   auto result = _api_mgr->get_database_api()->verify_account_authority( args );
   return result.valid;
}

/////////////////////////////////////////////////////////////////////////////
// Broadcasting - routed to network_broadcast_api
/////////////////////////////////////////////////////////////////////////////

void remote_node_api::broadcast_transaction( protocol::signed_transaction trx )
{
   network_broadcast_api::broadcast_transaction_args args;
   args.trx = trx;
   _api_mgr->get_network_broadcast_api()->broadcast_transaction( args );
}

remote_node_api::broadcast_transaction_synchronous_return remote_node_api::broadcast_transaction_synchronous( protocol::signed_transaction trx )
{
   network_broadcast_api::broadcast_transaction_synchronous_args args;
   args.trx = trx;
   auto result = _api_mgr->get_network_broadcast_api()->broadcast_transaction_synchronous( args );

   broadcast_transaction_synchronous_return ret;
   ret.id = result.id;
   ret.block_num = result.block_num;
   ret.trx_num = result.trx_num;
   ret.expired = result.expired;
   return ret;
}

void remote_node_api::broadcast_block( signed_block block )
{
   network_broadcast_api::broadcast_block_args args;
   args.block = block;
   _api_mgr->get_network_broadcast_api()->broadcast_block( args );
}

/////////////////////////////////////////////////////////////////////////////
// Content/Discussion queries - routed to tags_api
/////////////////////////////////////////////////////////////////////////////

vector< tags::api_tag_object > remote_node_api::get_trending_tags( string after_tag, uint32_t limit )
{
   auto api = _api_mgr->get_tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   tags::get_trending_tags_args args;
   args.start_tag = after_tag;
   args.limit = limit;
   auto result = (*api)->get_trending_tags( args );
   return result.tags;
}

vector< tags::vote_state > remote_node_api::get_active_votes( account_name_type author, string permlink )
{
   auto api = _api_mgr->get_tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   tags::get_active_votes_args args;
   args.author = author;
   args.permlink = permlink;
   auto result = (*api)->get_active_votes( args );
   return result.votes;
}

vector< remote_node_api::account_vote > remote_node_api::get_account_votes( account_name_type voter )
{
   // This is a condenser_api specific function that doesn't have a direct equivalent
   // We would need to iterate through all discussions which is not practical
   FC_ASSERT( false, "get_account_votes is not available - use tags_api queries instead" );
}

tags::discussion remote_node_api::get_content( account_name_type author, string permlink )
{
   auto api = _api_mgr->get_tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   tags::get_discussion_args args;
   args.author = author;
   args.permlink = permlink;
   auto result = (*api)->get_discussion( args );
   return result.discussion;
}

vector< tags::discussion > remote_node_api::get_content_replies( account_name_type author, string permlink )
{
   auto api = _api_mgr->get_tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   tags::get_content_replies_args args;
   args.author = author;
   args.permlink = permlink;
   auto result = (*api)->get_content_replies( args );
   return result.discussions;
}

vector< tags::tag_count_object > remote_node_api::get_tags_used_by_author( account_name_type author )
{
   auto api = _api_mgr->get_tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   tags::get_tags_used_by_author_args args;
   args.author = author;
   auto result = (*api)->get_tags_used_by_author( args );
   return result.tags;
}

vector< tags::discussion > remote_node_api::get_discussions_by_payout( tags::discussion_query query )
{
   auto api = _api_mgr->get_tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   auto result = (*api)->get_discussions_by_payout( query );
   return result.discussions;
}

vector< tags::discussion > remote_node_api::get_post_discussions_by_payout( tags::discussion_query query )
{
   auto api = _api_mgr->get_tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   auto result = (*api)->get_post_discussions_by_payout( query );
   return result.discussions;
}

vector< tags::discussion > remote_node_api::get_comment_discussions_by_payout( tags::discussion_query query )
{
   auto api = _api_mgr->get_tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   auto result = (*api)->get_comment_discussions_by_payout( query );
   return result.discussions;
}

vector< tags::discussion > remote_node_api::get_discussions_by_trending( tags::discussion_query query )
{
   auto api = _api_mgr->get_tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   auto result = (*api)->get_discussions_by_trending( query );
   return result.discussions;
}

vector< tags::discussion > remote_node_api::get_discussions_by_created( tags::discussion_query query )
{
   auto api = _api_mgr->get_tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   auto result = (*api)->get_discussions_by_created( query );
   return result.discussions;
}

vector< tags::discussion > remote_node_api::get_discussions_by_active( tags::discussion_query query )
{
   auto api = _api_mgr->get_tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   auto result = (*api)->get_discussions_by_active( query );
   return result.discussions;
}

vector< tags::discussion > remote_node_api::get_discussions_by_cashout( tags::discussion_query query )
{
   auto api = _api_mgr->get_tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   auto result = (*api)->get_discussions_by_cashout( query );
   return result.discussions;
}

vector< tags::discussion > remote_node_api::get_discussions_by_votes( tags::discussion_query query )
{
   auto api = _api_mgr->get_tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   auto result = (*api)->get_discussions_by_votes( query );
   return result.discussions;
}

vector< tags::discussion > remote_node_api::get_discussions_by_children( tags::discussion_query query )
{
   auto api = _api_mgr->get_tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   auto result = (*api)->get_discussions_by_children( query );
   return result.discussions;
}

vector< tags::discussion > remote_node_api::get_discussions_by_hot( tags::discussion_query query )
{
   auto api = _api_mgr->get_tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   auto result = (*api)->get_discussions_by_hot( query );
   return result.discussions;
}

vector< tags::discussion > remote_node_api::get_discussions_by_feed( tags::discussion_query query )
{
   auto api = _api_mgr->get_tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   auto result = (*api)->get_discussions_by_feed( query );
   return result.discussions;
}

vector< tags::discussion > remote_node_api::get_discussions_by_blog( tags::discussion_query query )
{
   auto api = _api_mgr->get_tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   auto result = (*api)->get_discussions_by_blog( query );
   return result.discussions;
}

vector< tags::discussion > remote_node_api::get_discussions_by_comments( tags::discussion_query query )
{
   auto api = _api_mgr->get_tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   auto result = (*api)->get_discussions_by_comments( query );
   return result.discussions;
}

vector< tags::discussion > remote_node_api::get_discussions_by_promoted( tags::discussion_query query )
{
   auto api = _api_mgr->get_tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   auto result = (*api)->get_discussions_by_promoted( query );
   return result.discussions;
}

vector< tags::discussion > remote_node_api::get_replies_by_last_update( tags::discussion_query query )
{
   auto api = _api_mgr->get_tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   auto result = (*api)->get_replies_by_last_update( query );
   return result.discussions;
}

vector< tags::discussion > remote_node_api::get_discussions_by_author_before_date( tags::discussion_query query )
{
   auto api = _api_mgr->get_tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   auto result = (*api)->get_discussions_by_author_before_date( query );
   return result.discussions;
}

/////////////////////////////////////////////////////////////////////////////
// Follow/Social queries - routed to follow_api
/////////////////////////////////////////////////////////////////////////////

vector< follow::api_follow_object > remote_node_api::get_followers( account_name_type account, account_name_type start, follow::follow_type type, uint32_t limit )
{
   auto api = _api_mgr->get_follow_api();
   if( !api ) {
      FC_ASSERT( false, "follow_api not available" );
   }

   follow::get_followers_args args;
   args.account = account;
   args.start = start;
   args.type = type;
   args.limit = limit;
   auto result = (*api)->get_followers( args );
   return result.followers;
}

vector< follow::api_follow_object > remote_node_api::get_following( account_name_type account, account_name_type start, follow::follow_type type, uint32_t limit )
{
   auto api = _api_mgr->get_follow_api();
   if( !api ) {
      FC_ASSERT( false, "follow_api not available" );
   }

   follow::get_following_args args;
   args.account = account;
   args.start = start;
   args.type = type;
   args.limit = limit;
   auto result = (*api)->get_following( args );
   return result.following;
}

follow::get_follow_count_return remote_node_api::get_follow_count( account_name_type account )
{
   auto api = _api_mgr->get_follow_api();
   if( !api ) {
      FC_ASSERT( false, "follow_api not available" );
   }

   follow::get_follow_count_args args;
   args.account = account;
   return (*api)->get_follow_count( args );
}

vector< follow::feed_entry > remote_node_api::get_feed_entries( account_name_type account, uint32_t entry_id, uint32_t limit )
{
   auto api = _api_mgr->get_follow_api();
   if( !api ) {
      FC_ASSERT( false, "follow_api not available" );
   }

   follow::get_feed_entries_args args;
   args.account = account;
   args.start_entry_id = entry_id;
   args.limit = limit;
   auto result = (*api)->get_feed_entries( args );
   return result.feed;
}

vector< follow::comment_feed_entry > remote_node_api::get_feed( account_name_type account, uint32_t entry_id, uint32_t limit )
{
   auto api = _api_mgr->get_follow_api();
   if( !api ) {
      FC_ASSERT( false, "follow_api not available" );
   }

   follow::get_feed_args args;
   args.account = account;
   args.start_entry_id = entry_id;
   args.limit = limit;
   auto result = (*api)->get_feed( args );
   return result.feed;
}

vector< follow::blog_entry > remote_node_api::get_blog_entries( account_name_type account, uint32_t entry_id, uint32_t limit )
{
   auto api = _api_mgr->get_follow_api();
   if( !api ) {
      FC_ASSERT( false, "follow_api not available" );
   }

   follow::get_blog_entries_args args;
   args.account = account;
   args.start_entry_id = entry_id;
   args.limit = limit;
   auto result = (*api)->get_blog_entries( args );
   return result.blog;
}

vector< follow::comment_blog_entry > remote_node_api::get_blog( account_name_type account, uint32_t entry_id, uint32_t limit )
{
   auto api = _api_mgr->get_follow_api();
   if( !api ) {
      FC_ASSERT( false, "follow_api not available" );
   }

   follow::get_blog_args args;
   args.account = account;
   args.start_entry_id = entry_id;
   args.limit = limit;
   auto result = (*api)->get_blog( args );
   return result.blog;
}

vector< follow::account_reputation > remote_node_api::get_account_reputations( account_name_type account_lower_bound, uint32_t limit )
{
   auto api = _api_mgr->get_follow_api();
   if( !api ) {
      FC_ASSERT( false, "follow_api not available" );
   }

   follow::get_account_reputations_args args;
   args.account_lower_bound = account_lower_bound;
   args.limit = limit;
   auto result = (*api)->get_account_reputations( args );
   return result.reputations;
}

vector< account_name_type > remote_node_api::get_reblogged_by( account_name_type author, string permlink )
{
   auto api = _api_mgr->get_follow_api();
   if( !api ) {
      FC_ASSERT( false, "follow_api not available" );
   }

   follow::get_reblogged_by_args args;
   args.author = author;
   args.permlink = permlink;
   auto result = (*api)->get_reblogged_by( args );
   return result.accounts;
}

vector< follow::reblog_count > remote_node_api::get_blog_authors( account_name_type blog_account )
{
   auto api = _api_mgr->get_follow_api();
   if( !api ) {
      FC_ASSERT( false, "follow_api not available" );
   }

   follow::get_blog_authors_args args;
   args.blog_account = blog_account;
   auto result = (*api)->get_blog_authors( args );
   return result.blog_authors;
}

/////////////////////////////////////////////////////////////////////////////
// Version query
/////////////////////////////////////////////////////////////////////////////

remote_node_api::get_version_return remote_node_api::get_version()
{
   database_api::get_version_args args;
   auto result = _api_mgr->get_database_api()->get_version( args );

   get_version_return ret;
   ret.blockchain_version = result.blockchain_version;
   ret.steem_revision = result.steem_revision;
   ret.fc_revision = result.fc_revision;
   return ret;
}

} } // steem::wallet
