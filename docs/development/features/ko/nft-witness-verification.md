# NFT 기반 증인 검증 기능

## 개요

이 문서는 증인들이 블록을 확인하기 전에 이더리움 호환 체인의 ERC-721 NFT 소유권을 증명해야 하는 기능의 구현을 설명합니다. 이는 추가적인 권한 계층을 추가하고 블록 생성을 위한 NFT 기반 거버넌스를 가능하게 합니다.

## 기능 설명

증인들은 블록 확인 자격을 얻기 위해 특정 ERC-721 NFT의 소유권을 입증해야 합니다. 주요 기능:

1. **여러 NFT 컬렉션 지원** - 여러 ERC-721 컨트랙트 주소 지원
2. **크로스체인 검증** - 이더리움 호환 체인(Ethereum, Polygon, BSC 등)에서 NFT 소유권 검증
3. **소유권 증명** - 증인들이 NFT 소유권에 대한 암호학적 증명 제출
4. **동적 업데이트** - 증인 합의를 통해 NFT 요구사항 업데이트 가능
5. **하위 호환성** - 하드포크 게이트로 기존 증인들과의 호환성 유지

## 아키텍처

### 상위 수준 흐름

```
┌─────────────────┐
│   증인 노드       │
│                 │
│  1. 소유권        │
│     증명 생성     │
└────────┬────────┘
         │
         ▼
┌─────────────────┐      ┌──────────────────┐
│  Steem 체인      │◄────►│  오라클 서비스      │
│                 │      │                  │
│  2. 서명 검증     │      │  3. EVM 체인에서   │
│                 │      │     NFT 소유권     │
│  4. 증인         │      │     조회          │
│     스케줄 검증    │      └──────────────────┘
└─────────────────┘
```

### 구성 요소

1. **NFT 검증 오라클** - EVM 체인을 조회하는 오프체인 서비스
2. **증인 검증 플러그인** - NFT 소유권 증명을 검증하는 플러그인
3. **NFT 레지스트리** - 승인된 NFT 컨트랙트의 온체인 레지스트리
4. **증명 메커니즘** - Steem 키를 이더리움 주소에 연결하는 암호학적 증명 시스템

## 구현 설계

### 1. 프로토콜 변경

#### 새로운 Operation

[libraries/protocol/include/steem/protocol/steem_operations.hpp](../../../libraries/protocol/include/steem/protocol/steem_operations.hpp)에 새로운 operation 추가:

```cpp
/**
 * 증인이 NFT 소유권 증명을 제출
 * 주기적으로 제출되어야 함 (예: 24시간마다)
 */
struct witness_nft_proof_operation : public base_operation
{
   account_name_type witness_account;

   // NFT를 소유한 이더리움 호환 주소
   string eth_address;

   // NFT 컨트랙트 주소
   string nft_contract_address;

   // 해당 주소가 소유한 토큰 ID
   string token_id;

   // 체인 ID (1 = Ethereum, 137 = Polygon, 56 = BSC 등)
   uint32_t chain_id;

   // Steem 개인키를 사용하여 eth_address의 제어권을 증명하는 서명
   // 서명 메시지: "steem:<witness_account>:nft:<nft_contract_address>:<token_id>:<timestamp>"
   string eth_signature;

   // 증명의 타임스탬프 (최근이어야 함)
   fc::time_point_sec timestamp;

   // 증인 서명 키로 operation에 서명
   extensions_type extensions;

   void validate() const;
   void get_required_active_authorities( flat_set<account_name_type>& a ) const
   {
      a.insert( witness_account );
   }
};

/**
 * 증인 검증에 사용할 수 있는 새로운 NFT 컬렉션 등록
 * 다수 증인 승인 필요
 */
struct nft_collection_register_operation : public base_operation
{
   account_name_type proposer;

   // 컬렉션의 사람이 읽을 수 있는 이름
   string collection_name;

   // 대상 체인의 NFT 컨트랙트 주소
   string nft_contract_address;

   // 체인 ID (1 = Ethereum, 137 = Polygon, 56 = BSC 등)
   uint32_t chain_id;

   // 이 컬렉션에서 필요한 최소 NFT 개수
   uint32_t min_nft_count = 1;

   // 이 컬렉션이 현재 활성화되어 있는지 여부
   bool active = true;

   extensions_type extensions;

   void validate() const;
   void get_required_active_authorities( flat_set<account_name_type>& a ) const
   {
      a.insert( proposer );
   }
};

/**
 * NFT 컬렉션 등록을 승인 또는 거부
 * 활성 증인이 서명해야 함
 */
struct nft_collection_approve_operation : public base_operation
{
   account_name_type witness_account;

   // 승인/거부할 컬렉션 등록 ID
   uint64_t collection_id;

   // true = 승인, false = 거부
   bool approve;

   extensions_type extensions;

   void validate() const;
   void get_required_active_authorities( flat_set<account_name_type>& a ) const
   {
      a.insert( witness_account );
   }
};
```

직렬화를 위한 `FC_REFLECT` 추가:

```cpp
FC_REFLECT( steem::protocol::witness_nft_proof_operation,
            (witness_account)
            (eth_address)
            (nft_contract_address)
            (token_id)
            (chain_id)
            (eth_signature)
            (timestamp)
            (extensions)
          )

FC_REFLECT( steem::protocol::nft_collection_register_operation,
            (proposer)
            (collection_name)
            (nft_contract_address)
            (chain_id)
            (min_nft_count)
            (active)
            (extensions)
          )

FC_REFLECT( steem::protocol::nft_collection_approve_operation,
            (witness_account)
            (collection_id)
            (approve)
            (extensions)
          )
```

#### 검증 구현

