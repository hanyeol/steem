# 보상 레벨 시스템

## 개요

보상 레벨 시스템은 계정을 서로 다른 보상 등급(0-3)으로 분류하고, 레벨에 따라 차등 보상률을 적용하는 기능입니다. 이 메커니즘을 통해 특정 사용자 행동이나 계정 특성에 인센티브를 부여하는 맞춤형 보상 분배 전략을 구현할 수 있습니다.

**중요**: 보상 레벨 변경과 배율 조정 모두 **증인 합의**를 필요로 하며, 이를 통해 탈중앙화된 거버넌스를 보장하고 임의적인 보상 조작을 방지합니다.

## 보상 레벨

시스템은 4개의 보상 레벨을 정의합니다:

- **레벨 0**: 기본 레벨 (신규 계정 기본값)
- **레벨 1**: 향상된 보상
- **레벨 2**: 프리미엄 보상
- **레벨 3**: 최대 보상

## 아키텍처

### 핵심 구성요소

#### 1. 계정 객체 확장

보상 레벨은 블록체인 상태의 계정 객체에 저장됩니다.

**위치**: `src/core/chain/include/steem/chain/account_object.hpp`

```cpp
// account_object 클래스에 추가
uint8_t reward_level = 0; // 기본값 레벨 0
```

#### 2. 보상 레벨 제안 오퍼레이션

증인이 보상 레벨 변경을 제안하며, 합의 승인이 필요합니다.

**위치**: `src/core/protocol/include/steem/protocol/steem_operations.hpp`

```cpp
struct reward_level_proposal_operation
{
   account_name_type proposing_witness;  // 변경을 제안하는 증인
   account_name_type target_account;     // 조정 대상 계정
   uint8_t           new_reward_level;   // 제안된 레벨 (0-3)
   string            justification;      // 변경 사유

   void validate() const;
   void get_required_active_authorities( flat_set<account_name_type>& a ) const
   {
      a.insert( proposing_witness );
   }
};
```

**검증**:
```cpp
void reward_level_proposal_operation::validate() const
{
   FC_ASSERT( is_valid_account_name( proposing_witness ), "유효하지 않은 증인 계정명" );
   FC_ASSERT( is_valid_account_name( target_account ), "유효하지 않은 대상 계정명" );
   FC_ASSERT( new_reward_level <= 3, "보상 레벨은 0에서 3 사이여야 합니다" );
   FC_ASSERT( justification.size() > 0 && justification.size() <= 1000,
              "사유 필수 (최대 1000자)" );
}
```

#### 3. 보상 레벨 제안 객체

보류 중인 제안과 증인 승인을 추적합니다.

**위치**: `src/core/chain/include/steem/chain/reward_level_objects.hpp`

```cpp
class reward_level_proposal_object : public object< reward_level_proposal_object_type, reward_level_proposal_object >
{
   public:
      template< typename Constructor, typename Allocator >
      reward_level_proposal_object( Constructor&& c, allocator< Allocator > a )
      {
         c( *this );
      }

      id_type                    id;
      account_name_type          target_account;
      uint8_t                    proposed_level;
      account_name_type          proposing_witness;
      time_point_sec             created;
      string                     justification;
      flat_set<account_name_type> approvals;  // 승인한 증인들
};

struct by_target_account;
struct by_created;

typedef multi_index_container<
   reward_level_proposal_object,
   indexed_by<
      ordered_unique< tag< by_id >,
         member< reward_level_proposal_object, reward_level_proposal_id_type, &reward_level_proposal_object::id >
      >,
      ordered_non_unique< tag< by_target_account >,
         member< reward_level_proposal_object, account_name_type, &reward_level_proposal_object::target_account >
      >,
      ordered_non_unique< tag< by_created >,
         member< reward_level_proposal_object, time_point_sec, &reward_level_proposal_object::created >
      >
   >
> reward_level_proposal_index;
```

#### 4. 승인 오퍼레이션

증인들이 제안을 승인하거나 거부하기 위해 투표합니다.

