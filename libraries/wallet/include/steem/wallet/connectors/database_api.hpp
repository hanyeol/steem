#pragma once

#include <steem/plugins/database_api/database_api.hpp>

namespace steem { namespace wallet {

namespace database_api_ns = steem::plugins::database_api;

/**
 * Remote database API for wallet
 * This is a dummy API that provides method signatures for fc::api RPC calls
 */
struct database_api_proxy
{
   // Globals
   database_api_ns::get_config_return get_config();
   database_api_ns::get_dynamic_global_properties_return get_dynamic_global_properties();
   database_api_ns::get_witness_schedule_return get_witness_schedule();
   database_api_ns::get_hardfork_properties_return get_hardfork_properties();
   database_api_ns::get_reward_funds_return get_reward_funds( database_api_ns::get_reward_funds_args );
   database_api_ns::get_current_price_feed_return get_current_price_feed();
   database_api_ns::get_feed_history_return get_feed_history();

   // Witnesses
   database_api_ns::list_witnesses_return list_witnesses( database_api_ns::list_witnesses_args );
   database_api_ns::find_witnesses_return find_witnesses( database_api_ns::find_witnesses_args );
   database_api_ns::list_witness_votes_return list_witness_votes( database_api_ns::list_witness_votes_args );
   database_api_ns::get_active_witnesses_return get_active_witnesses();

   // Accounts
   database_api_ns::list_accounts_return list_accounts( database_api_ns::list_accounts_args );
   database_api_ns::find_accounts_return find_accounts( database_api_ns::find_accounts_args );
   database_api_ns::list_owner_histories_return list_owner_histories( database_api_ns::list_owner_histories_args );
   database_api_ns::find_owner_histories_return find_owner_histories( database_api_ns::find_owner_histories_args );
   database_api_ns::list_account_recovery_requests_return list_account_recovery_requests( database_api_ns::list_account_recovery_requests_args );
   database_api_ns::find_account_recovery_requests_return find_account_recovery_requests( database_api_ns::find_account_recovery_requests_args );
   database_api_ns::list_change_recovery_account_requests_return list_change_recovery_account_requests( database_api_ns::list_change_recovery_account_requests_args );
   database_api_ns::find_change_recovery_account_requests_return find_change_recovery_account_requests( database_api_ns::find_change_recovery_account_requests_args );
   database_api_ns::list_escrows_return list_escrows( database_api_ns::list_escrows_args );
   database_api_ns::find_escrows_return find_escrows( database_api_ns::find_escrows_args );
   database_api_ns::list_withdraw_vesting_routes_return list_withdraw_vesting_routes( database_api_ns::list_withdraw_vesting_routes_args );
   database_api_ns::find_withdraw_vesting_routes_return find_withdraw_vesting_routes( database_api_ns::find_withdraw_vesting_routes_args );
   database_api_ns::list_savings_withdrawals_return list_savings_withdrawals( database_api_ns::list_savings_withdrawals_args );
   database_api_ns::find_savings_withdrawals_return find_savings_withdrawals( database_api_ns::find_savings_withdrawals_args );
   database_api_ns::list_vesting_delegations_return list_vesting_delegations( database_api_ns::list_vesting_delegations_args );
   database_api_ns::find_vesting_delegations_return find_vesting_delegations( database_api_ns::find_vesting_delegations_args );
   database_api_ns::list_vesting_delegation_expirations_return list_vesting_delegation_expirations( database_api_ns::list_vesting_delegation_expirations_args );
   database_api_ns::find_vesting_delegation_expirations_return find_vesting_delegation_expirations( database_api_ns::find_vesting_delegation_expirations_args );
   database_api_ns::list_sbd_conversion_requests_return list_sbd_conversion_requests( database_api_ns::list_sbd_conversion_requests_args );
   database_api_ns::find_sbd_conversion_requests_return find_sbd_conversion_requests( database_api_ns::find_sbd_conversion_requests_args );
   database_api_ns::list_decline_voting_rights_requests_return list_decline_voting_rights_requests( database_api_ns::list_decline_voting_rights_requests_args );
   database_api_ns::find_decline_voting_rights_requests_return find_decline_voting_rights_requests( database_api_ns::find_decline_voting_rights_requests_args );

   // Comments
   database_api_ns::list_comments_return list_comments( database_api_ns::list_comments_args );
   database_api_ns::find_comments_return find_comments( database_api_ns::find_comments_args );
   database_api_ns::list_votes_return list_votes( database_api_ns::list_votes_args );
   database_api_ns::find_votes_return find_votes( database_api_ns::find_votes_args );

   // Market
   database_api_ns::list_limit_orders_return list_limit_orders( database_api_ns::list_limit_orders_args );
   database_api_ns::find_limit_orders_return find_limit_orders( database_api_ns::find_limit_orders_args );
   database_api_ns::get_order_book_return get_order_book( database_api_ns::get_order_book_args );

   // Authority / Validation
   database_api_ns::get_transaction_hex_return get_transaction_hex( database_api_ns::get_transaction_hex_args );
   database_api_ns::get_required_signatures_return get_required_signatures( database_api_ns::get_required_signatures_args );
   database_api_ns::get_potential_signatures_return get_potential_signatures( database_api_ns::get_potential_signatures_args );
   database_api_ns::verify_authority_return verify_authority( database_api_ns::verify_authority_args );
   database_api_ns::verify_account_authority_return verify_account_authority( database_api_ns::verify_account_authority_args );
   database_api_ns::verify_signatures_return verify_signatures( database_api_ns::verify_signatures_args );
};

} } // steem::wallet

FC_API( steem::wallet::database_api_proxy,
   // Globals
   (get_config)
   (get_dynamic_global_properties)
   (get_witness_schedule)
   (get_hardfork_properties)
   (get_reward_funds)
   (get_current_price_feed)
   (get_feed_history)

   // Witnesses
   (list_witnesses)
   (find_witnesses)
   (list_witness_votes)
   (get_active_witnesses)

   // Accounts
   (list_accounts)
   (find_accounts)
   (list_owner_histories)
   (find_owner_histories)
   (list_account_recovery_requests)
   (find_account_recovery_requests)
   (list_change_recovery_account_requests)
   (find_change_recovery_account_requests)
   (list_escrows)
   (find_escrows)
   (list_withdraw_vesting_routes)
   (find_withdraw_vesting_routes)
   (list_savings_withdrawals)
   (find_savings_withdrawals)
   (list_vesting_delegations)
   (find_vesting_delegations)
   (list_vesting_delegation_expirations)
   (find_vesting_delegation_expirations)
   (list_sbd_conversion_requests)
   (find_sbd_conversion_requests)
   (list_decline_voting_rights_requests)
   (find_decline_voting_rights_requests)

   // Comments
   (list_comments)
   (find_comments)
   (list_votes)
   (find_votes)

   // Market
   (list_limit_orders)
   (find_limit_orders)
   (get_order_book)

   // Authority / Validation
   (get_transaction_hex)
   (get_required_signatures)
   (get_potential_signatures)
   (verify_authority)
   (verify_account_authority)
   (verify_signatures)
)