```cpp
void witness_nft_proof_operation::validate() const
{
   FC_ASSERT( is_valid_account_name( witness_account ),
              "유효하지 않은 증인 계정 이름" );

   // 이더리움 주소 형식 검증 (0x + 40 hex 문자)
   FC_ASSERT( eth_address.size() == 42,
              "유효하지 않은 이더리움 주소 길이" );
   FC_ASSERT( eth_address.substr(0, 2) == "0x",
              "이더리움 주소는 0x로 시작해야 함" );

   // 컨트랙트 주소 형식 검증
   FC_ASSERT( nft_contract_address.size() == 42,
              "유효하지 않은 NFT 컨트랙트 주소 길이" );
   FC_ASSERT( nft_contract_address.substr(0, 2) == "0x",
              "NFT 컨트랙트 주소는 0x로 시작해야 함" );

   // 토큰 ID가 비어있지 않은지 검증
   FC_ASSERT( !token_id.empty(),
              "토큰 ID는 비어있을 수 없음" );

   // 서명이 비어있지 않은지 검증
   FC_ASSERT( !eth_signature.empty(),
              "이더리움 서명은 비어있을 수 없음" );

   // 타임스탬프가 너무 오래되었거나 미래가 아닌지 검증
   auto now = fc::time_point::now();
   FC_ASSERT( timestamp <= now,
              "타임스탬프는 미래일 수 없음" );
   FC_ASSERT( now - timestamp <= fc::days(1),
              "증명 타임스탬프가 너무 오래됨 (24시간 이내여야 함)" );
}

void nft_collection_register_operation::validate() const
{
   FC_ASSERT( is_valid_account_name( proposer ),
              "유효하지 않은 제안자 계정 이름" );

   FC_ASSERT( !collection_name.empty(),
              "컬렉션 이름은 비어있을 수 없음" );
   FC_ASSERT( collection_name.size() <= 100,
              "컬렉션 이름이 너무 김 (최대 100자)" );

   FC_ASSERT( nft_contract_address.size() == 42,
              "유효하지 않은 NFT 컨트랙트 주소" );
   FC_ASSERT( nft_contract_address.substr(0, 2) == "0x",
              "NFT 컨트랙트 주소는 0x로 시작해야 함" );

   FC_ASSERT( min_nft_count > 0 && min_nft_count <= 100,
              "최소 NFT 개수는 1에서 100 사이여야 함" );
}

void nft_collection_approve_operation::validate() const
{
   FC_ASSERT( is_valid_account_name( witness_account ),
              "유효하지 않은 증인 계정 이름" );
}
```

### 2. 체인 상태 변경

#### 데이터베이스 객체

[libraries/chain/include/steem/chain/](../../../libraries/chain/include/steem/chain/)에 새로운 객체 추가:

`nft_verification_objects.hpp` 생성:

```cpp
#pragma once

#include <steem/chain/steem_object_types.hpp>
#include <steem/protocol/types.hpp>

namespace steem { namespace chain {

using namespace steem::protocol;

/**
 * 증인 검증에 사용할 수 있는 등록된 NFT 컬렉션 추적
 */
class nft_collection_object : public object< nft_collection_object_type, nft_collection_object >
{
   public:
      template< typename Constructor, typename Allocator >
      nft_collection_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      id_type           id;

      string            collection_name;
      string            nft_contract_address;
      uint32_t          chain_id;
      uint32_t          min_nft_count;
      bool              active;

      // 증인 승인 추적
      flat_set<account_name_type> approved_by;
      flat_set<account_name_type> rejected_by;

      fc::time_point_sec created;
      fc::time_point_sec last_updated;
};

/**
 * 증인들이 제출한 NFT 소유권 증명 추적
 */
class witness_nft_proof_object : public object< witness_nft_proof_object_type, witness_nft_proof_object >
{
   public:
      template< typename Constructor, typename Allocator >
      witness_nft_proof_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      id_type           id;

      account_name_type witness_account;
      string            eth_address;
      string            nft_contract_address;
      string            token_id;
      uint32_t          chain_id;

      fc::time_point_sec proof_timestamp;
      fc::time_point_sec verification_timestamp;

      // 검증 상태
      enum verification_status_type {
         pending,        // 오라클 검증 대기 중
         verified,       // 오라클이 소유권 확인
         failed,         // 오라클 검증 실패
         expired         // 증명이 너무 오래됨
      };

      uint8_t           verification_status;
      string            verification_details; // 오류 메시지 또는 오라클 트랜잭션 해시
};

struct by_collection_name;
struct by_contract_address;
struct by_witness;
struct by_verification_status;

typedef multi_index_container<
   nft_collection_object,
   indexed_by<
      ordered_unique< tag< by_id >,
         member< nft_collection_object, nft_collection_object::id_type, &nft_collection_object::id >
      >,
      ordered_unique< tag< by_collection_name >,
         member< nft_collection_object, string, &nft_collection_object::collection_name >
      >,
      ordered_non_unique< tag< by_contract_address >,
         composite_key< nft_collection_object,
            member< nft_collection_object, string, &nft_collection_object::nft_contract_address >,
            member< nft_collection_object, uint32_t, &nft_collection_object::chain_id >
         >
      >
   >,
   allocator< nft_collection_object >
> nft_collection_index;

typedef multi_index_container<
   witness_nft_proof_object,
   indexed_by<
      ordered_unique< tag< by_id >,
         member< witness_nft_proof_object, witness_nft_proof_object::id_type, &witness_nft_proof_object::id >
      >,
      ordered_non_unique< tag< by_witness >,
         member< witness_nft_proof_object, account_name_type, &witness_nft_proof_object::witness_account >
      >,
      ordered_non_unique< tag< by_verification_status >,
         composite_key< witness_nft_proof_object,
            member< witness_nft_proof_object, uint8_t, &witness_nft_proof_object::verification_status >,
            member< witness_nft_proof_object, fc::time_point_sec, &witness_nft_proof_object::proof_timestamp >
         >
      >
   >,
   allocator< witness_nft_proof_object >
> witness_nft_proof_index;

} } // steem::chain

FC_REFLECT( steem::chain::nft_collection_object,
            (id)
            (collection_name)
            (nft_contract_address)
            (chain_id)
            (min_nft_count)
            (active)
            (approved_by)
            (rejected_by)
            (created)
            (last_updated)
          )

FC_REFLECT_ENUM( steem::chain::witness_nft_proof_object::verification_status_type,
                 (pending)
                 (verified)
                 (failed)
                 (expired)
               )

FC_REFLECT( steem::chain::witness_nft_proof_object,
            (id)
            (witness_account)
            (eth_address)
            (nft_contract_address)
            (token_id)
            (chain_id)
            (proof_timestamp)
            (verification_timestamp)
            (verification_status)
            (verification_details)
          )

CHAINBASE_SET_INDEX_TYPE( steem::chain::nft_collection_object, steem::chain::nft_collection_index )
CHAINBASE_SET_INDEX_TYPE( steem::chain::witness_nft_proof_object, steem::chain::witness_nft_proof_index )
```

