#pragma once

#include <steem/plugins/account_history_api/account_history_api.hpp>

namespace steem { namespace wallet {

using namespace steem::plugins::account_history;

/**
 * Remote account history API for wallet
 * This is a dummy API that provides method signatures for fc::api RPC calls
 */
struct account_history_api
{
   get_ops_in_block_return get_ops_in_block( get_ops_in_block_args );
   get_transaction_return get_transaction( get_transaction_args );
   get_account_history_return get_account_history( get_account_history_args );
};

} } // steem::wallet

FC_API( steem::wallet::account_history_api,
   (get_ops_in_block)
   (get_transaction)
   (get_account_history)
)