```cpp
struct reward_level_approval_operation
{
   account_name_type          witness;
   reward_level_proposal_id_type proposal_id;
   bool                       approve;  // true = 승인, false = 거부

   void validate() const;
   void get_required_active_authorities( flat_set<account_name_type>& a ) const
   {
      a.insert( witness );
   }
};
```

#### 5. 보상 레벨 평가자

**제안 평가자** - 새로운 제안을 생성합니다:

**위치**: `src/core/chain/steem_evaluator.cpp`

```cpp
void reward_level_proposal_evaluator::do_apply( const reward_level_proposal_operation& o )
{
   const auto& witness = _db.get_witness( o.proposing_witness );
   FC_ASSERT( witness.signing_key != public_key_type(), "활성 증인이 아닙니다" );

   const auto& target = _db.get_account( o.target_account );

   // 해당 계정에 대한 기존 제안 확인
   const auto& proposal_idx = _db.get_index< reward_level_proposal_index >().indices().get< by_target_account >();
   auto existing = proposal_idx.find( o.target_account );
   FC_ASSERT( existing == proposal_idx.end(), "이 계정에 대한 제안이 이미 존재합니다" );

   _db.create< reward_level_proposal_object >( [&]( reward_level_proposal_object& p )
   {
      p.target_account = o.target_account;
      p.proposed_level = o.new_reward_level;
      p.proposing_witness = o.proposing_witness;
      p.created = _db.head_block_time();
      p.justification = o.justification;
      p.approvals.insert( o.proposing_witness );  // 제안자가 자동 승인
   });
}
```

**승인 평가자** - 증인 투표를 처리하고 합의 도달 시 변경사항을 적용합니다:

```cpp
void reward_level_approval_evaluator::do_apply( const reward_level_approval_operation& o )
{
   const auto& witness = _db.get_witness( o.witness );
   FC_ASSERT( witness.signing_key != public_key_type(), "활성 증인이 아닙니다" );

   const auto& proposal = _db.get< reward_level_proposal_object >( o.proposal_id );

   _db.modify( proposal, [&]( reward_level_proposal_object& p )
   {
      if( o.approve )
      {
         p.approvals.insert( o.witness );
      }
      else
      {
         p.approvals.erase( o.witness );
      }
   });

   // 합의 도달 여부 확인 (활성 증인의 과반수)
   const auto& witness_idx = _db.get_index< witness_index >().indices().get< by_vote_name >();
   uint32_t total_active_witnesses = 0;
   for( auto itr = witness_idx.begin(); itr != witness_idx.end() && total_active_witnesses < STEEM_MAX_WITNESSES; ++itr )
   {
      if( itr->signing_key != public_key_type() )
         total_active_witnesses++;
   }

   const uint32_t required_approvals = (total_active_witnesses * 2 / 3) + 1;  // 2/3 과반수

   if( proposal.approvals.size() >= required_approvals )
   {
      // 보상 레벨 변경 적용
      const auto& account = _db.get_account( proposal.target_account );
      _db.modify( account, [&]( account_object& a )
      {
         a.reward_level = proposal.proposed_level;
      });

      // 제안 제거
      _db.remove( proposal );

      ilog( "증인 합의에 의해 ${account}의 보상 레벨이 ${level}로 변경되었습니다",
            ("account", proposal.target_account)("level", proposal.proposed_level) );
   }
}
```

### 보상 계산

#### 보상 배율

각 레벨은 보상 분배에 영향을 미치는 연관된 배율을 가집니다:

```cpp
// src/core/chain/database.cpp 또는 적절한 보상 계산 위치
static const std::array<uint16_t, 4> REWARD_LEVEL_MULTIPLIERS = {
   10000,  // 레벨 0: 100% (1.00배)
   12500,  // 레벨 1: 125% (1.25배)
   15000,  // 레벨 2: 150% (1.50배)
   20000   // 레벨 3: 200% (2.00배)
};

inline uint16_t get_reward_multiplier( uint8_t level )
{
   FC_ASSERT( level <= 3, "유효하지 않은 보상 레벨" );
   return REWARD_LEVEL_MULTIPLIERS[level];
}
```

#### 기존 보상 로직과의 통합

보상 계산 시 보상 레벨 배율이 적용됩니다:

```cpp
// 예시: 저자 보상
asset calculate_author_reward( const account_object& author, asset base_reward )
{
   uint16_t multiplier = get_reward_multiplier( author.reward_level );
   return asset( (base_reward.amount.value * multiplier) / 10000, base_reward.symbol );
}

// 예시: 큐레이션 보상
asset calculate_curation_reward( const account_object& curator, asset base_reward )
{
   uint16_t multiplier = get_reward_multiplier( curator.reward_level );
   return asset( (base_reward.amount.value * multiplier) / 10000, base_reward.symbol );
}
```

## 구현 가이드

### 단계 1: 오퍼레이션 정의

1. `reward_level_proposal_operation`과 `reward_level_approval_operation`을 `steem_operations.hpp`에 추가
2. 두 오퍼레이션을 `operation` typedef variant에 추가
3. 직렬화를 위한 `FC_REFLECT` 매크로 구현:

```cpp
FC_REFLECT( steem::protocol::reward_level_proposal_operation,
            (proposing_witness)
            (target_account)
            (new_reward_level)
            (justification) )

FC_REFLECT( steem::protocol::reward_level_approval_operation,
            (witness)
            (proposal_id)
            (approve) )
```

### 단계 2: 계정 객체 확장

1. `account_object`에 `reward_level` 필드 추가
2. 필요 시 계정 객체 인덱스 정의 업데이트
3. 레벨별 조회가 필요한 경우 인덱스 추가:

```cpp
struct by_reward_level;

typedef multi_index_container<
   account_object,
   indexed_by<
      // ... 기존 인덱스들 ...
      ordered_non_unique< tag< by_reward_level >,
         composite_key< account_object,
            member< account_object, uint8_t, &account_object::reward_level >,
            member< account_object, account_id_type, &account_object::id >
         >
      >
   >
> account_index;
```

### 단계 3: 제안 객체 생성

1. 제안 객체 정의가 포함된 `reward_level_objects.hpp` 생성
2. 데이터베이스에 제안 인덱스 추가
3. 객체 ID 열거형에 객체 타입 등록

### 단계 4: 평가자 구현

1. `reward_level_proposal_evaluator` 클래스 생성
2. `reward_level_approval_evaluator` 클래스 생성
3. 데이터베이스 초기화 시 두 평가자 등록
4. 증인 검증 및 합의 로직 구현

### 단계 5: 보상 계산 수정

1. 보상 계산 함수 위치 파악:
   - 저자 보상 (콘텐츠 생성)
   - 큐레이션 보상 (투표)
   - 증인 보상 (블록 생성)
   - 이자/APR 계산

2. 각 보상 유형에 보상 레벨 배율 적용

### 단계 6: API 지원 추가

보상 레벨 정보 및 제안을 조회하는 API 메서드 추가:

```cpp
// database_api 또는 적절한 플러그인에 추가
struct get_reward_level_args
{
   account_name_type account;
};

struct get_reward_level_return
{
   uint8_t reward_level;
   uint16_t reward_multiplier;
};

struct get_reward_level_proposals_args
{
   account_name_type account;  // 계정별 필터 (선택사항)
};

struct get_reward_level_proposals_return
{
   vector<reward_level_proposal_api_object> proposals;
};

get_reward_level_return get_reward_level( get_reward_level_args args );
get_reward_level_proposals_return get_reward_level_proposals( get_reward_level_proposals_args args );
```

### 단계 7: 테스트 작성

**위치**: `tests/tests/operation_tests.cpp`