#### 객체 타입 열거형 업데이트

[libraries/chain/include/steem/chain/steem_object_types.hpp](../../../libraries/chain/include/steem/chain/steem_object_types.hpp)에 추가:

```cpp
enum object_type
{
   // ... 기존 타입들 ...
   nft_collection_object_type,
   witness_nft_proof_object_type
};
```

### 3. Evaluator 구현

[libraries/chain/](../../../libraries/chain/)에 evaluator 생성:

`nft_evaluator.cpp` 생성:

```cpp
#include <steem/chain/steem_evaluator.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/nft_verification_objects.hpp>
#include <steem/protocol/steem_operations.hpp>

namespace steem { namespace chain {

void witness_nft_proof_evaluator::do_apply( const witness_nft_proof_operation& o )
{
   const auto& witness = _db.get_witness( o.witness_account );

   // 증인이 존재하고 활성화되어 있는지 검증
   FC_ASSERT( witness.signing_key != public_key_type(),
              "증인이 활성화되어 있지 않음" );

   // 이 컨트랙트에 대해 NFT 컬렉션이 등록되어 있는지 확인
   const auto& collection_idx = _db.get_index< nft_collection_index >().indices().get< by_contract_address >();
   auto collection_itr = collection_idx.find( boost::make_tuple( o.nft_contract_address, o.chain_id ) );

   FC_ASSERT( collection_itr != collection_idx.end(),
              "NFT 컬렉션이 등록되지 않음: ${contract} on chain ${chain}",
              ("contract", o.nft_contract_address)("chain", o.chain_id) );

   FC_ASSERT( collection_itr->active,
              "NFT 컬렉션이 활성화되어 있지 않음" );

   // 이더리움 서명 검증
   // 메시지 형식: "steem:<witness_account>:nft:<nft_contract_address>:<token_id>:<timestamp>"
   std::string message = "steem:" + o.witness_account.operator string() +
                         ":nft:" + o.nft_contract_address +
                         ":" + o.token_id +
                         ":" + std::to_string( o.timestamp.sec_since_epoch() );

   // 서명에서 이더리움 주소 복구
   string recovered_address = recover_eth_address( message, o.eth_signature );

   FC_ASSERT( recovered_address == o.eth_address,
              "서명 검증 실패: 복구된 주소 ${recovered}가 제공된 주소 ${provided}와 일치하지 않음",
              ("recovered", recovered_address)("provided", o.eth_address) );

   // 증명 객체 생성 또는 업데이트
   const auto& proof_idx = _db.get_index< witness_nft_proof_index >().indices().get< by_witness >();
   auto proof_itr = proof_idx.find( o.witness_account );

   if( proof_itr == proof_idx.end() )
   {
      _db.create< witness_nft_proof_object >( [&]( witness_nft_proof_object& proof )
      {
         proof.witness_account = o.witness_account;
         proof.eth_address = o.eth_address;
         proof.nft_contract_address = o.nft_contract_address;
         proof.token_id = o.token_id;
         proof.chain_id = o.chain_id;
         proof.proof_timestamp = o.timestamp;
         proof.verification_status = witness_nft_proof_object::pending;
         proof.verification_details = "오라클 검증 대기 중";
      });
   }
   else
   {
      _db.modify( *proof_itr, [&]( witness_nft_proof_object& proof )
      {
         proof.eth_address = o.eth_address;
         proof.nft_contract_address = o.nft_contract_address;
         proof.token_id = o.token_id;
         proof.chain_id = o.chain_id;
         proof.proof_timestamp = o.timestamp;
         proof.verification_status = witness_nft_proof_object::pending;
         proof.verification_details = "오라클 검증 대기 중";
      });
   }

   // 오라클 서비스가 수집할 수 있도록 신호 발생
   _db.notify_nft_proof_submitted( o );
}

void nft_collection_register_evaluator::do_apply( const nft_collection_register_operation& o )
{
   // 제안자가 활성 증인인지 검증
   const auto* witness = _db.find_witness( o.proposer );
   FC_ASSERT( witness != nullptr,
              "제안자는 활성 증인이어야 함" );
   FC_ASSERT( witness->signing_key != public_key_type(),
              "제안자 증인이 활성화되어 있지 않음" );

   // 컬렉션이 이미 존재하는지 확인
   const auto& collection_idx = _db.get_index< nft_collection_index >().indices().get< by_contract_address >();
   auto collection_itr = collection_idx.find( boost::make_tuple( o.nft_contract_address, o.chain_id ) );

   FC_ASSERT( collection_itr == collection_idx.end(),
              "이 컨트랙트와 체인에 대한 NFT 컬렉션이 이미 등록됨" );

   // 컬렉션 등록 생성 (승인 대기 중)
   _db.create< nft_collection_object >( [&]( nft_collection_object& collection )
   {
      collection.collection_name = o.collection_name;
      collection.nft_contract_address = o.nft_contract_address;
      collection.chain_id = o.chain_id;
      collection.min_nft_count = o.min_nft_count;
      collection.active = false; // 승인될 때까지 비활성
      collection.approved_by.insert( o.proposer ); // 제안자가 자동 승인
      collection.created = _db.head_block_time();
      collection.last_updated = _db.head_block_time();
   });
}

void nft_collection_approve_evaluator::do_apply( const nft_collection_approve_operation& o )
{
   // 승인자가 활성 증인인지 검증
   const auto& witness = _db.get_witness( o.witness_account );
   FC_ASSERT( witness.signing_key != public_key_type(),
              "승인자는 활성 증인이어야 함" );

   // 컬렉션 가져오기
   const auto& collection = _db.get< nft_collection_object >( o.collection_id );

   // 승인 상태 업데이트
   _db.modify( collection, [&]( nft_collection_object& c )
   {
      if( o.approve )
      {
         c.approved_by.insert( o.witness_account );
         c.rejected_by.erase( o.witness_account );
      }
      else
      {
         c.rejected_by.insert( o.witness_account );
         c.approved_by.erase( o.witness_account );
      }

      c.last_updated = _db.head_block_time();

      // 상위 증인의 과반수가 승인하면 활성화
      // 상위 21명의 증인 가져오기
      const auto& witness_idx = _db.get_index< witness_index >().indices().get< by_vote_name >();
      size_t top_witness_count = 0;
      size_t approvals_from_top = 0;

      for( auto wit_itr = witness_idx.begin();
           wit_itr != witness_idx.end() && top_witness_count < 21;
           ++wit_itr, ++top_witness_count )
      {
         if( c.approved_by.count( wit_itr->owner ) > 0 )
            approvals_from_top++;
      }

      // 과반수 필요 (21명 중 11명)
      if( approvals_from_top >= 11 )
         c.active = true;
      else
         c.active = false;
   });
}

// 서명에서 이더리움 주소를 복구하는 헬퍼 함수
string witness_nft_proof_evaluator::recover_eth_address(
   const string& message,
   const string& signature )
{
   // 단순화된 버전 - 실제 구현에서는
   // secp256k1 복구와 keccak256 해싱을 사용해야 함

   // 1. "\x19Ethereum Signed Message:\n" + 길이로 메시지에 접두사 추가
   string prefixed_message = "\x19Ethereum Signed Message:\n" +
                             std::to_string( message.length() ) +
                             message;

   // 2. Keccak256으로 해시
   fc::sha256 message_hash = fc::keccak256( prefixed_message );

   // 3. 서명에서 공개키 복구
   // 서명 파싱 (r, s, v)
   // 서명 형식: 0x + 130 hex 문자 (65 바이트: r(32) + s(32) + v(1))
   FC_ASSERT( signature.size() == 132, "유효하지 않은 서명 길이" );

   // 서명에서 r, s, v 추출
   // ... (fc::ecc 또는 secp256k1 라이브러리를 사용한 구현 세부사항)

   // 4. 공개키에서 이더리움 주소 도출
   // keccak256(public_key)의 마지막 20바이트 사용
   // ... (구현 세부사항)

   // 플레이스홀더 반환 - 실제 구현 필요
   return signature; // TODO: 적절한 복구 구현
}

} } // steem::chain
```

