#pragma once

#include <steem/plugins/market_history_api/market_history_api.hpp>

namespace steem { namespace wallet {

using namespace steem::plugins::market_history;

/**
 * Remote market history API for wallet
 * This is a dummy API that provides method signatures for fc::api RPC calls
 */
struct market_history_api
{
   get_ticker_return get_ticker();
   get_volume_return get_volume();
   get_order_book_return get_order_book( get_order_book_args );
   get_trade_history_return get_trade_history( get_trade_history_args );
   get_recent_trades_return get_recent_trades( get_recent_trades_args );
   get_market_history_return get_market_history( get_market_history_args );
   get_market_history_buckets_return get_market_history_buckets();
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
