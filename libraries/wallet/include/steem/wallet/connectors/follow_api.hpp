#pragma once

#include <steem/plugins/follow_api/follow_api.hpp>

namespace steem { namespace wallet {

using namespace steem::plugins::follow;

/**
 * Remote follow API for wallet
 * This is a dummy API that provides method signatures for fc::api RPC calls
 */
struct follow_api
{
   get_followers_return get_followers( get_followers_args );
   get_following_return get_following( get_following_args );
   get_follow_count_return get_follow_count( get_follow_count_args );
   get_feed_entries_return get_feed_entries( get_feed_entries_args );
   get_feed_return get_feed( get_feed_args );
   get_blog_entries_return get_blog_entries( get_blog_entries_args );
   get_blog_return get_blog( get_blog_args );
   get_account_reputations_return get_account_reputations( get_account_reputations_args );
   get_reblogged_by_return get_reblogged_by( get_reblogged_by_args );
   get_blog_authors_return get_blog_authors( get_blog_authors_args );
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
