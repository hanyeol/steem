# 계정 생성 시스템

## 개요

Steem 블록체인은 새로운 계정을 생성하기 위한 두 가지 메커니즘을 제공합니다: 기본적인 **`account_create_operation`**과 보다 유연한 **`account_create_with_delegation_operation`**입니다. 두 작업 모두 스팸 방지 수단으로 작동하면서도 정당한 사용자의 온보딩을 가능하게 합니다.

이 문서는 계정 생성 수수료 구조, 위임 기반 비용 절감 메커니즘, 그리고 두 작업의 구현 세부사항을 설명합니다.

## 목차

1. [계정 생성 작업](#1-계정-생성-작업)
2. [수수료 구조와 수정자](#2-수수료-구조와-수정자)
3. [위임 기반 계정 생성](#3-위임-기반-계정-생성)
4. [수수료 계산 로직](#4-수수료-계산-로직)
5. [구현 세부사항](#5-구현-세부사항)
6. [설정 매개변수](#6-설정-매개변수)
7. [예시](#7-예시)

---

## 1. 계정 생성 작업

### 1.1 기본 계정 생성

**파일:** [src/core/protocol/include/steem/protocol/steem_operations.hpp:12-25](../../../src/core/protocol/include/steem/protocol/steem_operations.hpp#L12-L25)

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

**주요 특징:**
- 단순 수수료 기반 계정 생성
- 수수료는 witness가 결정한 `account_creation_fee` 이상이어야 함
- 모든 수수료는 새 계정의 vesting shares(SP)로 전환됨
- 생성자 계정의 active 권한 필요

### 1.2 위임을 포함한 계정 생성

**파일:** [src/core/protocol/include/steem/protocol/steem_operations.hpp:28-44](../../../src/core/protocol/include/steem/protocol/steem_operations.hpp#L28-L44)

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

**주요 특징:**
- vesting delegation을 제공하여 **낮은 수수료** 허용
- Delegation은 임시적 (최소 30일간 잠금)
- 정당한 서비스의 비용 효율적 온보딩 가능
- 수수료 + delegation이 최소 임계값을 충족해야 함

---

## 2. 수수료 구조와 수정자

### 2.1 기본 계정 생성 수수료

기본 계정 생성 수수료는 **활성화된 21명의 witness 제안의 중앙값**으로 결정됩니다.

**파일:** [src/core/chain/include/steem/chain/witness_objects.hpp:34](../../../src/core/chain/include/steem/chain/witness_objects.hpp#L34)

```cpp
struct chain_properties {
   asset account_creation_fee = asset( STEEM_MIN_ACCOUNT_CREATION_FEE, STEEM_SYMBOL );
   // ...
};
```

**제약 조건:**
- **테스트넷 최소값:** `0 STEEM` (테스트 편의를 위해)
- **메인넷 최소값:** `1 STEEM` (프로토콜 수준 스팸 방지)
- Witness들은 스팸 방지를 위해 더 높은 수수료를 제안할 수 있음

### 2.2 수수료 수정자 상수

**파일:** [src/core/protocol/include/steem/protocol/config.hpp:132-134](../../../src/core/protocol/include/steem/protocol/config.hpp#L132-L134)

```cpp
#define STEEM_CREATE_ACCOUNT_WITH_STEEM_MODIFIER 30
#define STEEM_CREATE_ACCOUNT_DELEGATION_RATIO    5
#define STEEM_CREATE_ACCOUNT_DELEGATION_TIME     fc::days(30)
```

| 상수 | 값 | 목적 |
|------|-----|------|
| `STEEM_CREATE_ACCOUNT_WITH_STEEM_MODIFIER` | 30 | 전액 수수료 계정 생성 배수 |
| `STEEM_CREATE_ACCOUNT_DELEGATION_RATIO` | 5 | 수수료를 delegation 등가물로 전환하는 비율 |
| `STEEM_CREATE_ACCOUNT_DELEGATION_TIME` | 30일 | 최소 delegation 잠금 기간 |

### 2.3 30배 수정자가 필요한 이유

`STEEM_CREATE_ACCOUNT_WITH_STEEM_MODIFIER`는 두 가지 중요한 목적을 수행합니다:

1. **스팸 방지:** 대량 계정 생성을 경제적으로 불가능하게 만듦
   - Delegation 없이: `30 × account_creation_fee` 지불 필요
   - 예시: 기본 수수료 = 3 STEEM이면, 실제 비용 = 계정당 90 STEEM

2. **Delegation 메커니즘 장려:**
   - 서비스는 vesting delegation을 제공하여 비용 절감 가능
   - 순수 수수료 지불 대신 장기적 리소스 투자 권장
   - 스팸 방지와 정당한 온보딩 사이의 경제적 균형 창출

---

## 3. 위임 기반 계정 생성

### 3.1 목표 Delegation 공식

**파일:** [src/core/chain/steem_evaluator.cpp:320](../../../src/core/chain/steem_evaluator.cpp#L320)

```cpp
auto target_delegation = asset(
   wso.median_props.account_creation_fee.amount
   * STEEM_CREATE_ACCOUNT_WITH_STEEM_MODIFIER
   * STEEM_CREATE_ACCOUNT_DELEGATION_RATIO,
   STEEM_SYMBOL
) * props.get_vesting_share_price();
```

**계산 과정:**
```
target_delegation = account_creation_fee × 30 × 5 × vesting_price
                  = account_creation_fee × 150 × vesting_price
```

### 3.2 현재 Delegation 공식

**파일:** [src/core/chain/steem_evaluator.cpp:322](../../../src/core/chain/steem_evaluator.cpp#L322)

```cpp
auto current_delegation = asset(
   o.fee.amount * STEEM_CREATE_ACCOUNT_DELEGATION_RATIO,
   STEEM_SYMBOL
) * props.get_vesting_share_price() + o.delegation;
```

**계산 과정:**
```
current_delegation = (fee × 5 × vesting_price) + delegation
```

### 3.3 검증 로직

**파일:** [src/core/chain/steem_evaluator.cpp:324-329](../../../src/core/chain/steem_evaluator.cpp#L324-L329)

```cpp
FC_ASSERT( current_delegation >= target_delegation,
   "Insufficient Delegation ${f} required, ${p} provided.",
   ("f", target_delegation )
   ("p", current_delegation )
   ("account_creation_fee", wso.median_props.account_creation_fee )
   ("o.fee", o.fee )
   ("o.delegation", o.delegation ) );
```

**요구사항:**
1. `current_delegation >= target_delegation`
2. `fee >= account_creation_fee` (최소 기본 수수료 항상 필요)
3. 생성자는 수수료에 충분한 잔액 보유 필요
4. 생성자는 delegation에 충분한 vesting shares 보유 필요

---

## 4. 수수료 계산 로직

### 4.1 시나리오 비교

가정:
- `account_creation_fee` = 3 STEEM
- `vesting_price` = STEEM당 1000 VESTS

| 방법 | 수수료 | Delegation | 총 비용 (STEEM 환산) |
|------|--------|------------|---------------------|
| 기본 `account_create` | 90 STEEM | 0 VESTS | 90 STEEM |
| Delegation 포함 (최소 수수료) | 3 STEEM | 435,000 VESTS | 3 STEEM + 435 SP 잠금 |
| Delegation 포함 (균형) | 30 STEEM | 300,000 VESTS | 30 STEEM + 300 SP 잠금 |

### 4.2 장단점 분석

**전액 수수료 방식 (90 STEEM):**
- ✅ Delegation 불필요
- ✅ 잠금 기간 없음
- ❌ 높은 초기 비용
- **사용 사례:** 일회성 계정 생성

**Delegation 방식 (3 STEEM + delegation):**
- ✅ 낮은 유동 STEEM 비용
- ✅ 30일 후 delegation 회수 가능
- ❌ 상당한 SP 보유 필요
- ❌ 최소 30일간 SP 잠금
- **사용 사례:** 다수 사용자를 온보딩하는 서비스

---

## 5. 구현 세부사항

### 5.1 기본 계정 생성 평가자

**파일:** [src/core/chain/steem_evaluator.cpp:256-304](../../../src/core/chain/steem_evaluator.cpp#L256-L304)

```cpp
void account_create_evaluator::do_apply( const account_create_operation& o )
{
   const auto& creator = _db.get_account( o.creator );
   const auto& props = _db.get_dynamic_global_properties();

   // 1. 생성자의 충분한 잔액 검증
   FC_ASSERT( creator.balance >= o.fee, "Insufficient balance to create account.",
              ( "creator.balance", creator.balance )( "required", o.fee ) );

   // 2. 수수료가 최소 요구사항을 충족하는지 검증
   const witness_schedule_object& wso = _db.get_witness_schedule_object();
   FC_ASSERT( o.fee >= wso.median_props.account_creation_fee,
              "Insufficient Fee: ${f} required, ${p} provided.",
              ("f", wso.median_props.account_creation_fee)("p", o.fee) );

   // 3. 권한 구조 크기 검증
   if( _db.is_producing() )
   {
      validate_auth_size( o.owner );
      validate_auth_size( o.active );
      validate_auth_size( o.posting );
   }

   // 4. 모든 권한 계정이 존재하는지 확인
   verify_authority_accounts_exist( _db, o.owner, o.new_account_name, authority::owner );
   verify_authority_accounts_exist( _db, o.active, o.new_account_name, authority::active );
   verify_authority_accounts_exist( _db, o.posting, o.new_account_name, authority::posting );

   // 5. 생성자로부터 수수료 차감
   _db.modify( creator, [&]( account_object& c ){
      c.balance -= o.fee;
   });

   // 6. 새 계정 생성
   const auto& new_account = _db.create< account_object >( [&]( account_object& acc )
   {
      initialize_account_object( acc, o.new_account_name, o.memo_key, props,
                                 false /*mined*/, o.creator, _db.get_hardfork() );
      #ifndef IS_LOW_MEM
         from_string( acc.json_metadata, o.json_metadata );
      #endif
   });

   // 7. 권한 객체 생성
   _db.create< account_authority_object >( [&]( account_authority_object& auth )
   {
      auth.account = o.new_account_name;
      auth.owner = o.owner;
      auth.active = o.active;
      auth.posting = o.posting;
      auth.last_owner_update = fc::time_point_sec::min();
   });

   // 8. 새 계정을 위해 수수료를 vesting shares로 전환
   if( o.fee.amount > 0 )
      _db.create_vesting( new_account, o.fee );
}
```

**실행 흐름:**
1. 생성자 잔액과 수수료 금액 검증
2. 수수료가 witness가 결정한 최소값을 충족하는지 확인
3. 권한 구조 검증 (프로덕션 노드만)
4. 권한 내 모든 계정이 존재하는지 확인
5. 생성자의 잔액에서 수수료 차감
6. 새 계정 객체 생성
7. owner/active/posting 키를 가진 권한 객체 생성
8. 새 계정을 위해 수수료를 vesting shares(SP)로 전환

### 5.2 Delegation 기반 생성 평가자

**파일:** [src/core/chain/steem_evaluator.cpp:306-396](../../../src/core/chain/steem_evaluator.cpp#L306-L396)

```cpp
void account_create_with_delegation_evaluator::do_apply(
   const account_create_with_delegation_operation& o )
{
   const auto& creator = _db.get_account( o.creator );
   const auto& props = _db.get_dynamic_global_properties();
   const witness_schedule_object& wso = _db.get_witness_schedule_object();

   // 1. 생성자 잔액 검증
   FC_ASSERT( creator.balance >= o.fee, "Insufficient balance to create account.",
              ( "creator.balance", creator.balance )( "required", o.fee ) );

   // 2. 생성자가 delegation에 충분한 vesting을 보유하는지 검증
   FC_ASSERT( creator.vesting_shares
              - creator.delegated_vesting_shares
              - asset( creator.to_withdraw - creator.withdrawn, VESTS_SYMBOL )
              >= o.delegation,
              "Insufficient vesting shares to delegate to new account.",
              ( "creator.vesting_shares", creator.vesting_shares )
              ( "creator.delegated_vesting_shares", creator.delegated_vesting_shares )
              ( "required", o.delegation ) );

   // 3. 목표 delegation 요구사항 계산
   auto target_delegation = asset(
      wso.median_props.account_creation_fee.amount
      * STEEM_CREATE_ACCOUNT_WITH_STEEM_MODIFIER
      * STEEM_CREATE_ACCOUNT_DELEGATION_RATIO,
      STEEM_SYMBOL
   ) * props.get_vesting_share_price();

   // 4. 제공된 현재 delegation 계산
   auto current_delegation = asset(
      o.fee.amount * STEEM_CREATE_ACCOUNT_DELEGATION_RATIO,
      STEEM_SYMBOL
   ) * props.get_vesting_share_price() + o.delegation;

   // 5. Delegation이 요구사항을 충족하는지 검증
   FC_ASSERT( current_delegation >= target_delegation,
              "Insufficient Delegation ${f} required, ${p} provided.",
              ("f", target_delegation )( "p", current_delegation )
              ( "account_creation_fee", wso.median_props.account_creation_fee )
              ( "o.fee", o.fee )( "o.delegation", o.delegation ) );

   // 6. 최소 수수료 검증
   FC_ASSERT( o.fee >= wso.median_props.account_creation_fee,
              "Insufficient Fee: ${f} required, ${p} provided.",
              ("f", wso.median_props.account_creation_fee)("p", o.fee) );

   // 7. 권한 크기 검증
   if( _db.is_producing() )
   {
      validate_auth_size( o.owner );
      validate_auth_size( o.active );
      validate_auth_size( o.posting );
   }

   // 8. 권한 계정 존재 확인
   for( const auto& a : o.owner.account_auths )
      _db.get_account( a.first );
   for( const auto& a : o.active.account_auths )
      _db.get_account( a.first );
   for( const auto& a : o.posting.account_auths )
      _db.get_account( a.first );

   // 9. 수수료 차감 및 delegated vesting 업데이트
   _db.modify( creator, [&]( account_object& c )
   {
      c.balance -= o.fee;
      c.delegated_vesting_shares += o.delegation;
   });

   // 10. Delegation을 포함한 새 계정 생성
   const auto& new_account = _db.create< account_object >( [&]( account_object& acc )
   {
      initialize_account_object( acc, o.new_account_name, o.memo_key, props,
                                 false /*mined*/, o.creator, _db.get_hardfork() );
      acc.received_vesting_shares = o.delegation;

      #ifndef IS_LOW_MEM
         from_string( acc.json_metadata, o.json_metadata );
      #endif
   });

   // 11. 권한 객체 생성
   _db.create< account_authority_object >( [&]( account_authority_object& auth )
   {
      auth.account = o.new_account_name;
      auth.owner = o.owner;
      auth.active = o.active;
      auth.posting = o.posting;
      auth.last_owner_update = fc::time_point_sec::min();
   });

   // 12. Time lock이 있는 delegation 객체 생성
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

   // 13. 새 계정을 위해 수수료를 vesting으로 전환
   if( o.fee.amount > 0 )
      _db.create_vesting( new_account, o.fee );
}
```

**기본 생성과의 주요 차이점:**
1. 생성자가 delegation을 위한 충분한 **vesting shares**를 보유했는지 검증
2. **목표 vs. 현재 delegation** 계산 및 검증
3. 생성자의 `delegated_vesting_shares` 업데이트
4. 새 계정의 `received_vesting_shares` 설정
5. **30일 최소 잠금**이 있는 `vesting_delegation_object` 생성
6. `min_delegation_time` 이전에는 delegation 제거 불가

---

## 6. 설정 매개변수

### 6.1 프로토콜 상수

**파일:** [src/core/protocol/include/steem/protocol/config.hpp](../../../src/core/protocol/include/steem/protocol/config.hpp)

#### 테스트넷 설정 (Line 27)
```cpp
#define STEEM_MIN_ACCOUNT_CREATION_FEE        0
```

#### 메인넷 설정 (Line 54)
```cpp
#define STEEM_MIN_ACCOUNT_CREATION_FEE        1
```

#### 계정 생성 수정자 (Lines 132-134)
```cpp
#define STEEM_CREATE_ACCOUNT_WITH_STEEM_MODIFIER 30
#define STEEM_CREATE_ACCOUNT_DELEGATION_RATIO    5
#define STEEM_CREATE_ACCOUNT_DELEGATION_TIME     fc::days(30)
```

### 6.2 동적 매개변수 (Witness 제어)

**파일:** [src/core/protocol/include/steem/protocol/steem_operations.hpp:375](../../../src/core/protocol/include/steem/protocol/steem_operations.hpp#L375)

```cpp
struct chain_properties
{
   legacy_steem_asset account_creation_fee =
      legacy_steem_asset::from_amount( STEEM_MIN_ACCOUNT_CREATION_FEE );
   // ...
};
```

**Witness 제어:**
- 활성화된 21명의 witness가 각자 선호하는 `account_creation_fee` 제안
- 블록체인은 모든 제안의 **중앙값** 사용
- 매 witness 스케줄 라운드(~63초)마다 업데이트
- 스팸 방지 또는 성장 장려를 위한 동적 조정 가능

---

## 7. 예시

### 7.1 예시 1: 기본 계정 생성

**시나리오:**
- Witness 중앙값 `account_creation_fee` = 3 STEEM
- 생성자가 기본 작업으로 1개 계정 생성

**계산:**
```
필요 수수료 = account_creation_fee (기본 작업에는 수정자 미적용)
           = 3 STEEM

생성자 지불: 3 STEEM
새 계정 수령: 3 STEEM 상당의 vesting shares (SP)
```

**참고:** 기본 `account_create_operation`은 30배 수정자를 **사용하지 않습니다**. 해당 수정자는 wallet의 편의 기능을 사용하거나 `account_create_with_delegation_operation`의 delegation 요구사항을 계산할 때만 적용됩니다.

### 7.2 예시 2: Wallet 기반 계정 생성

**시나리오:**
- `cli_wallet`을 사용하여 계정 생성
- Witness 중앙값 `account_creation_fee` = 3 STEEM

**파일:** [src/wallet/wallet.cpp:1196](../../../src/wallet/wallet.cpp#L1196)

```cpp
op.fee = my->_remote_api.get_chain_properties().account_creation_fee
       * asset( STEEM_CREATE_ACCOUNT_WITH_STEEM_MODIFIER, STEEM_SYMBOL );
```

**계산:**
```
Wallet 수수료 = 3 STEEM × 30 = 90 STEEM
```

**이유:** Wallet은 delegation이 0인 `account_create_with_delegation_operation`을 사용하므로, 전체 30배 수수료가 필요합니다.

### 7.3 예시 3: Delegation 기반 생성 (최소 수수료)

**시나리오:**
- Witness 중앙값 `account_creation_fee` = 3 STEEM
- Vesting 가격 = STEEM당 1000 VESTS
- 생성자가 최소 유동 STEEM 비용 원함

**1단계: 목표 delegation 계산**
```
target_delegation = 3 STEEM × 30 × 5 × 1000 VESTS/STEEM
                  = 450,000 VESTS
```

**2단계: 최소 수수료로 계산**
```
current_delegation = (3 STEEM × 5 × 1000 VESTS/STEEM) + delegation
                   = 15,000 VESTS + delegation

필요: 15,000 + delegation >= 450,000
     delegation >= 435,000 VESTS
```

**결과:**
- 수수료: **3 STEEM** (최소)
- Delegation: **435,000 VESTS** (1000 VESTS/STEEM에서 435 SP)
- 총 비용: 3 유동 STEEM + 30일간 잠긴 435 SP

### 7.4 예시 4: Delegation 기반 생성 (균형 접근)

**시나리오:**
- 예시 3과 동일한 매개변수
- 생성자가 균형잡힌 수수료/delegation 분할 선호

**옵션: 30 STEEM 수수료 지불**
```
current_delegation = (30 STEEM × 5 × 1000) + delegation
                   = 150,000 VESTS + delegation

필요: 150,000 + delegation >= 450,000
     delegation >= 300,000 VESTS
```

**결과:**
- 수수료: **30 STEEM** (기본 수수료의 10배)
- Delegation: **300,000 VESTS** (300 SP)
- 총 비용: 30 유동 STEEM + 30일간 잠긴 300 SP

### 7.5 예시 5: 대량 서비스

**시나리오:**
- 월 1000개 계정을 생성하는 서비스
- Witness 중앙값 `account_creation_fee` = 3 STEEM
- Vesting 가격 = STEEM당 1000 VESTS

**옵션 A: 전액 수수료 방식**
```
계정당 비용 = 90 STEEM (wallet 기본값 사용)
월별 비용 = 90 × 1000 = 90,000 STEEM
```

**옵션 B: Delegation 방식**
```
계정당 비용 = 3 STEEM + 435,000 VESTS
월별 비용 = 3,000 STEEM + 435,000,000 VESTS (435,000 SP)

30일 후: Delegation 회수 및 새 계정에 재사용 가능
지속 비용 = 월 3,000 STEEM + 일회성 435,000 SP 잠금
```

**Delegation으로 절감:**
```
첫 달:   90,000 STEEM vs 3,000 STEEM + 435,000 SP
둘째 달: 90,000 STEEM vs 3,000 STEEM (delegation 재사용)
연간 비용: 1,080,000 STEEM vs 36,000 STEEM + 435,000 SP
```

**Delegation 방식은 대량 서비스의 지속 비용을 ~96% 절감합니다.**

### 7.6 테스트 픽스처 예시

**파일:** [tests/db_fixture/database_fixture.cpp:303](../../../tests/db_fixture/database_fixture.cpp#L303)

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

**목적:**
- 계정 생성을 위한 테스트 헬퍼 함수
- `(base_fee × 30)` 또는 `100` 중 더 큰 값 사용하여 충분한 수수료 보장
- 매우 낮은 witness 수수료에서도 테스트 작동 보장

---

## 참고 자료

### 소스 파일

- **Operations:** [src/core/protocol/include/steem/protocol/steem_operations.hpp](../../../src/core/protocol/include/steem/protocol/steem_operations.hpp)
- **Evaluators:** [src/core/chain/steem_evaluator.cpp](../../../src/core/chain/steem_evaluator.cpp)
- **Configuration:** [src/core/protocol/include/steem/protocol/config.hpp](../../../src/core/protocol/include/steem/protocol/config.hpp)
- **Wallet Implementation:** [src/wallet/wallet.cpp](../../../src/wallet/wallet.cpp)
- **Test Fixture:** [tests/db_fixture/database_fixture.cpp](../../../tests/db_fixture/database_fixture.cpp)

### 관련 문서

- [체인 매개변수](chain-parameters.md) - Witness가 제어하는 블록체인 매개변수
- [Vesting 시스템](vesting-system.md) - Vesting shares와 delegation 작동 방식
- [새 작업 생성](../../development/create-operation.md) - 새 작업 추가 방법

### 외부 리소스

- [Steem 백서](https://steem.com/steem-whitepaper.pdf) - 원본 설계 근거
- [API 문서](api-notes.md) - RPC를 통한 계정 생성 매개변수 조회 방법
