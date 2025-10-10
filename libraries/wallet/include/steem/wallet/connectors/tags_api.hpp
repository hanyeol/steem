#pragma once

#include <steem/plugins/tags_api/tags_api.hpp>

namespace steem { namespace wallet {

using namespace steem::plugins::tags;

/**
 * Remote tags API for wallet
 * This is a dummy API that provides method signatures for fc::api RPC calls
 */
struct tags_api
{
   get_trending_tags_return get_trending_tags( get_trending_tags_args );
   get_tags_used_by_author_return get_tags_used_by_author( get_tags_used_by_author_args );
   get_discussion_return get_discussion( get_discussion_args );
   get_content_replies_return get_content_replies( get_content_replies_args );
   get_discussions_by_trending_return get_discussions_by_trending( get_discussions_by_trending_args );
   get_discussions_by_created_return get_discussions_by_created( get_discussions_by_created_args );
   get_discussions_by_active_return get_discussions_by_active( get_discussions_by_active_args );
   get_discussions_by_cashout_return get_discussions_by_cashout( get_discussions_by_cashout_args );
   get_discussions_by_votes_return get_discussions_by_votes( get_discussions_by_votes_args );
   get_discussions_by_children_return get_discussions_by_children( get_discussions_by_children_args );
   get_discussions_by_hot_return get_discussions_by_hot( get_discussions_by_hot_args );
   get_discussions_by_feed_return get_discussions_by_feed( get_discussions_by_feed_args );
   get_discussions_by_blog_return get_discussions_by_blog( get_discussions_by_blog_args );
   get_discussions_by_comments_return get_discussions_by_comments( get_discussions_by_comments_args );
   get_discussions_by_promoted_return get_discussions_by_promoted( get_discussions_by_promoted_args );
   get_discussions_by_author_before_date_return get_discussions_by_author_before_date( get_discussions_by_author_before_date_args );
   get_replies_by_last_update_return get_replies_by_last_update( get_replies_by_last_update_args );
};

} } // steem::wallet

FC_API( steem::wallet::tags_api,
   (get_trending_tags)
   (get_tags_used_by_author)
   (get_discussion)
   (get_content_replies)
   (get_discussions_by_trending)
   (get_discussions_by_created)
   (get_discussions_by_active)
   (get_discussions_by_cashout)
   (get_discussions_by_votes)
   (get_discussions_by_children)
   (get_discussions_by_hot)
   (get_discussions_by_feed)
   (get_discussions_by_blog)
   (get_discussions_by_comments)
   (get_discussions_by_promoted)
   (get_discussions_by_author_before_date)
   (get_replies_by_last_update)
)
