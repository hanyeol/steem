#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <steem/protocol/exceptions.hpp>
#include <steem/protocol/hardfork.hpp>

#include <steem/chain/block_summary_object.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/database_exceptions.hpp>
#include <steem/chain/history_object.hpp>
#include <steem/chain/steem_objects.hpp>

#include <steem/chain/utils/reward.hpp>

#include <steem/plugins/debug_node/debug_node_plugin.hpp>
#include <steem/plugins/witness/witness_objects.hpp>

#include <fc/macros.hpp>
#include <fc/crypto/digest.hpp>

#include "../../fixtures/database_fixture.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace steem;
using namespace steem::chain;
using namespace steem::protocol;
using fc::string;

BOOST_FIXTURE_TEST_SUITE( operation_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( pow_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: pow_validate" );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( pow_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: pow_authorities" );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( pow_apply )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: pow_apply" );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif
