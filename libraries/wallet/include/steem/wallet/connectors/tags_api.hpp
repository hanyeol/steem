#pragma once

#include <steem/plugins/tags_api/tags_api.hpp>

namespace steem { namespace wallet {

namespace tags_api_ns = steem::plugins::tags;

/**
 * Remote tags API for wallet
 * This is a dummy API that provides method signatures for fc::api RPC calls
 */
struct tags_api_proxy
{
   tags_api_ns::get_trending_tags_return get_trending_tags( tags_api_ns::get_trending_tags_args );
   tags_api_ns::get_tags_used_by_author_return get_tags_used_by_author( tags_api_ns::get_tags_used_by_author_args );
   tags_api_ns::get_discussion_return get_discussion( tags_api_ns::get_discussion_args );
   tags_api_ns::get_content_replies_return get_content_replies( tags_api_ns::get_content_replies_args );
   tags_api_ns::get_post_discussions_by_payout_return get_post_discussions_by_payout( tags_api_ns::get_post_discussions_by_payout_args );
   tags_api_ns::get_comment_discussions_by_payout_return get_comment_discussions_by_payout( tags_api_ns::get_comment_discussions_by_payout_args );
   tags_api_ns::get_discussions_by_trending_return get_discussions_by_trending( tags_api_ns::get_discussions_by_trending_args );
   tags_api_ns::get_discussions_by_created_return get_discussions_by_created( tags_api_ns::get_discussions_by_created_args );
   tags_api_ns::get_discussions_by_active_return get_discussions_by_active( tags_api_ns::get_discussions_by_active_args );
   tags_api_ns::get_discussions_by_cashout_return get_discussions_by_cashout( tags_api_ns::get_discussions_by_cashout_args );
   tags_api_ns::get_discussions_by_votes_return get_discussions_by_votes( tags_api_ns::get_discussions_by_votes_args );
   tags_api_ns::get_discussions_by_children_return get_discussions_by_children( tags_api_ns::get_discussions_by_children_args );
   tags_api_ns::get_discussions_by_hot_return get_discussions_by_hot( tags_api_ns::get_discussions_by_hot_args );
   tags_api_ns::get_discussions_by_feed_return get_discussions_by_feed( tags_api_ns::get_discussions_by_feed_args );
   tags_api_ns::get_discussions_by_blog_return get_discussions_by_blog( tags_api_ns::get_discussions_by_blog_args );
   tags_api_ns::get_discussions_by_comments_return get_discussions_by_comments( tags_api_ns::get_discussions_by_comments_args );
   tags_api_ns::get_discussions_by_promoted_return get_discussions_by_promoted( tags_api_ns::get_discussions_by_promoted_args );
   tags_api_ns::get_discussions_by_author_before_date_return get_discussions_by_author_before_date( tags_api_ns::get_discussions_by_author_before_date_args );
   tags_api_ns::get_replies_by_last_update_return get_replies_by_last_update( tags_api_ns::get_replies_by_last_update_args );
   tags_api_ns::get_active_votes_return get_active_votes( tags_api_ns::get_active_votes_args );
};

} } // steem::wallet

FC_API( steem::wallet::tags_api_proxy,
   (get_trending_tags)
   (get_tags_used_by_author)
   (get_discussion)
   (get_content_replies)
   (get_post_discussions_by_payout)
   (get_comment_discussions_by_payout)
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
   (get_active_votes)
)