```cpp
BOOST_AUTO_TEST_SUITE(reward_level_tests)

BOOST_AUTO_TEST_CASE( reward_level_proposal_validate )
{
   try
   {
      BOOST_TEST_MESSAGE( "테스트: reward_level_proposal_operation 검증" );

      reward_level_proposal_operation op;
      op.proposing_witness = "witness1";
      op.target_account = "alice";
      op.justification = "Good contributor";

      // 유효한 레벨들
      for( uint8_t level = 0; level <= 3; level++ )
      {
         op.new_reward_level = level;
         REQUIRE_OP_VALIDATION_SUCCESS( op );
      }

      // 유효하지 않은 레벨
      op.new_reward_level = 4;
      REQUIRE_OP_VALIDATION_FAILURE( op );

      // 사유 미입력
      op.new_reward_level = 1;
      op.justification = "";
      REQUIRE_OP_VALIDATION_FAILURE( op );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( reward_level_proposal_create )
{
   try
   {
      BOOST_TEST_MESSAGE( "테스트: 보상 레벨 제안 생성" );

      ACTORS( (alice)(witness1) )
      fund( "witness1", ASSET( "1.000 TESTS" ) );

      // witness1을 활성 증인으로 설정
      witness_create( "witness1", witness1_private_key, "foo.bar", witness1_private_key.get_public_key(), 0 );

      const auto& alice_account = db->get_account( "alice" );
      BOOST_REQUIRE_EQUAL( alice_account.reward_level, 0 ); // 기본 레벨

      reward_level_proposal_operation op;
      op.proposing_witness = "witness1";
      op.target_account = "alice";
      op.new_reward_level = 2;
      op.justification = "뛰어난 커뮤니티 기여";

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      push_transaction( tx, witness1_private_key );

      // 제안 생성 확인
      const auto& proposal_idx = db->get_index< reward_level_proposal_index >().indices().get< by_target_account >();
      auto proposal = proposal_idx.find( "alice" );
      BOOST_REQUIRE( proposal != proposal_idx.end() );
      BOOST_REQUIRE_EQUAL( proposal->proposed_level, 2 );
      BOOST_REQUIRE_EQUAL( proposal->approvals.size(), 1 );  // 제안자가 자동 승인
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( reward_level_consensus )
{
   try
   {
      BOOST_TEST_MESSAGE( "테스트: 증인 합의를 통한 보상 레벨 변경" );

      ACTORS( (alice)(witness1)(witness2)(witness3)(witness4) )

      // 4명의 활성 증인 생성
      for( auto witness : { "witness1", "witness2", "witness3", "witness4" } )
      {
         fund( witness, ASSET( "1.000 TESTS" ) );
         witness_create( witness, generate_private_key( witness ), "foo.bar",
                        generate_private_key( witness ).get_public_key(), 0 );
      }

      const auto& alice_account = db->get_account( "alice" );
      BOOST_REQUIRE_EQUAL( alice_account.reward_level, 0 );

      // Witness1이 레벨 변경 제안
      reward_level_proposal_operation prop_op;
      prop_op.proposing_witness = "witness1";
      prop_op.target_account = "alice";
      prop_op.new_reward_level = 3;
      prop_op.justification = "탁월한 기여자";

      signed_transaction tx;
      tx.operations.push_back( prop_op );
      tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
      push_transaction( tx, witness1_private_key );

      // 제안 ID 획득
      const auto& proposal_idx = db->get_index< reward_level_proposal_index >().indices().get< by_target_account >();
      auto proposal = proposal_idx.find( "alice" );
      BOOST_REQUIRE( proposal != proposal_idx.end() );
      auto proposal_id = proposal->id;

      // 아직 레벨이 변경되지 않아야 함 (1/4 승인만)
      BOOST_REQUIRE_EQUAL( alice_account.reward_level, 0 );

      // Witness2와 witness3이 승인 (총 3/4 = 75%, 2/3 임계값 도달)
      for( auto witness : { "witness2", "witness3" } )
      {
         reward_level_approval_operation approve_op;
         approve_op.witness = witness;
         approve_op.proposal_id = proposal_id;
         approve_op.approve = true;

         tx.operations.clear();
         tx.operations.push_back( approve_op );
         tx.set_expiration( db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION );
         push_transaction( tx, generate_private_key( witness ) );
      }

      // 레벨이 변경되어야 함 (3/4 = 75% >= 66.7% 요구)
      BOOST_REQUIRE_EQUAL( alice_account.reward_level, 3 );

      // 제안이 제거되어야 함
      proposal = proposal_idx.find( "alice" );
      BOOST_REQUIRE( proposal == proposal_idx.end() );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( reward_level_multiplier_application )
{
   try
   {
      BOOST_TEST_MESSAGE( "테스트: 보상 레벨 배율 적용" );

      ACTORS( (alice)(bob) )
      fund( "alice", ASSET( "10.000 TESTS" ) );
      fund( "bob", ASSET( "10.000 TESTS" ) );

      // 서로 다른 보상 레벨 설정
      // ... 테스트 구현 ...

      // bob이 alice에 비해 약 2배의 보상을 받는지 확인
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
```

