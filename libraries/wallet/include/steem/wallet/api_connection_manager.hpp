#pragma once

#include <fc/rpc/websocket_api.hpp>
#include <fc/api.hpp>

#include <steem/wallet/connectors/database_api.hpp>
#include <steem/wallet/connectors/network_broadcast_api.hpp>
#include <steem/wallet/connectors/account_by_key_api.hpp>
#include <steem/wallet/connectors/block_api.hpp>
#include <steem/wallet/connectors/account_history_api.hpp>
#include <steem/wallet/connectors/follow_api.hpp>
#include <steem/wallet/connectors/market_history_api.hpp>
#include <steem/wallet/connectors/tags_api.hpp>

namespace steem { namespace wallet {

/**
 * Manages connections to multiple API modules on a remote steemd node
 *
 * This class provides lazy loading of API connections - APIs are only
 * connected when first accessed. It also provides graceful degradation
 * if some APIs are not available on the node.
 */
class api_connection_manager
{
public:
   api_connection_manager( std::shared_ptr<fc::rpc::websocket_api_connection> api_connection )
      : _api_connection( api_connection )
   {
   }

   // Core APIs - required for basic wallet functionality
   fc::api<database_api_proxy> get_database_api()
   {
      if( !_database_api )
      {
         try {
            _database_api = _api_connection->get_remote_api<database_api_proxy>( 0, "database_api" );
         } catch( const fc::exception& e ) {
            elog( "Failed to connect to database_api: ${e}", ("e", e.to_detail_string()) );
            FC_THROW_EXCEPTION( fc::assert_exception, "database_api is required but not available" );
         }
      }
      return *_database_api;
   }

   fc::api<network_broadcast_api_proxy> get_network_broadcast_api()
   {
      if( !_network_broadcast_api )
      {
         try {
            _network_broadcast_api = _api_connection->get_remote_api<network_broadcast_api_proxy>( 0, "network_broadcast_api" );
         } catch( const fc::exception& e ) {
            elog( "Failed to connect to network_broadcast_api: ${e}", ("e", e.to_detail_string()) );
            FC_THROW_EXCEPTION( fc::assert_exception, "network_broadcast_api is required but not available" );
         }
      }
      return *_network_broadcast_api;
   }

   fc::api<account_by_key_api_proxy> get_account_by_key_api()
   {
      if( !_account_by_key_api )
      {
         try {
            _account_by_key_api = _api_connection->get_remote_api<account_by_key_api_proxy>( 0, "account_by_key_api" );
         } catch( const fc::exception& e ) {
            elog( "Failed to connect to account_by_key_api: ${e}", ("e", e.to_detail_string()) );
            FC_THROW_EXCEPTION( fc::assert_exception, "account_by_key_api is required but not available" );
         }
      }
      return *_account_by_key_api;
   }

   // Optional APIs - wallet can function with degraded capabilities if not available
   fc::optional<fc::api<block_api_proxy>> get_block_api()
   {
      if( !_block_api && !_block_api_failed )
      {
         try {
            _block_api = _api_connection->get_remote_api<block_api_proxy>( 0, "block_api" );
         } catch( const fc::exception& e ) {
            wlog( "block_api not available: ${e}", ("e", e.what()) );
            _block_api_failed = true;
         }
      }
      return _block_api;
   }

   fc::optional<fc::api<account_history_api_proxy>> get_account_history_api()
   {
      if( !_account_history_api && !_account_history_api_failed )
      {
         try {
            _account_history_api = _api_connection->get_remote_api<account_history_api_proxy>( 0, "account_history_api" );
         } catch( const fc::exception& e ) {
            wlog( "account_history_api not available: ${e}", ("e", e.what()) );
            _account_history_api_failed = true;
         }
      }
      return _account_history_api;
   }

   fc::optional<fc::api<follow_api_proxy>> get_follow_api()
   {
      if( !_follow_api && !_follow_api_failed )
      {
         try {
            _follow_api = _api_connection->get_remote_api<follow_api_proxy>( 0, "follow_api" );
         } catch( const fc::exception& e ) {
            wlog( "follow_api not available: ${e}", ("e", e.what()) );
            _follow_api_failed = true;
         }
      }
      return _follow_api;
   }

   fc::optional<fc::api<market_history_api_proxy>> get_market_history_api()
   {
      if( !_market_history_api && !_market_history_api_failed )
      {
         try {
            _market_history_api = _api_connection->get_remote_api<market_history_api_proxy>( 0, "market_history_api" );
         } catch( const fc::exception& e ) {
            wlog( "market_history_api not available: ${e}", ("e", e.what()) );
            _market_history_api_failed = true;
         }
      }
      return _market_history_api;
   }

   fc::optional<fc::api<tags_api_proxy>> get_tags_api()
   {
      if( !_tags_api && !_tags_api_failed )
      {
         try {
            _tags_api = _api_connection->get_remote_api<tags_api_proxy>( 0, "tags_api" );
         } catch( const fc::exception& e ) {
            wlog( "tags_api not available: ${e}", ("e", e.what()) );
            _tags_api_failed = true;
         }
      }
      return _tags_api;
   }

   // Check which APIs are available
   bool is_block_api_available() const { return _block_api.valid() && !_block_api_failed; }
   bool is_account_history_api_available() const { return _account_history_api.valid() && !_account_history_api_failed; }
   bool is_follow_api_available() const { return _follow_api.valid() && !_follow_api_failed; }
   bool is_market_history_api_available() const { return _market_history_api.valid() && !_market_history_api_failed; }
   bool is_tags_api_available() const { return _tags_api.valid() && !_tags_api_failed; }

private:
   std::shared_ptr<fc::rpc::websocket_api_connection> _api_connection;

   // Required APIs
   fc::optional<fc::api<database_api_proxy>>            _database_api;
   fc::optional<fc::api<network_broadcast_api_proxy>>   _network_broadcast_api;
   fc::optional<fc::api<account_by_key_api_proxy>>      _account_by_key_api;

   // Optional APIs
   fc::optional<fc::api<block_api_proxy>>               _block_api;
   fc::optional<fc::api<account_history_api_proxy>>     _account_history_api;
   fc::optional<fc::api<follow_api_proxy>>              _follow_api;
   fc::optional<fc::api<market_history_api_proxy>>      _market_history_api;
   fc::optional<fc::api<tags_api_proxy>>                _tags_api;

   // Failure tracking for optional APIs
   bool _block_api_failed = false;
   bool _account_history_api_failed = false;
   bool _follow_api_failed = false;
   bool _market_history_api_failed = false;
   bool _tags_api_failed = false;
};

} } // steem::wallet
