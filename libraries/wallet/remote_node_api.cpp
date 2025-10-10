#include <steem/wallet/remote_node_api.hpp>

namespace steem { namespace wallet {

/////////////////////////////////////////////////////////////////////////////
// Core blockchain queries - routed to database_api
/////////////////////////////////////////////////////////////////////////////

fc::variant_object remote_node_api::get_config()
{
   return _api_mgr->database_api()->call( "get_config", fc::variants() ).as< fc::variant_object >();
}

condenser_api::extended_dynamic_global_properties remote_node_api::get_dynamic_global_properties()
{
   // TODO: Convert from database_api native type to condenser type
   FC_ASSERT( false, "Not yet implemented - requires type conversion" );
}

condenser_api::api_chain_properties remote_node_api::get_chain_properties()
{
   // This is condenser_api specific, may need fallback implementation
   FC_ASSERT( false, "Not yet implemented - condenser_api specific" );
}

condenser_api::legacy_price remote_node_api::get_current_median_history_price()
{
   // TODO: Convert from database_api::get_current_price_feed
   FC_ASSERT( false, "Not yet implemented - requires type conversion" );
}

condenser_api::api_feed_history_object remote_node_api::get_feed_history()
{
   // TODO: Convert from database_api native type
   FC_ASSERT( false, "Not yet implemented - requires type conversion" );
}

condenser_api::api_witness_schedule_object remote_node_api::get_witness_schedule()
{
   // TODO: Convert from database_api native type
   FC_ASSERT( false, "Not yet implemented - requires type conversion" );
}

hardfork_version remote_node_api::get_hardfork_version()
{
   // TODO: Extract from database_api::get_hardfork_properties
   FC_ASSERT( false, "Not yet implemented" );
}

condenser_api::scheduled_hardfork remote_node_api::get_next_scheduled_hardfork()
{
   // TODO: Extract from database_api::get_hardfork_properties
   FC_ASSERT( false, "Not yet implemented" );
}

condenser_api::api_reward_fund_object remote_node_api::get_reward_fund( string name )
{
   // TODO: Convert from database_api::get_reward_funds
   FC_ASSERT( false, "Not yet implemented - requires type conversion" );
}

/////////////////////////////////////////////////////////////////////////////
// Block queries - routed to block_api
/////////////////////////////////////////////////////////////////////////////

optional< block_header > remote_node_api::get_block_header( uint32_t block_num )
{
   auto api = _api_mgr->block_api();
   if( !api ) {
      // Fallback to database_api if block_api not available
      FC_ASSERT( false, "block_api not available" );
   }

   block_api::get_block_header_args args;
   args.block_num = block_num;
   auto result = (*api)->get_block_header( args );
   return result.header;
}

optional< condenser_api::legacy_signed_block > remote_node_api::get_block( uint32_t block_num )
{
   auto api = _api_mgr->block_api();
   if( !api ) {
      FC_ASSERT( false, "block_api not available" );
   }

   block_api::get_block_args args;
   args.block_num = block_num;
   auto result = (*api)->get_block( args );

   if( !result.block )
      return optional< condenser_api::legacy_signed_block >();

   // TODO: Convert signed_block to legacy_signed_block
   FC_ASSERT( false, "Not yet implemented - requires type conversion" );
}

/////////////////////////////////////////////////////////////////////////////
// Account queries - routed to database_api
/////////////////////////////////////////////////////////////////////////////

vector< condenser_api::extended_account > remote_node_api::get_accounts( vector< account_name_type > names )
{
   // TODO: Implement using database_api::find_accounts
   FC_ASSERT( false, "Not yet implemented" );
}

vector< account_id_type > remote_node_api::get_account_references( account_id_type account_id )
{
   // This is condenser_api specific, not in database_api
   FC_ASSERT( false, "Not available in database_api - condenser_api specific" );
}

vector< optional< condenser_api::api_account_object > > remote_node_api::lookup_account_names( vector< account_name_type > names )
{
   // TODO: Implement using database_api::find_accounts
   FC_ASSERT( false, "Not yet implemented" );
}

vector< account_name_type > remote_node_api::lookup_accounts( account_name_type lower_bound_name, uint32_t limit )
{
   // TODO: Implement using database_api::list_accounts
   FC_ASSERT( false, "Not yet implemented" );
}

uint64_t remote_node_api::get_account_count()
{
   // TODO: Implement using database_api::list_accounts and count
   FC_ASSERT( false, "Not yet implemented" );
}

vector< database_api::api_owner_authority_history_object > remote_node_api::get_owner_history( account_name_type account )
{
   // TODO: Implement using database_api::find_owner_histories
   FC_ASSERT( false, "Not yet implemented" );
}

optional< database_api::api_account_recovery_request_object > remote_node_api::get_recovery_request( account_name_type account )
{
   // TODO: Implement using database_api::find_account_recovery_requests
   FC_ASSERT( false, "Not yet implemented" );
}

optional< condenser_api::api_escrow_object > remote_node_api::get_escrow( account_name_type from, uint32_t escrow_id )
{
   // TODO: Implement using database_api::find_escrows
   FC_ASSERT( false, "Not yet implemented" );
}

vector< database_api::api_withdraw_vesting_route_object > remote_node_api::get_withdraw_routes( account_name_type account, condenser_api::withdraw_route_type type )
{
   // TODO: Implement using database_api::find_withdraw_vesting_routes
   FC_ASSERT( false, "Not yet implemented" );
}

vector< condenser_api::api_savings_withdraw_object > remote_node_api::get_savings_withdraw_from( account_name_type account )
{
   // TODO: Implement using database_api::find_savings_withdrawals
   FC_ASSERT( false, "Not yet implemented" );
}

vector< condenser_api::api_savings_withdraw_object > remote_node_api::get_savings_withdraw_to( account_name_type account )
{
   // TODO: Implement using database_api::find_savings_withdrawals
   FC_ASSERT( false, "Not yet implemented" );
}

vector< condenser_api::api_vesting_delegation_object > remote_node_api::get_vesting_delegations( account_name_type account, account_name_type start, uint32_t limit )
{
   // TODO: Implement using database_api::list_vesting_delegations
   FC_ASSERT( false, "Not yet implemented" );
}

vector< condenser_api::api_vesting_delegation_expiration_object > remote_node_api::get_expiring_vesting_delegations( account_name_type account, time_point_sec start, uint32_t limit )
{
   // TODO: Implement using database_api::list_vesting_delegation_expirations
   FC_ASSERT( false, "Not yet implemented" );
}

/////////////////////////////////////////////////////////////////////////////
// Key to account mapping - routed to account_by_key_api
/////////////////////////////////////////////////////////////////////////////

vector< vector< account_name_type > > remote_node_api::get_key_references( vector< public_key_type > keys )
{
   account_by_key::get_key_references_args args;
   args.keys = keys;
   auto result = _api_mgr->account_by_key_api()->get_key_references( args );
   return result.accounts;
}

/////////////////////////////////////////////////////////////////////////////
// Account history - routed to account_history_api
/////////////////////////////////////////////////////////////////////////////

map< uint32_t, condenser_api::api_operation_object > remote_node_api::get_account_history( account_name_type account, uint64_t start, uint32_t limit )
{
   auto api = _api_mgr->account_history_api();
   if( !api ) {
      FC_ASSERT( false, "account_history_api not available" );
   }

   account_history::get_account_history_args args;
   args.account = account;
   args.start = start;
   args.limit = limit;
   auto result = (*api)->get_account_history( args );

   // TODO: Convert account_history::api_operation_object to condenser_api::api_operation_object
   FC_ASSERT( false, "Not yet implemented - requires type conversion" );
}

vector< condenser_api::api_operation_object > remote_node_api::get_ops_in_block( uint32_t block_num, bool only_virtual )
{
   auto api = _api_mgr->account_history_api();
   if( !api ) {
      FC_ASSERT( false, "account_history_api not available" );
   }

   account_history::get_ops_in_block_args args;
   args.block_num = block_num;
   args.only_virtual = only_virtual;
   auto result = (*api)->get_ops_in_block( args );

   // TODO: Convert to condenser_api type
   FC_ASSERT( false, "Not yet implemented - requires type conversion" );
}

condenser_api::legacy_signed_transaction remote_node_api::get_transaction( transaction_id_type id )
{
   auto api = _api_mgr->account_history_api();
   if( !api ) {
      FC_ASSERT( false, "account_history_api not available" );
   }

   account_history::get_transaction_args args;
   args.id = id;
   auto result = (*api)->get_transaction( args );

   // TODO: Convert to legacy_signed_transaction
   FC_ASSERT( false, "Not yet implemented - requires type conversion" );
}

/////////////////////////////////////////////////////////////////////////////
// Witness queries - routed to database_api
/////////////////////////////////////////////////////////////////////////////

vector< account_name_type > remote_node_api::get_active_witnesses()
{
   // TODO: Implement using database_api::get_active_witnesses
   FC_ASSERT( false, "Not yet implemented" );
}

vector< optional< condenser_api::api_witness_object > > remote_node_api::get_witnesses( vector< witness_id_type > witness_ids )
{
   // TODO: Implement using database_api::find_witnesses
   FC_ASSERT( false, "Not yet implemented" );
}

vector< condenser_api::api_convert_request_object > remote_node_api::get_conversion_requests( account_name_type account )
{
   // TODO: Implement using database_api::find_sbd_conversion_requests
   FC_ASSERT( false, "Not yet implemented" );
}

optional< condenser_api::api_witness_object > remote_node_api::get_witness_by_account( account_name_type account )
{
   // TODO: Implement using database_api::find_witnesses
   FC_ASSERT( false, "Not yet implemented" );
}

vector< condenser_api::api_witness_object > remote_node_api::get_witnesses_by_vote( account_name_type from, uint32_t limit )
{
   // TODO: Implement using database_api::list_witnesses
   FC_ASSERT( false, "Not yet implemented" );
}

vector< account_name_type > remote_node_api::lookup_witness_accounts( string lower_bound_name, uint32_t limit )
{
   // TODO: Implement using database_api::list_witnesses
   FC_ASSERT( false, "Not yet implemented" );
}

uint64_t remote_node_api::get_witness_count()
{
   // TODO: Implement using database_api::list_witnesses and count
   FC_ASSERT( false, "Not yet implemented" );
}

optional< witness::api_account_bandwidth_object > remote_node_api::get_account_bandwidth( account_name_type account, witness::bandwidth_type type )
{
   // This requires witness plugin API
   FC_ASSERT( false, "Requires witness plugin API - not yet implemented" );
}

/////////////////////////////////////////////////////////////////////////////
// Market queries
/////////////////////////////////////////////////////////////////////////////

vector< condenser_api::api_limit_order_object > remote_node_api::get_open_orders( account_name_type account )
{
   // TODO: Implement using database_api::find_limit_orders
   FC_ASSERT( false, "Not yet implemented" );
}

condenser_api::get_ticker_return remote_node_api::get_ticker()
{
   auto api = _api_mgr->market_history_api();
   if( !api ) {
      FC_ASSERT( false, "market_history_api not available" );
   }

   auto result = (*api)->get_ticker();
   // TODO: Convert to condenser_api type
   FC_ASSERT( false, "Not yet implemented - requires type conversion" );
}

condenser_api::get_volume_return remote_node_api::get_volume()
{
   auto api = _api_mgr->market_history_api();
   if( !api ) {
      FC_ASSERT( false, "market_history_api not available" );
   }

   auto result = (*api)->get_volume();
   // TODO: Convert to condenser_api type
   FC_ASSERT( false, "Not yet implemented - requires type conversion" );
}

condenser_api::get_order_book_return remote_node_api::get_order_book( uint32_t limit )
{
   auto api = _api_mgr->market_history_api();
   if( !api ) {
      FC_ASSERT( false, "market_history_api not available" );
   }

   market_history::get_order_book_args args;
   args.limit = limit;
   auto result = (*api)->get_order_book( args );
   // TODO: Convert to condenser_api type
   FC_ASSERT( false, "Not yet implemented - requires type conversion" );
}

vector< condenser_api::market_trade > remote_node_api::get_trade_history( time_point_sec start, time_point_sec end, uint32_t limit )
{
   auto api = _api_mgr->market_history_api();
   if( !api ) {
      FC_ASSERT( false, "market_history_api not available" );
   }

   market_history::get_trade_history_args args;
   args.start = start;
   args.end = end;
   args.limit = limit;
   auto result = (*api)->get_trade_history( args );
   // TODO: Convert to condenser_api type
   FC_ASSERT( false, "Not yet implemented - requires type conversion" );
}

vector< condenser_api::market_trade > remote_node_api::get_recent_trades( uint32_t limit )
{
   auto api = _api_mgr->market_history_api();
   if( !api ) {
      FC_ASSERT( false, "market_history_api not available" );
   }

   market_history::get_recent_trades_args args;
   args.limit = limit;
   auto result = (*api)->get_recent_trades( args );
   // TODO: Convert to condenser_api type
   FC_ASSERT( false, "Not yet implemented - requires type conversion" );
}

vector< market_history::bucket_object > remote_node_api::get_market_history( uint32_t bucket_seconds, time_point_sec start, time_point_sec end )
{
   auto api = _api_mgr->market_history_api();
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
   auto api = _api_mgr->market_history_api();
   if( !api ) {
      FC_ASSERT( false, "market_history_api not available" );
   }

   auto result = (*api)->get_market_history_buckets();
   return result.bucket_sizes;
}

/////////////////////////////////////////////////////////////////////////////
// Transaction utilities - routed to database_api
/////////////////////////////////////////////////////////////////////////////

string remote_node_api::get_transaction_hex( condenser_api::legacy_signed_transaction trx )
{
   // TODO: Convert legacy to signed_transaction and use database_api
   FC_ASSERT( false, "Not yet implemented - requires type conversion" );
}

set< public_key_type > remote_node_api::get_required_signatures( condenser_api::legacy_signed_transaction trx, flat_set< public_key_type > available_keys )
{
   // TODO: Convert legacy to signed_transaction and use database_api
   FC_ASSERT( false, "Not yet implemented - requires type conversion" );
}

set< public_key_type > remote_node_api::get_potential_signatures( condenser_api::legacy_signed_transaction trx )
{
   // TODO: Convert legacy to signed_transaction and use database_api
   FC_ASSERT( false, "Not yet implemented - requires type conversion" );
}

bool remote_node_api::verify_authority( condenser_api::legacy_signed_transaction trx )
{
   // TODO: Convert legacy to signed_transaction and use database_api
   FC_ASSERT( false, "Not yet implemented - requires type conversion" );
}

bool remote_node_api::verify_account_authority( string account, flat_set< public_key_type > signers )
{
   // TODO: Implement using database_api::verify_account_authority
   FC_ASSERT( false, "Not yet implemented" );
}

/////////////////////////////////////////////////////////////////////////////
// Broadcasting - routed to network_broadcast_api
/////////////////////////////////////////////////////////////////////////////

void remote_node_api::broadcast_transaction( condenser_api::legacy_signed_transaction trx )
{
   network_broadcast_api::broadcast_transaction_args args;
   // TODO: Convert legacy_signed_transaction to signed_transaction
   // args.trx = convert_legacy_to_signed(trx);
   _api_mgr->network_broadcast_api()->broadcast_transaction( args );
}

condenser_api::broadcast_transaction_synchronous_return remote_node_api::broadcast_transaction_synchronous( condenser_api::legacy_signed_transaction trx )
{
   // This is condenser_api specific - needs special handling
   FC_ASSERT( false, "Not yet implemented - condenser_api specific synchronous broadcast" );
}

void remote_node_api::broadcast_block( signed_block block )
{
   network_broadcast_api::broadcast_block_args args;
   args.block = block;
   _api_mgr->network_broadcast_api()->broadcast_block( args );
}

/////////////////////////////////////////////////////////////////////////////
// Content/Discussion queries - routed to tags_api
/////////////////////////////////////////////////////////////////////////////

vector< condenser_api::api_tag_object > remote_node_api::get_trending_tags( string after_tag, uint32_t limit )
{
   auto api = _api_mgr->tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   tags::get_trending_tags_args args;
   args.start_tag = after_tag;
   args.limit = limit;
   auto result = (*api)->get_trending_tags( args );
   // TODO: Convert to condenser_api type
   FC_ASSERT( false, "Not yet implemented - requires type conversion" );
}

condenser_api::state remote_node_api::get_state( string path )
{
   // This is a complex condenser_api specific query
   FC_ASSERT( false, "Not yet implemented - requires multiple API calls" );
}

vector< tags::vote_state > remote_node_api::get_active_votes( account_name_type author, string permlink )
{
   auto api = _api_mgr->tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   // TODO: Implement using tags_api::get_active_votes
   FC_ASSERT( false, "Not yet implemented" );
}

vector< condenser_api::account_vote > remote_node_api::get_account_votes( account_name_type voter )
{
   // This is condenser_api specific
   FC_ASSERT( false, "Not yet implemented - condenser_api specific" );
}

condenser_api::discussion remote_node_api::get_content( account_name_type author, string permlink )
{
   auto api = _api_mgr->tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   // TODO: Implement using tags_api::get_discussion
   FC_ASSERT( false, "Not yet implemented" );
}

vector< condenser_api::discussion > remote_node_api::get_content_replies( account_name_type author, string permlink )
{
   auto api = _api_mgr->tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   // TODO: Implement using tags_api::get_content_replies
   FC_ASSERT( false, "Not yet implemented" );
}

vector< tags::tag_count_object > remote_node_api::get_tags_used_by_author( account_name_type author )
{
   auto api = _api_mgr->tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   tags::get_tags_used_by_author_args args;
   args.author = author;
   auto result = (*api)->get_tags_used_by_author( args );
   return result.tags;
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_payout( tags::discussion_query query )
{
   // Condenser_api specific
   FC_ASSERT( false, "Not yet implemented - condenser_api specific" );
}

vector< condenser_api::discussion > remote_node_api::get_post_discussions_by_payout( tags::discussion_query query )
{
   // Condenser_api specific
   FC_ASSERT( false, "Not yet implemented - condenser_api specific" );
}

vector< condenser_api::discussion > remote_node_api::get_comment_discussions_by_payout( tags::discussion_query query )
{
   // Condenser_api specific
   FC_ASSERT( false, "Not yet implemented - condenser_api specific" );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_trending( tags::discussion_query query )
{
   auto api = _api_mgr->tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   // TODO: Implement using tags_api::get_discussions_by_trending
   FC_ASSERT( false, "Not yet implemented" );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_created( tags::discussion_query query )
{
   auto api = _api_mgr->tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   // TODO: Implement using tags_api::get_discussions_by_created
   FC_ASSERT( false, "Not yet implemented" );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_active( tags::discussion_query query )
{
   auto api = _api_mgr->tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   // TODO: Implement using tags_api::get_discussions_by_active
   FC_ASSERT( false, "Not yet implemented" );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_cashout( tags::discussion_query query )
{
   auto api = _api_mgr->tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   // TODO: Implement using tags_api::get_discussions_by_cashout
   FC_ASSERT( false, "Not yet implemented" );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_votes( tags::discussion_query query )
{
   auto api = _api_mgr->tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   // TODO: Implement using tags_api::get_discussions_by_votes
   FC_ASSERT( false, "Not yet implemented" );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_children( tags::discussion_query query )
{
   auto api = _api_mgr->tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   // TODO: Implement using tags_api::get_discussions_by_children
   FC_ASSERT( false, "Not yet implemented" );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_hot( tags::discussion_query query )
{
   auto api = _api_mgr->tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   // TODO: Implement using tags_api::get_discussions_by_hot
   FC_ASSERT( false, "Not yet implemented" );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_feed( tags::discussion_query query )
{
   auto api = _api_mgr->tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   // TODO: Implement using tags_api::get_discussions_by_feed
   FC_ASSERT( false, "Not yet implemented" );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_blog( tags::discussion_query query )
{
   auto api = _api_mgr->tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   // TODO: Implement using tags_api::get_discussions_by_blog
   FC_ASSERT( false, "Not yet implemented" );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_comments( tags::discussion_query query )
{
   auto api = _api_mgr->tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   // TODO: Implement using tags_api::get_discussions_by_comments
   FC_ASSERT( false, "Not yet implemented" );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_promoted( tags::discussion_query query )
{
   auto api = _api_mgr->tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   // TODO: Implement using tags_api::get_discussions_by_promoted
   FC_ASSERT( false, "Not yet implemented" );
}

vector< condenser_api::discussion > remote_node_api::get_replies_by_last_update( tags::discussion_query query )
{
   auto api = _api_mgr->tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   // TODO: Implement using tags_api::get_replies_by_last_update
   FC_ASSERT( false, "Not yet implemented" );
}

vector< condenser_api::discussion > remote_node_api::get_discussions_by_author_before_date( tags::discussion_query query )
{
   auto api = _api_mgr->tags_api();
   if( !api ) {
      FC_ASSERT( false, "tags_api not available" );
   }

   // TODO: Implement using tags_api::get_discussions_by_author_before_date
   FC_ASSERT( false, "Not yet implemented" );
}

/////////////////////////////////////////////////////////////////////////////
// Follow/Social queries - routed to follow_api
/////////////////////////////////////////////////////////////////////////////

vector< follow::api_follow_object > remote_node_api::get_followers( account_name_type account, account_name_type start, follow::follow_type type, uint32_t limit )
{
   auto api = _api_mgr->follow_api();
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
   auto api = _api_mgr->follow_api();
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
   auto api = _api_mgr->follow_api();
   if( !api ) {
      FC_ASSERT( false, "follow_api not available" );
   }

   follow::get_follow_count_args args;
   args.account = account;
   return (*api)->get_follow_count( args );
}

vector< follow::feed_entry > remote_node_api::get_feed_entries( account_name_type account, uint32_t entry_id, uint32_t limit )
{
   auto api = _api_mgr->follow_api();
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
   auto api = _api_mgr->follow_api();
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
   auto api = _api_mgr->follow_api();
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
   auto api = _api_mgr->follow_api();
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
   auto api = _api_mgr->follow_api();
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
   auto api = _api_mgr->follow_api();
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
   auto api = _api_mgr->follow_api();
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

condenser_api::get_version_return remote_node_api::get_version()
{
   // This needs to return version info - can get from database_api or construct locally
   FC_ASSERT( false, "Not yet implemented" );
}

} } // steem::wallet