### 4. 증인 스케줄 검증

블록 생성을 허용하기 전에 NFT 소유권을 검증하도록 [libraries/chain/database.cpp](../../../libraries/chain/database.cpp) 수정:

```cpp
bool database::validate_witness_nft_ownership( const account_name_type& witness ) const
{
   // 증인 NFT 증명 가져오기
   const auto& proof_idx = get_index< witness_nft_proof_index >().indices().get< by_witness >();
   auto proof_itr = proof_idx.find( witness );

   if( proof_itr == proof_idx.end() )
      return false; // 제출된 증명 없음

   // 검증 상태 확인
   if( proof_itr->verification_status != witness_nft_proof_object::verified )
      return false; // 검증되지 않음

   // 증명이 여전히 유효한지 확인 (만료되지 않음)
   auto now = head_block_time();
   if( now - proof_itr->proof_timestamp > fc::days(7) )
      return false; // 증명 만료 (7일마다 갱신 필요)

   return true;
}

void database::update_witness_schedule()
{
   // ... 기존 증인 스케줄 로직 ...

   // 유효한 NFT 소유권이 없는 증인 필터링
   // 유효한 NFT 증명이 없는 증인을 스케줄에서 제거
   auto& active_witnesses = get_witness_schedule_object().current_shuffled_witnesses;

   active_witnesses.erase(
      std::remove_if( active_witnesses.begin(), active_witnesses.end(),
         [this]( const account_name_type& witness ) {
            return !validate_witness_nft_ownership( witness );
         }
      ),
      active_witnesses.end()
   );

   FC_ASSERT( active_witnesses.size() > 0,
              "블록 생성에 사용할 유효한 NFT 소유권을 가진 증인이 없음" );

   // ... 나머지 스케줄링 로직 ...
}
```

### 5. NFT 검증 오라클 플러그인

오프체인 검증을 처리하기 위해 [libraries/plugins/nft_oracle/](../../../libraries/plugins/nft_oracle/)에 새로운 플러그인 생성:

`nft_oracle_plugin.hpp` 생성:

