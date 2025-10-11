#pragma once

#include <steem/plugins/account_history_api/account_history_api.hpp>

namespace steem { namespace wallet {

namespace account_history_api_ns = steem::plugins::account_history;

/**
 * Remote account history API for wallet
 * This is a dummy API that provides method signatures for fc::api RPC calls
 */
struct account_history_api
{
   account_history_api_ns::get_ops_in_block_return get_ops_in_block( account_history_api_ns::get_ops_in_block_args );
   account_history_api_ns::get_transaction_return get_transaction( account_history_api_ns::get_transaction_args );
   account_history_api_ns::get_account_history_return get_account_history( account_history_api_ns::get_account_history_args );
};

} } // steem::wallet

FC_API( steem::wallet::account_history_api,
   (get_ops_in_block)
   (get_transaction)
   (get_account_history)
)
