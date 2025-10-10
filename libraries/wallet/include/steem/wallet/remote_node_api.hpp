#pragma once

#include <steem/wallet/api_connection_manager.hpp>
#include <steem/plugins/database_api/database_api.hpp>
#include <steem/plugins/account_history_api/account_history_api.hpp>
#include <steem/plugins/tags_api/tags_api.hpp>
#include <steem/plugins/market_history_api/market_history_api.hpp>
#include <steem/plugins/network_broadcast_api/network_broadcast_api.hpp>
#include <steem/plugins/follow_api/follow_api.hpp>
#include <steem/plugins/witness/witness_plugin.hpp>
#include <steem/protocol/types.hpp>

namespace steem { namespace wallet {

using std::vector;
using fc::variant;
using fc::optional;

using namespace chain;
using namespace plugins;

/**
 * Facade API that delegates calls to appropriate module APIs
 *
 * This class maintains backward compatibility with the old remote_node_api
 * while internally routing calls to the new modular API structure.
 */
class remote_node_api
{
public:
   remote_node_api( std::shared_ptr<api_connection_manager> api_mgr )
      : _api_mgr( api_mgr )
   {
   }

   /////////////////////////////////////////////////////////////////////////////
   // Core blockchain queries - routed to database_api
   /////////////////////////////////////////////////////////////////////////////

   struct scheduled_hardfork
   {
      hardfork_version     hf_version;
      fc::time_point_sec   live_time;
   };

   fc::variant_object get_config();
   database_api::api_dynamic_global_property_object get_dynamic_global_properties();
   protocol::legacy_chain_properties get_chain_properties();
   protocol::price get_current_median_history_price();
   database_api::api_feed_history_object get_feed_history();
   database_api::api_witness_schedule_object get_witness_schedule();
   hardfork_version get_hardfork_version();
   scheduled_hardfork get_next_scheduled_hardfork();
   database_api::api_reward_fund_object get_reward_fund( string );

   /////////////////////////////////////////////////////////////////////////////
   // Block queries - routed to block_api (fallback to database_api)
   /////////////////////////////////////////////////////////////////////////////

   optional< block_header > get_block_header( uint32_t );
   optional< protocol::signed_block > get_block( uint32_t );

   /////////////////////////////////////////////////////////////////////////////
   // Account queries - routed to database_api
   /////////////////////////////////////////////////////////////////////////////

   enum withdraw_route_type
   {
      incoming,
      outgoing,
      all
   };

   vector< database_api::api_account_object > get_accounts( vector< account_name_type > );
   vector< account_id_type > get_account_references( account_id_type account_id );
   vector< optional< database_api::api_account_object > > lookup_account_names( vector< account_name_type > );
   vector< account_name_type > lookup_accounts( account_name_type, uint32_t );
   uint64_t get_account_count();
   vector< database_api::api_owner_authority_history_object > get_owner_history( account_name_type );
   optional< database_api::api_account_recovery_request_object > get_recovery_request( account_name_type );
   optional< database_api::api_escrow_object > get_escrow( account_name_type, uint32_t );
   vector< database_api::api_withdraw_vesting_route_object > get_withdraw_routes( account_name_type, withdraw_route_type );
   vector< database_api::api_savings_withdraw_object > get_savings_withdraw_from( account_name_type );
   vector< database_api::api_savings_withdraw_object > get_savings_withdraw_to( account_name_type );
   vector< database_api::api_vesting_delegation_object > get_vesting_delegations( account_name_type, account_name_type, uint32_t );
   vector< database_api::api_vesting_delegation_expiration_object > get_expiring_vesting_delegations( account_name_type, time_point_sec, uint32_t );

   /////////////////////////////////////////////////////////////////////////////
   // Key to account mapping - routed to account_by_key_api
   /////////////////////////////////////////////////////////////////////////////

   vector< vector< account_name_type > > get_key_references( vector< public_key_type > );

   /////////////////////////////////////////////////////////////////////////////
   // Account history - routed to account_history_api
   /////////////////////////////////////////////////////////////////////////////

   map< uint32_t, account_history::api_operation_object > get_account_history( account_name_type, uint64_t, uint32_t );
   vector< account_history::api_operation_object > get_ops_in_block( uint32_t, bool only_virtual = true );
   protocol::signed_transaction get_transaction( transaction_id_type );

   /////////////////////////////////////////////////////////////////////////////
   // Witness queries - routed to database_api
   /////////////////////////////////////////////////////////////////////////////

