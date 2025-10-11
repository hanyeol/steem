#pragma once

#include <steem/plugins/market_history_api/market_history_api.hpp>

namespace steem { namespace wallet {

namespace market_history_api_ns = steem::plugins::market_history;

/**
 * Remote market history API for wallet
 * This is a dummy API that provides method signatures for fc::api RPC calls
 */
struct market_history_api
{
   market_history_api_ns::get_ticker_return get_ticker();
   market_history_api_ns::get_volume_return get_volume();
   market_history_api_ns::get_order_book_return get_order_book( market_history_api_ns::get_order_book_args );
   market_history_api_ns::get_trade_history_return get_trade_history( market_history_api_ns::get_trade_history_args );
   market_history_api_ns::get_recent_trades_return get_recent_trades( market_history_api_ns::get_recent_trades_args );
   market_history_api_ns::get_market_history_return get_market_history( market_history_api_ns::get_market_history_args );
   market_history_api_ns::get_market_history_buckets_return get_market_history_buckets();
};

} } // steem::wallet

FC_API( steem::wallet::market_history_api,
   (get_ticker)
   (get_volume)
   (get_order_book)
   (get_trade_history)
   (get_recent_trades)
   (get_market_history)
   (get_market_history_buckets)
)