## 인증 및 보안

### 증인 합의 요구사항

보상 레벨 변경은 다음을 보장하기 위해 증인 합의를 필요로 합니다:
- **탈중앙화된 거버넌스**: 어느 누구도 임의로 보상을 조정할 수 없음
- **투명성**: 모든 제안과 승인이 온체인에 기록되어 감사 가능
- **책임성**: 사유가 필수이며 영구적으로 기록됨

### 합의 임계값

- **제안**: 모든 활성 증인이 보상 레벨 변경 제안 가능
- **승인**: 활성 증인(상위 21명)의 2/3 과반수 필요
- **자동 승인**: 제안하는 증인이 자동으로 자신의 제안을 승인
- **철회**: 합의 도달 전까지 증인이 승인 투표 변경 가능

### 권한 요구사항

- **제안**: 유효한 서명 키가 있는 증인 계정의 active 권한 필요
- **승인**: 유효한 서명 키가 있는 증인 계정의 active 권한 필요
- **대상 계정**: 영향을 받는 계정의 허가 불필요

### 보안 고려사항

```cpp
// 평가자에서 증인 검증
const auto& witness = _db.get_witness( o.proposing_witness );
FC_ASSERT( witness.signing_key != public_key_type(), "활성 증인이 아닙니다" );

// 중복 제안 방지
const auto& proposal_idx = _db.get_index< reward_level_proposal_index >().indices().get< by_target_account >();
auto existing = proposal_idx.find( o.target_account );
FC_ASSERT( existing == proposal_idx.end(), "이 계정에 대한 제안이 이미 존재합니다" );

// 활성 증인 수에 기반한 동적 합의 임계값 계산
const uint32_t required_approvals = (total_active_witnesses * 2 / 3) + 1;
```

### 경제적 고려사항

1. **인플레이션 영향**: 높은 보상 레벨은 총 보상 풀 분배를 증가시킴
2. **레벨 분포**: 각 레벨별 계정 비율 모니터링
3. **악용 방지**: 레벨 자격 기준을 구현하여 악용 방지
4. **점진적 전환**: 레벨 변경 간 쿨다운 기간 구현 고려

## API 통합

### Database API

```cpp
// 단일 계정의 보상 레벨 조회
DEFINE_API( database_api, get_reward_level )
{
   auto account = _db.find_account( args.account );
   FC_ASSERT( account != nullptr, "계정을 찾을 수 없습니다" );

   get_reward_level_return result;
   result.reward_level = account->reward_level;
   result.reward_multiplier = get_reward_multiplier( account->reward_level );

   return result;
}

// 보상 레벨 분포 통계 조회
struct get_reward_level_stats_return
{
   std::array<uint64_t, 4> account_counts; // 레벨별 계정 수
   std::array<share_type, 4> total_vests;  // 레벨별 총 VESTS
};

DEFINE_API( database_api, get_reward_level_stats )
{
   get_reward_level_stats_return result;
   // ... 구현 ...
   return result;
}

// 보류 중인 제안 조회
DEFINE_API( database_api, get_reward_level_proposals )
{
   get_reward_level_proposals_return result;

   const auto& proposal_idx = _db.get_index< reward_level_proposal_index >().indices().get< by_created >();

   if( args.account.size() > 0 )
   {
      // 특정 계정으로 필터링
      const auto& by_account_idx = _db.get_index< reward_level_proposal_index >().indices().get< by_target_account >();
      auto itr = by_account_idx.find( args.account );
      if( itr != by_account_idx.end() )
      {
         result.proposals.push_back( *itr );
      }
   }
   else
   {
      // 모든 제안 반환
      for( auto itr = proposal_idx.begin(); itr != proposal_idx.end(); ++itr )
      {
         result.proposals.push_back( *itr );
      }
   }

   return result;
}
```

### Condenser API

기존 애플리케이션을 위한 condenser API 호환 메서드 추가:

