#pragma once

#include <steem/plugins/database_api/database_api.hpp>

namespace steem { namespace wallet {

using namespace steem::plugins::database_api;

/**
 * Remote database API for wallet
 * This is a dummy API that provides method signatures for fc::api RPC calls
 */
struct database_api
{
   // Globals
   get_config_return get_config();
   get_dynamic_global_properties_return get_dynamic_global_properties();
   get_witness_schedule_return get_witness_schedule();
   get_hardfork_properties_return get_hardfork_properties();
   get_reward_funds_return get_reward_funds( get_reward_funds_args );
   get_current_price_feed_return get_current_price_feed();
   get_feed_history_return get_feed_history();

   // Witnesses
   list_witnesses_return list_witnesses( list_witnesses_args );
   find_witnesses_return find_witnesses( find_witnesses_args );
   list_witness_votes_return list_witness_votes( list_witness_votes_args );
   get_active_witnesses_return get_active_witnesses();

   // Accounts
   list_accounts_return list_accounts( list_accounts_args );
   find_accounts_return find_accounts( find_accounts_args );
   list_owner_histories_return list_owner_histories( list_owner_histories_args );
   find_owner_histories_return find_owner_histories( find_owner_histories_args );
   list_account_recovery_requests_return list_account_recovery_requests( list_account_recovery_requests_args );
   find_account_recovery_requests_return find_account_recovery_requests( find_account_recovery_requests_args );
   list_change_recovery_account_requests_return list_change_recovery_account_requests( list_change_recovery_account_requests_args );
   find_change_recovery_account_requests_return find_change_recovery_account_requests( find_change_recovery_account_requests_args );
   list_escrows_return list_escrows( list_escrows_args );
   find_escrows_return find_escrows( find_escrows_args );
   list_withdraw_vesting_routes_return list_withdraw_vesting_routes( list_withdraw_vesting_routes_args );
   find_withdraw_vesting_routes_return find_withdraw_vesting_routes( find_withdraw_vesting_routes_args );
   list_savings_withdrawals_return list_savings_withdrawals( list_savings_withdrawals_args );
   find_savings_withdrawals_return find_savings_withdrawals( find_savings_withdrawals_args );
   list_vesting_delegations_return list_vesting_delegations( list_vesting_delegations_args );
   find_vesting_delegations_return find_vesting_delegations( find_vesting_delegations_args );
   list_vesting_delegation_expirations_return list_vesting_delegation_expirations( list_vesting_delegation_expirations_args );
   find_vesting_delegation_expirations_return find_vesting_delegation_expirations( find_vesting_delegation_expirations_args );
   list_sbd_conversion_requests_return list_sbd_conversion_requests( list_sbd_conversion_requests_args );
   find_sbd_conversion_requests_return find_sbd_conversion_requests( find_sbd_conversion_requests_args );
   list_decline_voting_rights_requests_return list_decline_voting_rights_requests( list_decline_voting_rights_requests_args );
   find_decline_voting_rights_requests_return find_decline_voting_rights_requests( find_decline_voting_rights_requests_args );

   // Comments
   list_comments_return list_comments( list_comments_args );
   find_comments_return find_comments( find_comments_args );
   list_votes_return list_votes( list_votes_args );
   find_votes_return find_votes( find_votes_args );

   // Market
   list_limit_orders_return list_limit_orders( list_limit_orders_args );
   find_limit_orders_return find_limit_orders( find_limit_orders_args );
   get_order_book_return get_order_book( get_order_book_args );

   // Authority / Validation
   get_transaction_hex_return get_transaction_hex( get_transaction_hex_args );
   get_required_signatures_return get_required_signatures( get_required_signatures_args );
   get_potential_signatures_return get_potential_signatures( get_potential_signatures_args );
   verify_authority_return verify_authority( verify_authority_args );
   verify_account_authority_return verify_account_authority( verify_account_authority_args );
   verify_signatures_return verify_signatures( verify_signatures_args );

   // SMT (if enabled)
   #ifdef STEEM_ENABLE_SMT
   get_nai_pool_return get_nai_pool( get_nai_pool_args );
   list_smt_contributions_return list_smt_contributions( list_smt_contributions_args );
   find_smt_contributions_return find_smt_contributions( find_smt_contributions_args );
   list_smt_tokens_return list_smt_tokens( list_smt_tokens_args );
   find_smt_tokens_return find_smt_tokens( list_smt_tokens_args );
   list_smt_token_emissions_return list_smt_token_emissions( list_smt_token_emissions_args );
   find_smt_token_emissions_return find_smt_token_emissions( find_smt_token_emissions_args );
   #endif
};

} } // steem::wallet

FC_API( steem::wallet::database_api,
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

   // SMT (if enabled)
   #ifdef STEEM_ENABLE_SMT
   (get_nai_pool)
   (list_smt_contributions)
   (find_smt_contributions)
   (list_smt_tokens)
   (find_smt_tokens)
   (list_smt_token_emissions)
   (find_smt_token_emissions)
   #endif
)