   vector< account_name_type > get_active_witnesses();
   vector< optional< database_api::api_witness_object > > get_witnesses( vector< witness_id_type > );
   vector< database_api::api_convert_request_object > get_conversion_requests( account_name_type );
   optional< database_api::api_witness_object > get_witness_by_account( account_name_type );
   vector< database_api::api_witness_object > get_witnesses_by_vote( account_name_type, uint32_t );
   vector< account_name_type > lookup_witness_accounts( string, uint32_t );
   uint64_t get_witness_count();
   optional< witness::api_account_bandwidth_object > get_account_bandwidth( account_name_type, witness::bandwidth_type );

   /////////////////////////////////////////////////////////////////////////////
   // Market queries - routed to database_api for orders, market_history_api for history
   /////////////////////////////////////////////////////////////////////////////

   vector< database_api::api_limit_order_object > get_open_orders( account_name_type );
   market_history::get_ticker_return get_ticker();
   market_history::get_volume_return get_volume();
   market_history::get_order_book_return get_order_book( uint32_t );
   vector< market_history::market_trade > get_trade_history( time_point_sec, time_point_sec, uint32_t );
   vector< market_history::market_trade > get_recent_trades( uint32_t );
   vector< market_history::bucket_object > get_market_history( uint32_t, time_point_sec, time_point_sec );
   flat_set< uint32_t > get_market_history_buckets();

   /////////////////////////////////////////////////////////////////////////////
   // Transaction utilities - routed to database_api
   /////////////////////////////////////////////////////////////////////////////

   string get_transaction_hex( protocol::signed_transaction );
   set< public_key_type > get_required_signatures( protocol::signed_transaction, flat_set< public_key_type > );
   set< public_key_type > get_potential_signatures( protocol::signed_transaction );
   bool verify_authority( protocol::signed_transaction );
   bool verify_account_authority( string, flat_set< public_key_type > );

   /////////////////////////////////////////////////////////////////////////////
   // Broadcasting - routed to network_broadcast_api
   /////////////////////////////////////////////////////////////////////////////

   struct broadcast_transaction_synchronous_return
   {
      broadcast_transaction_synchronous_return() {}
      broadcast_transaction_synchronous_return( transaction_id_type txid, int32_t bn, int32_t tn, bool ex )
      : id(txid), block_num(bn), trx_num(tn), expired(ex) {}

      transaction_id_type   id;
      int32_t               block_num = 0;
      int32_t               trx_num   = 0;
      bool                  expired   = false;
   };

   void broadcast_transaction( protocol::signed_transaction );
   broadcast_transaction_synchronous_return broadcast_transaction_synchronous( protocol::signed_transaction );
   void broadcast_block( signed_block );

   /////////////////////////////////////////////////////////////////////////////
   // Content/Discussion queries - routed to tags_api
   /////////////////////////////////////////////////////////////////////////////

   struct account_vote
   {
      account_name_type authorperm;
      uint64_t          weight = 0;
      int64_t           rshares = 0;
      int16_t           percent = 0;
      time_point_sec    time;
   };

   vector< tags::api_tag_object > get_trending_tags( string, uint32_t );
   vector< tags::vote_state > get_active_votes( account_name_type, string );
   vector< account_vote > get_account_votes( account_name_type );
   tags::discussion get_content( account_name_type, string );
   vector< tags::discussion > get_content_replies( account_name_type, string );
   vector< tags::tag_count_object > get_tags_used_by_author( account_name_type );
   vector< tags::discussion > get_discussions_by_payout( tags::discussion_query );
   vector< tags::discussion > get_post_discussions_by_payout( tags::discussion_query );
   vector< tags::discussion > get_comment_discussions_by_payout( tags::discussion_query );
   vector< tags::discussion > get_discussions_by_trending( tags::discussion_query );
   vector< tags::discussion > get_discussions_by_created( tags::discussion_query );
   vector< tags::discussion > get_discussions_by_active( tags::discussion_query );
   vector< tags::discussion > get_discussions_by_cashout( tags::discussion_query );
   vector< tags::discussion > get_discussions_by_votes( tags::discussion_query );
   vector< tags::discussion > get_discussions_by_children( tags::discussion_query );
   vector< tags::discussion > get_discussions_by_hot( tags::discussion_query );
   vector< tags::discussion > get_discussions_by_feed( tags::discussion_query );
   vector< tags::discussion > get_discussions_by_blog( tags::discussion_query );
   vector< tags::discussion > get_discussions_by_comments( tags::discussion_query );
   vector< tags::discussion > get_discussions_by_promoted( tags::discussion_query );
   vector< tags::discussion > get_replies_by_last_update( tags::discussion_query );
   vector< tags::discussion > get_discussions_by_author_before_date( tags::discussion_query );

   /////////////////////////////////////////////////////////////////////////////
   // Follow/Social queries - routed to follow_api
   /////////////////////////////////////////////////////////////////////////////