```cpp
fc::variant get_account_reward_level( std::string account_name );
fc::variant get_reward_level_proposals( optional<std::string> account_name );
fc::variant get_reward_level_proposal_votes( uint64_t proposal_id );
```

## 설정

`config.ini`에 설정 옵션 추가:

```ini
# 보상 레벨 제안 및 승인 로깅 활성화
log-reward-level-changes = true

# 신규 계정의 기본 보상 레벨 설정 (0-3)
default-reward-level = 0

# 자동 만료 전 제안의 최대 기간 (일)
reward-level-proposal-expiration = 30

# 변경 제안을 위한 최소 증인 순위 (1-21, 0 = 모든 활성 증인)
reward-level-min-witness-rank = 0
```

## 모니터링 및 지표

구현할 지표 추적:

- 레벨별 계정 분포
- 레벨별 분배된 총 보상
- 보류 중인 제안 수
- 제안 승인률 및 타이밍
- 제안 투표에서의 증인 참여도
- 인플레이션율에 대한 경제적 영향
- 레벨 전환 빈도 및 패턴

## 워크플로우 예시

### 일반적인 보상 레벨 변경 프로세스

1. **제안 생성**
   - 증인이 레벨 조정이 필요한 계정 식별
   - 증인이 사유와 함께 `reward_level_proposal_operation` 생성
   - 제안이 브로드캐스트되고 블록에 포함됨
   - 제안하는 증인이 자동으로 승인 (1표)

2. **커뮤니티 검토**
   - 다른 증인들이 제안과 사유를 검토
   - 커뮤니티는 소셜 레이어를 통해 논의 가능 (온체인 아님)
   - 증인들이 독립적으로 결정

3. **증인 투표**
   - 증인들이 승인/거부를 위해 `reward_level_approval_operation` 제출
   - 합의 도달 전까지 투표 변경 가능
   - API 쿼리를 통해 진행 상황 확인 가능

4. **합의 및 실행**
   - 2/3 과반수 도달 시 변경사항 즉시 적용
   - 계정의 보상 레벨 업데이트
   - 제안 자동 제거
   - 모니터링을 위한 이벤트 로깅

5. **효과**
   - 계정의 향후 보상이 새로운 배율로 계산됨
   - 과거 보상은 변경되지 않음
   - 다른 제안이 통과될 때까지 변경사항은 영구적

### CLI 월렛 명령어

```bash
# 보상 레벨 변경 제안 (증인만 가능)
propose_reward_level witness1 alice 2 "500개 이상의 우수한 게시물을 작성한 뛰어난 커뮤니티 기여자"

# 제안 승인
approve_reward_level_proposal witness2 12345 true

# 보류 중인 제안 확인
list_reward_level_proposals

# 특정 계정의 제안 확인
get_reward_level_proposal alice

# 계정의 현재 보상 레벨 조회
get_account_reward_level alice
```

## 향후 개선사항

1. **제안 만료**: 설정 가능한 기간(예: 30일) 후 제안 자동 만료
2. **제안 수정**: 합의 전에 제안자가 사유를 업데이트할 수 있도록 허용
3. **일괄 제안**: 여러 계정에 영향을 미치는 단일 제안
4. **레벨 기준 프레임워크**: 자동 레벨 자격을 위한 온체인 기준
5. **임시 부스트**: 이벤트/프로모션을 위한 시간 제한 보상 레벨 증가
6. **레벨 감소**: 비활성 계정에 대한 점진적 레벨 감소
7. **커스텀 배율**: 증인들이 레벨별 배율 값에 투표할 수 있도록 허용
8. **카테고리별 레벨**: 보상 유형별로 다른 레벨 (저자 vs. 큐레이터)
9. **제안 코멘트**: 증인들이 투표에 온체인 코멘트를 첨부할 수 있도록 허용
10. **이력 추적**: 과거 레벨 변경 및 그 영향을 조회하는 API

## 참조

- [새 오퍼레이션 생성](../../create-operation.md)
- [플러그인 개발 가이드](../../plugin.md)
- [테스팅 가이드](../../testing.md)
- [하드포크 절차](../../../operations/hardfork-procedure.md)
