#pragma once

#include <steem/plugins/follow_api/follow_api.hpp>

namespace steem { namespace wallet {

namespace follow_api_ns = steem::plugins::follow;

/**
 * Remote follow API for wallet
 * This is a dummy API that provides method signatures for fc::api RPC calls
 */
struct follow_api
{
   follow_api_ns::get_followers_return get_followers( follow_api_ns::get_followers_args );
   follow_api_ns::get_following_return get_following( follow_api_ns::get_following_args );
   follow_api_ns::get_follow_count_return get_follow_count( follow_api_ns::get_follow_count_args );
   follow_api_ns::get_feed_entries_return get_feed_entries( follow_api_ns::get_feed_entries_args );
   follow_api_ns::get_feed_return get_feed( follow_api_ns::get_feed_args );
   follow_api_ns::get_blog_entries_return get_blog_entries( follow_api_ns::get_blog_entries_args );
   follow_api_ns::get_blog_return get_blog( follow_api_ns::get_blog_args );
   follow_api_ns::get_account_reputations_return get_account_reputations( follow_api_ns::get_account_reputations_args );
   follow_api_ns::get_reblogged_by_return get_reblogged_by( follow_api_ns::get_reblogged_by_args );
   follow_api_ns::get_blog_authors_return get_blog_authors( follow_api_ns::get_blog_authors_args );
};

} } // steem::wallet

FC_API( steem::wallet::follow_api,
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
)
