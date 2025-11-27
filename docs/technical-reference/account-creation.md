# Account Creation System

## Overview

The Steem blockchain provides two distinct mechanisms for creating new accounts: the basic **`account_create_operation`** and the more flexible **`account_create_with_delegation_operation`**. Both operations serve as anti-spam measures while enabling legitimate users to onboard new accounts.

This document explains the account creation fee structure, delegation-based cost reduction mechanism, and the implementation details of both operations.

## Table of Contents

1. [Account Creation Operations](#1-account-creation-operations)
2. [Fee Structure and Modifiers](#2-fee-structure-and-modifiers)
3. [Delegation-Based Account Creation](#3-delegation-based-account-creation)
4. [Fee Calculation Logic](#4-fee-calculation-logic)
5. [Implementation Details](#5-implementation-details)
6. [Configuration Parameters](#6-configuration-parameters)
7. [Examples](#7-examples)

---

## 1. Account Creation Operations

### 1.1 Basic Account Creation

**File:** [libraries/protocol/include/steem/protocol/steem_operations.hpp:12-25](../../libraries/protocol/include/steem/protocol/steem_operations.hpp#L12-L25)

```cpp
struct account_create_operation : public base_operation
{
   asset             fee;
   account_name_type creator;
   account_name_type new_account_name;
   authority         owner;
   authority         active;
   authority         posting;
   public_key_type   memo_key;
   string            json_metadata;

   void validate()const;
   void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(creator); }
};
```

**Key Characteristics:**
- Simple fee-only account creation
- Fee must be >= witness-determined `account_creation_fee`
- All fees are converted to vesting shares (SP) for the new account
- Requires active authority of creator account

### 1.2 Account Creation with Delegation

**File:** [libraries/protocol/include/steem/protocol/steem_operations.hpp:28-44](../../libraries/protocol/include/steem/protocol/steem_operations.hpp#L28-L44)

```cpp
struct account_create_with_delegation_operation : public base_operation
{
   asset             fee;
   asset             delegation;
   account_name_type creator;
   account_name_type new_account_name;
   authority         owner;
   authority         active;
   authority         posting;
   public_key_type   memo_key;
   string            json_metadata;

   extensions_type   extensions;

   void validate()const;
   void get_required_active_authorities( flat_set<account_name_type>& a )const{ a.insert(creator); }
};
```

**Key Characteristics:**
- Allows **lower fee** by providing vesting delegation
- Delegation is temporary (locked for 30 days minimum)
- Enables cost-effective onboarding for legitimate services
- Fee + delegation must meet minimum threshold

---

## 2. Fee Structure and Modifiers

### 2.1 Base Account Creation Fee

The base account creation fee is determined by the **median of all 21 active witnesses' proposals**.

**File:** [libraries/chain/include/steem/chain/witness_objects.hpp:34](../../libraries/chain/include/steem/chain/witness_objects.hpp#L34)

```cpp
struct chain_properties {
   asset account_creation_fee = asset( STEEM_MIN_ACCOUNT_CREATION_FEE, STEEM_SYMBOL );
   // ...
};
```

**Constraints:**
- **Testnet minimum:** `0 STEEM` (for testing convenience)
- **Mainnet minimum:** `1 STEEM` (protocol-enforced anti-spam)
- Witnesses can propose higher fees to combat spam

### 2.2 Fee Modifier Constants

**File:** [libraries/protocol/include/steem/protocol/config.hpp:132-134](../../libraries/protocol/include/steem/protocol/config.hpp#L132-L134)

```cpp
#define STEEM_CREATE_ACCOUNT_WITH_STEEM_MODIFIER 30
#define STEEM_CREATE_ACCOUNT_DELEGATION_RATIO    5
#define STEEM_CREATE_ACCOUNT_DELEGATION_TIME     fc::days(30)
```

| Constant | Value | Purpose |
|----------|-------|---------|
| `STEEM_CREATE_ACCOUNT_WITH_STEEM_MODIFIER` | 30 | Multiplier for full-fee account creation |
| `STEEM_CREATE_ACCOUNT_DELEGATION_RATIO` | 5 | Ratio for converting fee to delegation equivalent |
| `STEEM_CREATE_ACCOUNT_DELEGATION_TIME` | 30 days | Minimum delegation lock period |

### 2.3 Why the 30x Modifier?

The `STEEM_CREATE_ACCOUNT_WITH_STEEM_MODIFIER` serves two critical purposes:

1. **Spam Prevention:** Makes mass account creation economically prohibitive
   - Without delegation: Must pay `30 × account_creation_fee`
   - Example: If base fee = 3 STEEM, actual cost = 90 STEEM per account

2. **Incentivizes Delegation Mechanism:**
   - Services can reduce costs by providing vesting delegation
   - Encourages long-term resource commitment instead of pure fee payment
   - Creates economic balance between spam prevention and legitimate onboarding

---

## 3. Delegation-Based Account Creation

### 3.1 Target Delegation Formula

**File:** [libraries/chain/steem_evaluator.cpp:320](../../libraries/chain/steem_evaluator.cpp#L320)

```cpp
auto target_delegation = asset(
   wso.median_props.account_creation_fee.amount
   * STEEM_CREATE_ACCOUNT_WITH_STEEM_MODIFIER
   * STEEM_CREATE_ACCOUNT_DELEGATION_RATIO,
   STEEM_SYMBOL
) * props.get_vesting_share_price();
```

**Breakdown:**
```
target_delegation = account_creation_fee × 30 × 5 × vesting_price
                  = account_creation_fee × 150 × vesting_price
```

### 3.2 Current Delegation Formula

**File:** [libraries/chain/steem_evaluator.cpp:322](../../libraries/chain/steem_evaluator.cpp#L322)

```cpp
auto current_delegation = asset(
   o.fee.amount * STEEM_CREATE_ACCOUNT_DELEGATION_RATIO,
   STEEM_SYMBOL
) * props.get_vesting_share_price() + o.delegation;
```

**Breakdown:**
```
current_delegation = (fee × 5 × vesting_price) + delegation
```

### 3.3 Validation Logic

**File:** [libraries/chain/steem_evaluator.cpp:324-329](../../libraries/chain/steem_evaluator.cpp#L324-L329)

```cpp
FC_ASSERT( current_delegation >= target_delegation,
   "Insufficient Delegation ${f} required, ${p} provided.",
   ("f", target_delegation )
   ("p", current_delegation )
   ("account_creation_fee", wso.median_props.account_creation_fee )
   ("o.fee", o.fee )
   ("o.delegation", o.delegation ) );
```

**Requirements:**
1. `current_delegation >= target_delegation`
2. `fee >= account_creation_fee` (minimum base fee always required)
3. Creator must have sufficient balance for fee
4. Creator must have sufficient vesting shares for delegation

---

## 4. Fee Calculation Logic

### 4.1 Scenario Comparison

Assume:
- `account_creation_fee` = 3 STEEM
- `vesting_price` = 1000 VESTS per STEEM

| Method | Fee | Delegation | Total Cost (STEEM equivalent) |
|--------|-----|------------|-------------------------------|
| Basic `account_create` | 90 STEEM | 0 VESTS | 90 STEEM |
| With delegation (minimum fee) | 3 STEEM | 435,000 VESTS | 3 STEEM + 435 SP locked |
| With delegation (balanced) | 30 STEEM | 300,000 VESTS | 30 STEEM + 300 SP locked |

### 4.2 Trade-off Analysis

**Full Fee Approach (90 STEEM):**
- ✅ No delegation required
- ✅ No lock-up period
- ❌ High upfront cost
- **Use case:** One-time account creation

**Delegation Approach (3 STEEM + delegation):**
- ✅ Low liquid STEEM cost
- ✅ Delegation recoverable after 30 days
- ❌ Requires significant SP holdings
- ❌ SP locked for minimum 30 days
- **Use case:** Services onboarding many users

---

## 5. Implementation Details

### 5.1 Basic Account Creation Evaluator

**File:** [libraries/chain/steem_evaluator.cpp:256-304](../../libraries/chain/steem_evaluator.cpp#L256-L304)

```cpp
void account_create_evaluator::do_apply( const account_create_operation& o )
{
   const auto& creator = _db.get_account( o.creator );
   const auto& props = _db.get_dynamic_global_properties();

   // 1. Validate creator has sufficient balance
   FC_ASSERT( creator.balance >= o.fee, "Insufficient balance to create account.",
              ( "creator.balance", creator.balance )( "required", o.fee ) );

   // 2. Validate fee meets minimum requirement
   const witness_schedule_object& wso = _db.get_witness_schedule_object();
   FC_ASSERT( o.fee >= wso.median_props.account_creation_fee,
              "Insufficient Fee: ${f} required, ${p} provided.",
              ("f", wso.median_props.account_creation_fee)("p", o.fee) );

   // 3. Validate authority structure sizes
   if( _db.is_producing() )
   {
      validate_auth_size( o.owner );
      validate_auth_size( o.active );
      validate_auth_size( o.posting );
   }

   // 4. Verify all authority accounts exist
   verify_authority_accounts_exist( _db, o.owner, o.new_account_name, authority::owner );
   verify_authority_accounts_exist( _db, o.active, o.new_account_name, authority::active );
   verify_authority_accounts_exist( _db, o.posting, o.new_account_name, authority::posting );

   // 5. Deduct fee from creator
   _db.modify( creator, [&]( account_object& c ){
      c.balance -= o.fee;
   });

   // 6. Create new account
   const auto& new_account = _db.create< account_object >( [&]( account_object& acc )
   {
      initialize_account_object( acc, o.new_account_name, o.memo_key, props,
                                 false /*mined*/, o.creator, _db.get_hardfork() );
      #ifndef IS_LOW_MEM
         from_string( acc.json_metadata, o.json_metadata );
      #endif
   });

   // 7. Create authority object
   _db.create< account_authority_object >( [&]( account_authority_object& auth )
   {
      auth.account = o.new_account_name;
      auth.owner = o.owner;
      auth.active = o.active;
      auth.posting = o.posting;
      auth.last_owner_update = fc::time_point_sec::min();
   });

   // 8. Convert fee to vesting shares for new account
   if( o.fee.amount > 0 )
      _db.create_vesting( new_account, o.fee );
}
```

**Execution Flow:**
1. Validate creator balance and fee amount
2. Check fee meets witness-determined minimum
3. Validate authority structures (production nodes only)
4. Verify all accounts in authorities exist
5. Deduct fee from creator's balance
6. Create new account object
7. Create authority object with owner/active/posting keys
8. Convert fee to vesting shares (SP) for new account

### 5.2 Delegation-Based Creation Evaluator

**File:** [libraries/chain/steem_evaluator.cpp:306-396](../../libraries/chain/steem_evaluator.cpp#L306-L396)

```cpp
void account_create_with_delegation_evaluator::do_apply(
   const account_create_with_delegation_operation& o )
{
   const auto& creator = _db.get_account( o.creator );
   const auto& props = _db.get_dynamic_global_properties();
   const witness_schedule_object& wso = _db.get_witness_schedule_object();

   // 1. Validate creator balance
   FC_ASSERT( creator.balance >= o.fee, "Insufficient balance to create account.",
              ( "creator.balance", creator.balance )( "required", o.fee ) );

   // 2. Validate creator has sufficient vesting for delegation
   FC_ASSERT( creator.vesting_shares
              - creator.delegated_vesting_shares
              - asset( creator.to_withdraw - creator.withdrawn, VESTS_SYMBOL )
              >= o.delegation,
              "Insufficient vesting shares to delegate to new account.",
              ( "creator.vesting_shares", creator.vesting_shares )
              ( "creator.delegated_vesting_shares", creator.delegated_vesting_shares )
              ( "required", o.delegation ) );

   // 3. Calculate target delegation requirement
   auto target_delegation = asset(
      wso.median_props.account_creation_fee.amount
      * STEEM_CREATE_ACCOUNT_WITH_STEEM_MODIFIER
      * STEEM_CREATE_ACCOUNT_DELEGATION_RATIO,
      STEEM_SYMBOL
   ) * props.get_vesting_share_price();

   // 4. Calculate current delegation provided
   auto current_delegation = asset(
      o.fee.amount * STEEM_CREATE_ACCOUNT_DELEGATION_RATIO,
      STEEM_SYMBOL
   ) * props.get_vesting_share_price() + o.delegation;

   // 5. Validate delegation meets requirement
   FC_ASSERT( current_delegation >= target_delegation,
              "Insufficient Delegation ${f} required, ${p} provided.",
              ("f", target_delegation )( "p", current_delegation )
              ( "account_creation_fee", wso.median_props.account_creation_fee )
              ( "o.fee", o.fee )( "o.delegation", o.delegation ) );

   // 6. Validate minimum fee
   FC_ASSERT( o.fee >= wso.median_props.account_creation_fee,
              "Insufficient Fee: ${f} required, ${p} provided.",
              ("f", wso.median_props.account_creation_fee)("p", o.fee) );

   // 7. Validate authority sizes
   if( _db.is_producing() )
   {
      validate_auth_size( o.owner );
      validate_auth_size( o.active );
      validate_auth_size( o.posting );
   }

   // 8. Verify authority accounts exist
   for( const auto& a : o.owner.account_auths )
      _db.get_account( a.first );
   for( const auto& a : o.active.account_auths )
      _db.get_account( a.first );
   for( const auto& a : o.posting.account_auths )
      _db.get_account( a.first );

   // 9. Deduct fee and update delegated vesting
   _db.modify( creator, [&]( account_object& c )
   {
      c.balance -= o.fee;
      c.delegated_vesting_shares += o.delegation;
   });

   // 10. Create new account with delegation
   const auto& new_account = _db.create< account_object >( [&]( account_object& acc )
   {
      initialize_account_object( acc, o.new_account_name, o.memo_key, props,
                                 false /*mined*/, o.creator, _db.get_hardfork() );
      acc.received_vesting_shares = o.delegation;

      #ifndef IS_LOW_MEM
         from_string( acc.json_metadata, o.json_metadata );
      #endif
   });

   // 11. Create authority object
   _db.create< account_authority_object >( [&]( account_authority_object& auth )
   {
      auth.account = o.new_account_name;
      auth.owner = o.owner;
      auth.active = o.active;
      auth.posting = o.posting;
      auth.last_owner_update = fc::time_point_sec::min();
   });

   // 12. Create delegation object with time lock
   if( o.delegation.amount > 0 )
   {
      _db.create< vesting_delegation_object >( [&]( vesting_delegation_object& vdo )
      {
         vdo.delegator = o.creator;
         vdo.delegatee = o.new_account_name;
         vdo.vesting_shares = o.delegation;
         vdo.min_delegation_time = _db.head_block_time() + STEEM_CREATE_ACCOUNT_DELEGATION_TIME;
      });
   }

   // 13. Convert fee to vesting for new account
   if( o.fee.amount > 0 )
      _db.create_vesting( new_account, o.fee );
}
```

**Key Differences from Basic Creation:**
1. Validates creator has sufficient **vesting shares** for delegation
2. Calculates and validates **target vs. current delegation**
3. Updates creator's `delegated_vesting_shares`
4. Sets new account's `received_vesting_shares`
5. Creates `vesting_delegation_object` with **30-day minimum lock**
6. Delegation cannot be removed until after `min_delegation_time`

---

## 6. Configuration Parameters

### 6.1 Protocol Constants

**File:** [libraries/protocol/include/steem/protocol/config.hpp](../../libraries/protocol/include/steem/protocol/config.hpp)

#### Testnet Configuration (Line 27)
```cpp
#define STEEM_MIN_ACCOUNT_CREATION_FEE        0
```

#### Mainnet Configuration (Line 54)
```cpp
#define STEEM_MIN_ACCOUNT_CREATION_FEE        1
```

#### Account Creation Modifiers (Lines 132-134)
```cpp
#define STEEM_CREATE_ACCOUNT_WITH_STEEM_MODIFIER 30
#define STEEM_CREATE_ACCOUNT_DELEGATION_RATIO    5
#define STEEM_CREATE_ACCOUNT_DELEGATION_TIME     fc::days(30)
```

### 6.2 Dynamic Parameters (Witness-Controlled)

**File:** [libraries/protocol/include/steem/protocol/steem_operations.hpp:375](../../libraries/protocol/include/steem/protocol/steem_operations.hpp#L375)

```cpp
struct chain_properties
{
   legacy_steem_asset account_creation_fee =
      legacy_steem_asset::from_amount( STEEM_MIN_ACCOUNT_CREATION_FEE );
   // ...
};
```

**Witness Control:**
- Each of 21 active witnesses proposes their preferred `account_creation_fee`
- Blockchain uses the **median value** of all proposals
- Updates every witness schedule round (~63 seconds)
- Allows dynamic adjustment to combat spam or encourage growth

---

## 7. Examples

### 7.1 Example 1: Basic Account Creation

**Scenario:**
- Witness median `account_creation_fee` = 3 STEEM
- Creator wants to create 1 account using basic operation

**Calculation:**
```
Required fee = account_creation_fee (no modifier for basic operation)
             = 3 STEEM

Creator pays: 3 STEEM
New account receives: 3 STEEM worth of vesting shares (SP)
```

**Note:** The basic `account_create_operation` does **NOT** use the 30x modifier. That modifier only applies when using the wallet's convenience function or when calculating the delegation requirement for `account_create_with_delegation_operation`.

### 7.2 Example 2: Wallet-Based Account Creation

**Scenario:**
- Using `cli_wallet` to create account
- Witness median `account_creation_fee` = 3 STEEM

**File:** [libraries/wallet/wallet.cpp:1196](../../libraries/wallet/wallet.cpp#L1196)

```cpp
op.fee = my->_remote_api.get_chain_properties().account_creation_fee
       * asset( STEEM_CREATE_ACCOUNT_WITH_STEEM_MODIFIER, STEEM_SYMBOL );
```

**Calculation:**
```
Wallet fee = 3 STEEM × 30 = 90 STEEM
```

**Why?** The wallet uses `account_create_with_delegation_operation` with zero delegation, requiring the full 30x fee.

### 7.3 Example 3: Delegation-Based Creation (Minimum Fee)

**Scenario:**
- Witness median `account_creation_fee` = 3 STEEM
- Vesting price = 1000 VESTS per STEEM
- Creator wants minimum liquid STEEM cost

**Step 1: Calculate target delegation**
```
target_delegation = 3 STEEM × 30 × 5 × 1000 VESTS/STEEM
                  = 450,000 VESTS
```

**Step 2: Calculate with minimum fee**
```
current_delegation = (3 STEEM × 5 × 1000 VESTS/STEEM) + delegation
                   = 15,000 VESTS + delegation

Required: 15,000 + delegation >= 450,000
         delegation >= 435,000 VESTS
```

**Result:**
- Fee: **3 STEEM** (minimum)
- Delegation: **435,000 VESTS** (435 SP at 1000 VESTS/STEEM)
- Total cost: 3 liquid STEEM + 435 SP locked for 30 days

### 7.4 Example 4: Delegation-Based Creation (Balanced Approach)

**Scenario:**
- Same parameters as Example 3
- Creator prefers balanced fee/delegation split

**Option: Pay 30 STEEM fee**
```
current_delegation = (30 STEEM × 5 × 1000) + delegation
                   = 150,000 VESTS + delegation

Required: 150,000 + delegation >= 450,000
         delegation >= 300,000 VESTS
```

**Result:**
- Fee: **30 STEEM** (10x base fee)
- Delegation: **300,000 VESTS** (300 SP)
- Total cost: 30 liquid STEEM + 300 SP locked for 30 days

### 7.5 Example 5: High-Volume Service

**Scenario:**
- Service creating 1000 accounts per month
- Witness median `account_creation_fee` = 3 STEEM
- Vesting price = 1000 VESTS per STEEM

**Option A: Full Fee Approach**
```
Cost per account = 90 STEEM (using wallet default)
Monthly cost = 90 × 1000 = 90,000 STEEM
```

**Option B: Delegation Approach**
```
Cost per account = 3 STEEM + 435,000 VESTS
Monthly cost = 3,000 STEEM + 435,000,000 VESTS (435,000 SP)

After 30 days: Can reclaim delegation and reuse for new accounts
Ongoing cost = 3,000 STEEM per month + 435,000 SP one-time lock
```

**Savings with delegation:**
```
First month:  90,000 STEEM vs 3,000 STEEM + 435,000 SP
Second month: 90,000 STEEM vs 3,000 STEEM (reuse delegation)
Annual cost:  1,080,000 STEEM vs 36,000 STEEM + 435,000 SP
```

**Delegation approach saves ~96% on ongoing costs** for high-volume services.

### 7.6 Test Fixture Example

**File:** [tests/db_fixture/database_fixture.cpp:303](../../tests/db_fixture/database_fixture.cpp#L303)

```cpp
return account_create(
   name,
   STEEM_GENESIS_WITNESS_NAME,
   init_account_priv_key,
   std::max( db->get_witness_schedule_object().median_props.account_creation_fee.amount
             * STEEM_CREATE_ACCOUNT_WITH_STEEM_MODIFIER, share_type( 100 ) ),
   key,
   post_key,
   "" );
```

**Purpose:**
- Test helper function for creating accounts
- Uses higher of `(base_fee × 30)` or `100` to ensure sufficient fee
- Ensures tests work even with very low witness fees

---

## References

### Source Files

- **Operations:** [libraries/protocol/include/steem/protocol/steem_operations.hpp](../../libraries/protocol/include/steem/protocol/steem_operations.hpp)
- **Evaluators:** [libraries/chain/steem_evaluator.cpp](../../libraries/chain/steem_evaluator.cpp)
- **Configuration:** [libraries/protocol/include/steem/protocol/config.hpp](../../libraries/protocol/include/steem/protocol/config.hpp)
- **Wallet Implementation:** [libraries/wallet/wallet.cpp](../../libraries/wallet/wallet.cpp)
- **Test Fixture:** [tests/db_fixture/database_fixture.cpp](../../tests/db_fixture/database_fixture.cpp)

### Related Documentation

- [Chain Parameters](chain-parameters.md) - Witness-controlled blockchain parameters
- [Vesting System](vesting-system.md) - How vesting shares and delegation work
- [Creating New Operations](../development/create-operation.md) - How to add new operations

### External Resources

- [Steem Whitepaper](https://steem.com/steem-whitepaper.pdf) - Original design rationale
- [API Documentation](api-notes.md) - How to query account creation parameters via RPC