```cpp
#pragma once

#include <appbase/application.hpp>
#include <steem/plugins/chain/chain_plugin.hpp>

namespace steem { namespace plugins { namespace nft_oracle {

namespace detail { class nft_oracle_plugin_impl; }

class nft_oracle_plugin : public appbase::plugin< nft_oracle_plugin >
{
   public:
      APPBASE_PLUGIN_REQUIRES( (steem::plugins::chain::chain_plugin) )

      nft_oracle_plugin();
      virtual ~nft_oracle_plugin();

      static const std::string& name() { static std::string name = "nft_oracle"; return name; }

      virtual void set_program_options(
         boost::program_options::options_description& cli,
         boost::program_options::options_description& cfg ) override;

      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;

   private:
      std::unique_ptr< detail::nft_oracle_plugin_impl > my;
};

} } } // steem::plugins::nft_oracle
```

`nft_oracle_plugin.cpp` 생성:

```cpp
#include <steem/plugins/nft_oracle/nft_oracle_plugin.hpp>
#include <steem/chain/database.hpp>
#include <steem/chain/nft_verification_objects.hpp>

namespace steem { namespace plugins { namespace nft_oracle {

namespace detail {

class nft_oracle_plugin_impl
{
   public:
      nft_oracle_plugin_impl( nft_oracle_plugin& _plugin )
         : _self( _plugin ) {}

      void on_nft_proof_submitted( const witness_nft_proof_operation& op );
      void verify_nft_ownership( const witness_nft_proof_object& proof );

      // EVM 체인 RPC 엔드포인트
      std::map< uint32_t, std::string > rpc_endpoints;

      // 비동기 검증을 위한 스레드 풀
      boost::asio::io_service io_service;
      std::unique_ptr< boost::asio::io_service::work > work;
      boost::thread_group thread_pool;

   private:
      nft_oracle_plugin& _self;
      chain::database* _db = nullptr;
};

void nft_oracle_plugin_impl::on_nft_proof_submitted( const witness_nft_proof_operation& op )
{
   // 검증 작업 큐에 추가
   io_service.post( [this, op]() {
      // 데이터베이스에서 증명 객체 가져오기
      auto& db = _self.chain_plugin().db();

      db.with_read_lock( [&]() {
         const auto& proof_idx = db.get_index< witness_nft_proof_index >().indices().get< by_witness >();
         auto proof_itr = proof_idx.find( op.witness_account );

         if( proof_itr != proof_idx.end() )
         {
            verify_nft_ownership( *proof_itr );
         }
      });
   });
}

void nft_oracle_plugin_impl::verify_nft_ownership( const witness_nft_proof_object& proof )
{
   try
   {
      // 체인의 RPC 엔드포인트 가져오기
      auto rpc_itr = rpc_endpoints.find( proof.chain_id );
      if( rpc_itr == rpc_endpoints.end() )
      {
         elog( "체인 ${chain}에 대한 RPC 엔드포인트가 구성되지 않음", ("chain", proof.chain_id) );
         return;
      }

      // ERC-721 ownerOf(tokenId) 함수 호출
      // 컨트랙트 주소에 함수 선택자로 eth_call: 0x6352211e + 패딩된 token_id

      std::string method = "eth_call";
      std::string to = proof.nft_contract_address;
      std::string data = "0x6352211e" + pad_hex( proof.token_id, 64 ); // ownerOf 함수 선택자

      // EVM 노드에 JSON-RPC 요청 수행
      fc::variant result = make_rpc_call( rpc_itr->second, method,
                                          fc::variants{ to, data, "latest" } );

      // 결과 파싱 (소유자 주소)
      std::string owner_address = result.as_string();

      // 주장된 주소와 비교
      bool verified = ( to_lowercase( owner_address ) == to_lowercase( proof.eth_address ) );

      // 증명 객체 업데이트
      auto& db = _self.chain_plugin().db();
      db.with_write_lock( [&]() {
         const auto& proof_obj = db.get< witness_nft_proof_object >( proof.id );
         db.modify( proof_obj, [&]( witness_nft_proof_object& p ) {
            p.verification_status = verified ?
               witness_nft_proof_object::verified :
               witness_nft_proof_object::failed;
            p.verification_timestamp = db.head_block_time();
            p.verification_details = verified ?
               "온체인에서 소유권 검증됨" :
               "주소 " + owner_address + "가 NFT를 소유함, " + proof.eth_address + "가 아님";
         });
      });

      ilog( "증인 ${witness}에 대한 NFT 검증 ${result}: ${contract}:${token}",
            ("result", verified ? "성공" : "실패")
            ("witness", proof.witness_account)
            ("contract", proof.nft_contract_address)
            ("token", proof.token_id) );
   }
   catch( const fc::exception& e )
   {
      elog( "NFT 검증 오류: ${e}", ("e", e.to_detail_string()) );

      // 실패로 표시
      auto& db = _self.chain_plugin().db();
      db.with_write_lock( [&]() {
         const auto& proof_obj = db.get< witness_nft_proof_object >( proof.id );
         db.modify( proof_obj, [&]( witness_nft_proof_object& p ) {
            p.verification_status = witness_nft_proof_object::failed;
            p.verification_timestamp = db.head_block_time();
            p.verification_details = "검증 오류: " + e.to_string();
         });
      });
   }
}

} // detail

nft_oracle_plugin::nft_oracle_plugin() {}
nft_oracle_plugin::~nft_oracle_plugin() {}

void nft_oracle_plugin::set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg )
{
   cfg.add_options()
      ("nft-oracle-rpc-endpoint", boost::program_options::value< std::vector<std::string> >()->composing(),
       "chain_id:url 형식의 EVM RPC 엔드포인트 (예: 1:https://eth.llamarpc.com)")
      ("nft-oracle-threads", boost::program_options::value<uint32_t>()->default_value(4),
       "NFT 검증을 위한 스레드 수")
      ;
}

void nft_oracle_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   my = std::make_unique< detail::nft_oracle_plugin_impl >( *this );

   // RPC 엔드포인트 파싱
   if( options.count("nft-oracle-rpc-endpoint") )
   {
      auto endpoints = options["nft-oracle-rpc-endpoint"].as< std::vector<std::string> >();
      for( const auto& endpoint : endpoints )
      {
         auto colon_pos = endpoint.find(':');
         FC_ASSERT( colon_pos != std::string::npos,
                    "유효하지 않은 RPC 엔드포인트 형식: ${ep}", ("ep", endpoint) );

         uint32_t chain_id = std::stoi( endpoint.substr(0, colon_pos) );
         std::string url = endpoint.substr( colon_pos + 1 );

         my->rpc_endpoints[chain_id] = url;
         ilog( "체인 ${chain}에 대한 NFT 오라클 구성: ${url}",
               ("chain", chain_id)("url", url) );
      }
   }

   // 스레드 풀 초기화
   uint32_t num_threads = options["nft-oracle-threads"].as<uint32_t>();
   my->work = std::make_unique< boost::asio::io_service::work >( my->io_service );

   for( uint32_t i = 0; i < num_threads; ++i )
   {
      my->thread_pool.create_thread( [this]() {
         my->io_service.run();
      });
   }

   // 신호 핸들러 등록
   chain_plugin().db().notify_nft_proof_submitted.connect(
      [this]( const witness_nft_proof_operation& op ) {
         my->on_nft_proof_submitted( op );
      }
   );
}

void nft_oracle_plugin::plugin_startup()
{
   ilog( "NFT 오라클 플러그인 시작됨" );
}

void nft_oracle_plugin::plugin_shutdown()
{
   my->work.reset();
   my->thread_pool.join_all();
   ilog( "NFT 오라클 플러그인 종료됨" );
}

} } } // steem::plugins::nft_oracle
```

