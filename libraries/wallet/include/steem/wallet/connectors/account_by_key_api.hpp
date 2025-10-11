#pragma once

#include <steem/plugins/account_by_key_api/account_by_key_api.hpp>

namespace steem { namespace wallet {

namespace account_by_key_api_ns = steem::plugins::account_by_key;

/**
 * Remote account by key API for wallet
 * This is a dummy API that provides method signatures for fc::api RPC calls
 */
struct account_by_key_api
{
   account_by_key_api_ns::get_key_references_return get_key_references( account_by_key_api_ns::get_key_references_args );
};

} } // steem::wallet

FC_API( steem::wallet::account_by_key_api,
   (get_key_references)
)
