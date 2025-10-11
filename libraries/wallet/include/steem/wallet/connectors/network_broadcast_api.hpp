#pragma once

#include <steem/plugins/network_broadcast_api/network_broadcast_api.hpp>

namespace steem { namespace wallet {

namespace network_broadcast_api_ns = steem::plugins::network_broadcast_api;

/**
 * Remote network broadcast API for wallet
 * This is a dummy API that provides method signatures for fc::api RPC calls
 */
struct network_broadcast_api
{
   network_broadcast_api_ns::broadcast_transaction_return broadcast_transaction( network_broadcast_api_ns::broadcast_transaction_args );
   network_broadcast_api_ns::broadcast_block_return broadcast_block( network_broadcast_api_ns::broadcast_block_args );
};

} } // steem::wallet

FC_API( steem::wallet::network_broadcast_api,
   (broadcast_transaction)
   (broadcast_block)
)