### 6. API 지원

NFT 검증 상태를 조회하기 위해 [libraries/plugins/apis/database_api/database_api.cpp](../../../libraries/plugins/apis/database_api/database_api.cpp)에 API 메서드 추가:

```cpp
struct get_witness_nft_proof_return
{
   bool has_proof = false;
   optional< witness_nft_proof_object > proof;
};

struct list_nft_collections_return
{
   vector< nft_collection_object > collections;
};

get_witness_nft_proof_return database_api::get_witness_nft_proof(
   string witness_account ) const
{
   return my->_db.with_read_lock( [&]()
   {
      get_witness_nft_proof_return result;

      const auto& proof_idx = my->_db.get_index< witness_nft_proof_index >()
                                     .indices().get< by_witness >();
      auto proof_itr = proof_idx.find( witness_account );

      if( proof_itr != proof_idx.end() )
      {
         result.has_proof = true;
         result.proof = *proof_itr;
      }

      return result;
   });
}

list_nft_collections_return database_api::list_nft_collections(
   bool active_only ) const
{
   return my->_db.with_read_lock( [&]()
   {
      list_nft_collections_return result;

      const auto& collection_idx = my->_db.get_index< nft_collection_index >()
                                          .indices().get< by_id >();

      for( auto itr = collection_idx.begin(); itr != collection_idx.end(); ++itr )
      {
         if( !active_only || itr->active )
         {
            result.collections.push_back( *itr );
         }
      }

      return result;
   });
}
```

## 사용 예제

### 1. NFT 컬렉션 등록

증인들이 NFT 컬렉션을 제안하고 투표:

```javascript
// 증인이 새로운 NFT 컬렉션 제안
const registerOp = {
  type: 'nft_collection_register_operation',
  value: {
    proposer: 'witness-alice',
    collection_name: 'Bored Ape Yacht Club',
    nft_contract_address: '0xBC4CA0EdA7647A8aB7C2061c2E118A18a936f13D',
    chain_id: 1, // Ethereum Mainnet
    min_nft_count: 1,
    active: true,
    extensions: []
  }
};

// 다른 증인들이 승인
const approveOp = {
  type: 'nft_collection_approve_operation',
  value: {
    witness_account: 'witness-bob',
    collection_id: 1,
    approve: true,
    extensions: []
  }
};
```

### 2. NFT 소유권 증명 제출

증인들은 주기적으로 NFT 소유권을 증명해야 함:

```javascript
// 증인이 NFT 소유권 증명
// 1. 이더리움 개인키로 서명 생성
const ethPrivateKey = '0x...'; // 증인의 이더리움 개인키
const witnessAccount = 'witness-alice';
const nftContract = '0xBC4CA0EdA7647A8aB7C2061c2E118A18a936f13D';
const tokenId = '1234';
const timestamp = Math.floor(Date.now() / 1000);

// 메시지 형식: "steem:<witness_account>:nft:<contract>:<token_id>:<timestamp>"
const message = `steem:${witnessAccount}:nft:${nftContract}:${tokenId}:${timestamp}`;

// eth_sign으로 서명 (ethers.js 또는 web3.js 사용)
const signature = await wallet.signMessage(message);

// 2. 증명 operation 제출
const proofOp = {
  type: 'witness_nft_proof_operation',
  value: {
    witness_account: 'witness-alice',
    eth_address: '0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb',
    nft_contract_address: nftContract,
    token_id: tokenId,
    chain_id: 1,
    eth_signature: signature,
    timestamp: timestamp,
    extensions: []
  }
};
```

### 3. NFT 검증 상태 조회

증인이 유효한 NFT 증명을 가지고 있는지 확인:

