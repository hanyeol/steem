#pragma once

#include <steem/plugins/block_api/block_api.hpp>

namespace steem { namespace wallet {

using namespace steem::plugins::block_api;

/**
 * Remote block API for wallet
 * This is a dummy API that provides method signatures for fc::api RPC calls
 */
struct block_api
{
   get_block_header_return get_block_header( get_block_header_args );
   get_block_return get_block( get_block_args );
};

} } // steem::wallet

FC_API( steem::wallet::block_api,
   (get_block_header)
   (get_block)
)