   vector< follow::api_follow_object > get_followers( account_name_type, account_name_type, follow::follow_type, uint32_t );
   vector< follow::api_follow_object > get_following( account_name_type, account_name_type, follow::follow_type, uint32_t );
   follow::get_follow_count_return get_follow_count( account_name_type );
   vector< follow::feed_entry > get_feed_entries( account_name_type, uint32_t, uint32_t );
   vector< follow::comment_feed_entry > get_feed( account_name_type, uint32_t, uint32_t );
   vector< follow::blog_entry > get_blog_entries( account_name_type, uint32_t, uint32_t );
   vector< follow::comment_blog_entry > get_blog( account_name_type, uint32_t, uint32_t );
   vector< follow::account_reputation > get_account_reputations( account_name_type, uint32_t );
   vector< account_name_type > get_reblogged_by( account_name_type, string );
   vector< follow::reblog_count > get_blog_authors( account_name_type );

   /////////////////////////////////////////////////////////////////////////////
   // Version query - placeholder
   /////////////////////////////////////////////////////////////////////////////

   struct get_version_return
   {
      get_version_return() {}
      get_version_return( fc::string bc_v, fc::string s_v, fc::string fc_v )
         :blockchain_version( bc_v ), steem_revision( s_v ), fc_revision( fc_v ) {}

      fc::string blockchain_version;
      fc::string steem_revision;
      fc::string fc_revision;
   };

   get_version_return get_version();

private:
   std::shared_ptr<api_connection_manager> _api_mgr;
};

} }

FC_API( steem::wallet::remote_node_api,
        (get_version)
        (get_trending_tags)
        (get_active_witnesses)
        (get_block_header)
        (get_block)
        (get_ops_in_block)
        (get_config)
        (get_dynamic_global_properties)
        (get_chain_properties)
        (get_current_median_history_price)
        (get_feed_history)
        (get_witness_schedule)
        (get_hardfork_version)
        (get_next_scheduled_hardfork)
        (get_reward_fund)
        (get_key_references)
        (get_accounts)
        (get_account_references)
        (lookup_account_names)
        (lookup_accounts)
        (get_account_count)
        (get_owner_history)
        (get_recovery_request)
        (get_escrow)
        (get_withdraw_routes)
        (get_account_bandwidth)
        (get_savings_withdraw_from)
        (get_savings_withdraw_to)
        (get_vesting_delegations)
        (get_expiring_vesting_delegations)
        (get_witnesses)
        (get_conversion_requests)
        (get_witness_by_account)
        (get_witnesses_by_vote)
        (get_witness_count)
        (lookup_witness_accounts)
        (get_open_orders)
        (get_transaction_hex)
        (get_transaction)
        (get_required_signatures)
        (get_potential_signatures)
        (verify_authority)
        (verify_account_authority)
        (get_active_votes)
        (get_account_votes)
        (get_content)
        (get_content_replies)
        (get_tags_used_by_author)
        (get_discussions_by_payout)
        (get_post_discussions_by_payout)
        (get_comment_discussions_by_payout)
        (get_discussions_by_trending)
        (get_discussions_by_created)
        (get_discussions_by_active)
        (get_discussions_by_cashout)
        (get_discussions_by_votes)
        (get_discussions_by_children)
        (get_discussions_by_hot)
        (get_discussions_by_feed)
        (get_discussions_by_blog)
        (get_discussions_by_comments)
        (get_discussions_by_promoted)
        (get_replies_by_last_update)
        (get_discussions_by_author_before_date)
        (get_account_history)
        (broadcast_transaction)
        (broadcast_transaction_synchronous)
        (broadcast_block)
        (get_followers)
        (get_following)
        (get_follow_count)
        (get_feed_entries)
        (get_feed)
        (get_blog_entries)
        (get_blog)
        (get_account_reputations)
        (get_reblogged_by)
        (get_blog_authors)
        (get_ticker)
        (get_volume)
        (get_order_book)
        (get_trade_history)
        (get_recent_trades)
        (get_market_history)
        (get_market_history_buckets)
      )

FC_REFLECT_ENUM( steem::wallet::remote_node_api::withdraw_route_type, (incoming)(outgoing)(all) )

FC_REFLECT( steem::wallet::remote_node_api::scheduled_hardfork,
            (hf_version)(live_time) )

FC_REFLECT( steem::wallet::remote_node_api::get_version_return,
            (blockchain_version)(steem_revision)(fc_revision) )

FC_REFLECT( steem::wallet::remote_node_api::broadcast_transaction_synchronous_return,
            (id)(block_num)(trx_num)(expired) )

FC_REFLECT( steem::wallet::remote_node_api::account_vote,
            (authorperm)(weight)(rshares)(percent)(time) )