```javascript
// 증인 NFT 증명 상태 확인
const proof = await api.call('database_api', 'get_witness_nft_proof', {
  witness_account: 'witness-alice'
});

if (proof.has_proof) {
  console.log('검증 상태:', proof.proof.verification_status);
  console.log('증명 타임스탬프:', proof.proof.proof_timestamp);
  console.log('세부 정보:', proof.proof.verification_details);
} else {
  console.log('제출된 NFT 증명 없음');
}

// 등록된 모든 NFT 컬렉션 나열
const collections = await api.call('database_api', 'list_nft_collections', {
  active_only: true
});

console.log('활성 NFT 컬렉션:', collections.collections);
```

## 테스트

### 단위 테스트

[tests/tests/operation_tests.cpp](../../../tests/tests/operation_tests.cpp)에 테스트 추가:

```cpp
BOOST_AUTO_TEST_SUITE( nft_witness_tests )

BOOST_AUTO_TEST_CASE( nft_collection_register_validation )
{ try {
   BOOST_TEST_MESSAGE( "NFT 컬렉션 등록 검증 테스트" );

   nft_collection_register_operation op;
   op.proposer = "alice";
   op.collection_name = "Test Collection";
   op.nft_contract_address = "0x1234567890123456789012345678901234567890";
   op.chain_id = 1;
   op.min_nft_count = 1;

   REQUIRE_OP_VALIDATION_SUCCESS( op, nft_collection_register_operation );

   // 유효하지 않은 컨트랙트 주소
   op.nft_contract_address = "invalid";
   REQUIRE_OP_VALIDATION_FAILURE( op, nft_collection_register_operation );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( witness_nft_proof_validation )
{ try {
   BOOST_TEST_MESSAGE( "증인 NFT 증명 검증 테스트" );

   witness_nft_proof_operation op;
   op.witness_account = "alice";
   op.eth_address = "0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb";
   op.nft_contract_address = "0xBC4CA0EdA7647A8aB7C2061c2E118A18a936f13D";
   op.token_id = "1234";
   op.chain_id = 1;
   op.eth_signature = "0x1234..."; // 유효한 서명
   op.timestamp = fc::time_point::now();

   REQUIRE_OP_VALIDATION_SUCCESS( op, witness_nft_proof_operation );

   // 유효하지 않은 이더리움 주소
   op.eth_address = "invalid";
   REQUIRE_OP_VALIDATION_FAILURE( op, witness_nft_proof_operation );

   // 타임스탬프가 너무 오래됨
   op.eth_address = "0x742d35Cc6634C0532925a3b844Bc9e7595f0bEb";
   op.timestamp = fc::time_point::now() - fc::days(2);
   REQUIRE_OP_VALIDATION_FAILURE( op, witness_nft_proof_operation );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( nft_collection_approval_process )
{ try {
   BOOST_TEST_MESSAGE( "NFT 컬렉션 승인 프로세스 테스트" );

   // 21명의 증인 계정 생성
   ACTORS( (alice)(bob)(charlie) /* ... 21명의 증인 생성 ... */ )

   // 컬렉션 등록
   nft_collection_register_operation register_op;
   register_op.proposer = "alice";
   register_op.collection_name = "Test NFT";
   register_op.nft_contract_address = "0x1234567890123456789012345678901234567890";
   register_op.chain_id = 1;

   push_transaction( register_op, alice_private_key );
   generate_block();

   // 컬렉션 ID 가져오기
   const auto& collection_idx = db->get_index< nft_collection_index >()
                                    .indices().get< by_collection_name >();
   auto collection_itr = collection_idx.find( "Test NFT" );
   BOOST_REQUIRE( collection_itr != collection_idx.end() );
   BOOST_REQUIRE( !collection_itr->active ); // 아직 활성화되지 않음

   uint64_t collection_id = collection_itr->id._id;

   // 10명의 증인이 더 승인 (제안자 포함 총 11명)
   for( int i = 0; i < 10; i++ )
   {
      nft_collection_approve_operation approve_op;
      approve_op.witness_account = /* 증인 계정 */;
      approve_op.collection_id = collection_id;
      approve_op.approve = true;

      push_transaction( approve_op, /* 증인 키 */ );
   }

   generate_block();

   // 컬렉션이 이제 활성화되었는지 확인 (과반수 승인)
   collection_itr = collection_idx.find( "Test NFT" );
   BOOST_REQUIRE( collection_itr->active );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( witness_block_production_requires_nft )
{ try {
   BOOST_TEST_MESSAGE( "증인 블록 생성에 NFT 증명 필요 테스트" );

   ACTORS( (alice)(bob) )

   // 하드포크 활성화
   generate_blocks( STEEM_HARDFORK_0_XX_TIME );

   // Alice가 증인이 되지만 NFT 증명이 없음
   // 블록을 생성할 수 없어야 함

   // Bob이 증인이 되고 NFT 증명을 제출
   // 검증 후 블록을 생성할 수 있어야 함

   // TODO: 테스트 구현 완료

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
```

## 보안 고려사항

### 1. 서명 검증

- eth_address의 제어권을 증명하기 위해 이더리움 서명을 검증해야 함
- 메시지 서명에 EIP-191 표준 사용
- 타임스탬프 검증으로 재생 공격 방지

### 2. 오라클 신뢰

- 오라클 서비스는 손상될 수 있는 신뢰할 수 있는 구성 요소
- 중요한 작업에 대해 다중 오라클 검증 고려
- 오라클 평판 시스템 구현
- 오라클 실패 시 대체 메커니즘 추가

### 3. NFT 전송 공격

- 공격자가 NFT를 잠시 획득하고, 증명을 제출한 후 전송할 수 있음
- 증명이 최근이어야 함 (< 24시간)
- 장기간 소유권 증명 요구 고려
- 증명 제출 후 빠른 NFT 전송 모니터링

### 4. 체인 재구성

- 블록체인 재구성을 적절하게 처리
- 증명은 최근 블록 해시를 참조해야 함
- 오라클은 충분한 확인을 기다려야 함

### 5. 증인 담합

- 악의적인 증인들이 가짜 NFT 컬렉션을 승인할 수 있음
- 논란의 여지가 있는 컬렉션에 대해 절대다수(예: 21명 중 15명) 필요
- 컬렉션 제거를 위한 거버넌스 메커니즘 구현

### 6. DoS 공격

- 증인당 증명 제출 속도 제한
- 스팸 NFT 컬렉션 등록 방지
- 등록 및 증명에 대한 수수료 구현

## 성능 고려사항

### 1. 오라클 지연

- NFT 검증에는 외부 RPC 호출 필요 (느릴 수 있음)
- 최근 검증된 증명에 대해 캐싱 사용
- 체인 작업을 차단하지 않도록 비동기 검증 구현
- 증인 스케줄링 전 증명 사전 제출 고려

### 2. 스토리지 비용

- 각 증명 객체는 이더리움 주소와 서명을 저장
- 주기적으로 오래된 만료된 증명 정리
- 활성 NFT 컬렉션 수 제한

### 3. 네트워크 대역폭

- 이더리움 RPC 호출로 외부 네트워크 의존성 추가
- 가능한 경우 로컬 이더리움 노드 사용
- 연결 풀링 및 요청 배칭 구현

## 향후 개선 사항

### 1. 다중 NFT 요구사항

여러 컬렉션에서 여러 NFT를 요구하도록 지원:

```cpp
struct multi_nft_requirement {
   vector< nft_collection_id_type > required_collections;
   uint32_t min_collections_required; // "Y 중 X" 요구사항
};
```

### 2. NFT 스테이킹

증인들이 스마트 컨트랙트에 NFT를 잠가 "스테이킹"할 수 있도록 허용:

- 증명 후 빠른 전송 방지
- 경제적 보안 보장 제공
- 잘못된 행동에 대해 스테이킹된 NFT 슬래싱 가능

### 3. 위임된 NFT 소유권

NFT 소유자가 운영자에게 증인 권한을 위임할 수 있도록 허용:

```cpp
struct nft_delegation_operation {
   string nft_owner_eth_address;
   account_name_type delegated_witness;
   fc::time_point_sec expiration;
};
```

### 4. 동적 NFT 요구사항

체인 활동에 따라 NFT 요구사항 조정:

- 고위험 기간 동안 더 가치 있는 NFT 요구
- NFT 풀이 너무 작으면 전통적인 증인 투표로 대체 허용

### 5. 크로스체인 브리지

비 EVM 체인의 NFT를 검증하기 위해 크로스체인 브리지와 통합:

- Bitcoin Ordinals
- Solana NFT
- Cosmos NFT

### 6. 평판 기반 NFT 가치

다음을 기반으로 NFT에 다른 가중치 부여:

- 컬렉션 최저가
- 희귀도 특성
- 과거 소유권

## 배포 전략

### 1단계: 컬렉션 등록 (하드포크 전)

1. 모든 증인 노드에 nft_oracle 플러그인 배포
2. EVM RPC 엔드포인트 구성
3. 증인들이 초기 NFT 컬렉션 등록 및 승인
4. 테스트넷에서 오라클 검증 테스트

### 2단계: 소프트 배포 (하드포크 후)

1. NFT operation으로 하드포크 활성화
2. 증인들이 자발적으로 증명 제출
3. 증명 검증 성공률 모니터링
4. 블록 생성에 대한 NFT 요구사항은 아직 강제하지 않음

### 3단계: 강제 적용 (안정화 후)

1. 30일간 성공적인 증명 제출 후
2. NFT 소유권에 기반한 증인 스케줄 필터링 활성화
3. 유효한 증명이 없는 증인은 블록 생성에서 제외

### 롤백 계획

중요한 문제 발생 시:

1. NFT 요구사항을 비활성화하는 긴급 하드포크
2. 전통적인 증인 스케줄링으로 되돌림
3. 문제 조사 및 수정
4. 향후 하드포크에서 재활성화

## 구성

### 증인 노드 설정

`config.ini`에 추가:

```ini
# NFT 오라클 플러그인 활성화
plugin = nft_oracle

# EVM RPC 엔드포인트 구성
nft-oracle-rpc-endpoint = 1:https://eth.llamarpc.com
nft-oracle-rpc-endpoint = 137:https://polygon-rpc.com
nft-oracle-rpc-endpoint = 56:https://bsc-dataseed.binance.org

# 검증 스레드 수
nft-oracle-threads = 4
```

### 시드 노드 설정

시드 노드는 오라클 플러그인이 필요하지 않지만 NFT 객체를 추적해야 함:

```ini
# 최소 NFT 지원 (오라클 없음)
# NFT 객체는 추적되지만 검증되지 않음
```

## 관련 문서

- [플러그인 개발 가이드](../plugin.md)
- [Operation 생성 가이드](../create-operation.md)
- [하드포크 절차](../../operations/hardfork-procedure.md)
- [증인 운영 가이드](../../operations/witness-guide.md)

## 참고 자료

- **ERC-721 표준**: https://eips.ethereum.org/EIPS/eip-721
- **EIP-191 서명 데이터**: https://eips.ethereum.org/EIPS/eip-191
- **이더리움 JSON-RPC**: https://ethereum.org/en/developers/docs/apis/json-rpc/
- **프로토콜 Operations**: [libraries/protocol/include/steem/protocol/steem_operations.hpp](../../../libraries/protocol/include/steem/protocol/steem_operations.hpp)
- **체인 데이터베이스**: [libraries/chain/database.cpp](../../../libraries/chain/database.cpp)
- **플러그인 프레임워크**: [libraries/appbase/](../../../libraries/appbase/)
