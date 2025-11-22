# Witness Rewards: Genesis to 21 Witnesses

## 개요

이 문서는 Steem 블록체인의 증인(Witness) 보상 시스템이 제네시스 블록부터 21명의 증인이 확보될 때까지 어떻게 동작하는지 설명합니다. Steem은 DPOS(Delegated Proof of Stake) 블록체인으로, 최대 21명의 증인이 블록을 생성하고 보상을 받도록 설계되었지만, 부트스트랩 단계에서는 1명부터 시작하여 점진적으로 증인 수가 늘어나는 과정을 거칩니다.

## 목차

1. [증인 보상 계산 및 분배](#1-증인-보상-계산-및-분배)
2. [증인 스케줄링 로직](#2-증인-스케줄링-로직)
3. [제네시스 초기화](#3-제네시스-초기화)
4. [보상 정규화 메커니즘](#4-보상-정규화-메커니즘)
5. [실제 시나리오 예제](#5-실제-시나리오-예제)

---

## 1. 증인 보상 계산 및 분배

### 1.1 기본 인플레이션 모델

**파일:** [libraries/chain/database.cpp:1813-1860](../../libraries/chain/database.cpp#L1813-L1860)

Steem의 증인 보상은 블록마다 계산되는 인플레이션에서 나옵니다:

```cpp
void database::process_funds()
{
   // 현재 인플레이션율 계산 (9.78%에서 시작, 0.95%까지 감소)
   int64_t start_inflation_rate = int64_t( STEEM_INFLATION_RATE_START_PERCENT );  // 978 (9.78%)
   int64_t inflation_rate_adjustment = int64_t( head_block_num() / STEEM_INFLATION_NARROWING_PERIOD );
   int64_t inflation_rate_floor = int64_t( STEEM_INFLATION_RATE_STOP_PERCENT );   // 95 (0.95%)
   int64_t current_inflation_rate = std::max( start_inflation_rate - inflation_rate_adjustment, inflation_rate_floor );

   // 블록당 새로 생성되는 STEEM
   auto new_steem = ( props.virtual_supply.amount * current_inflation_rate ) /
                    ( int64_t( STEEM_100_PERCENT ) * int64_t( STEEM_BLOCKS_PER_YEAR ) );

   // 보상 분배 비율
   auto content_reward = ( new_steem * STEEM_CONTENT_REWARD_PERCENT ) / STEEM_100_PERCENT;  // 75%
   auto vesting_reward = ( new_steem * STEEM_VESTING_FUND_PERCENT ) / STEEM_100_PERCENT;   // 15%
   auto witness_reward = new_steem - content_reward - vesting_reward;                       // 10%
}
```

### 1.2 보상 비율 상수

**파일:** [libraries/protocol/include/steem/protocol/config.hpp](../../libraries/protocol/include/steem/protocol/config.hpp)

| 상수 | 값 | 설명 |
|------|-----|------|
| `STEEM_CONTENT_REWARD_PERCENT` | 75% | 콘텐츠 생성자에게 분배 |
| `STEEM_VESTING_FUND_PERCENT` | 15% | 베스팅 풀에 분배 |
| **증인 보상** | **10%** | **나머지 (100% - 75% - 15%)** |
| `STEEM_INFLATION_RATE_START_PERCENT` | 978 (9.78%) | 초기 인플레이션율 |
| `STEEM_INFLATION_RATE_STOP_PERCENT` | 95 (0.95%) | 최종 인플레이션율 |
| `STEEM_INFLATION_NARROWING_PERIOD` | 250,000 블록 | 인플레이션 감소 주기 |
| `STEEM_MAX_WITNESSES` | 21 | 최대 증인 수 |

### 1.3 증인 타입별 가중치 적용

**파일:** [libraries/chain/database.cpp:1840-1851](../../libraries/chain/database.cpp#L1840-L1851)

```cpp
// 증인 보상에 21배 곱하기 (정규화를 위해)
witness_reward *= STEEM_MAX_WITNESSES;  // × 21

// 증인 타입에 따른 가중치 적용
if( cwit.schedule == witness_object::timeshare )
   witness_reward *= wso.timeshare_weight;  // 가중치 = 5
else if( cwit.schedule == witness_object::top20 )
   witness_reward *= wso.top20_weight;      // 가중치 = 1

// 정규화 인수로 나누기
witness_reward /= wso.witness_pay_normalization_factor;
```

**증인 타입:**

- **top20**: 투표로 선출된 상위 20명 (가중치: 1배)
- **timeshare**: 백업 증인 1명, 가상 시간 스케줄링 기반 (가중치: 5배)

**가중치 이유:**
- Timeshare 증인은 블록 생성 빈도가 낮으므로 5배 가중치로 보상
- Top20 증인은 1배 가중치

### 1.4 보상 지급 방식

**파일:** [libraries/chain/database.cpp:1902-1915](../../libraries/chain/database.cpp#L1902-L1915)

```cpp
asset database::get_producer_reward()
{
   const auto& props = get_dynamic_global_properties();
   static_assert( STEEM_BLOCK_INTERVAL == 3, "this code assumes a 3-second time interval" );
   asset percent( protocol::calc_percent_reward_per_block< STEEM_PRODUCER_APR_PERCENT >( props.virtual_supply.amount ), STEEM_SYMBOL);
   auto pay = std::max( percent, STEEM_MIN_PRODUCER_REWARD );
   const auto& witness_account = get_account( props.current_witness );

   /// pay witness in vesting shares
   // Always pay in vesting shares (POW removed, no more bootstrap period)
   const auto& producer_reward = create_vesting( witness_account, pay );
   push_virtual_operation( producer_reward_operation( witness_account.name, producer_reward ) );

   return pay;
}
```

증인 보상은 **베스팅 주식(VESTS)** 형태로 지급됩니다. 이는 자동으로 "파워업(Power Up)"되어 증인의 장기적 네트워크 참여를 장려합니다.

**중요:** POW 기능이 제거되면서 초기 부트스트랩 기간의 유동 STEEM 지급 방식도 제거되었습니다. 모든 증인 보상은 제네시스부터 베스팅 주식으로 지급됩니다.

---

## 2. 증인 스케줄링 로직

### 2.1 증인이 21명 미만일 때의 처리

**파일:** [libraries/chain/witness_schedule.cpp:84-271](../../libraries/chain/witness_schedule.cpp#L84-L271)

Steem 블록체인은 21명 미만의 증인으로도 원활하게 동작하도록 설계되었습니다:

```cpp
void update_witness_schedule4( database& db )
{
   vector< account_name_type > active_witnesses;
   active_witnesses.reserve( STEEM_MAX_WITNESSES );  // 최대 21명 예약

   // 1. 투표로 선출된 증인 추가 (최대 19명 또는 20명)
   flat_set< witness_id_type > selected_voted;
   selected_voted.reserve( wso.max_voted_witnesses );

   const auto& widx = db.get_index<witness_index>().indices().get<by_vote_name>();
   for( auto itr = widx.begin();
        itr != widx.end() && selected_voted.size() < wso.max_voted_witnesses;
        ++itr )
   {
      if( itr->signing_key == public_key_type() )  // 서명 키가 없으면 스킵
         continue;
      selected_voted.insert( itr->id );
      active_witnesses.push_back( itr->owner );
      db.modify( *itr, [&]( witness_object& wo ) { wo.schedule = witness_object::top20; } );
   }

   auto num_elected = active_witnesses.size();

   // 2. Timeshare 증인 추가 (백업 증인)
   // ... (생략)

   // 핵심: 21명 미만의 증인 처리
   size_t expected_active_witnesses = std::min( size_t(STEEM_MAX_WITNESSES), widx.size() );
   FC_ASSERT( active_witnesses.size() == expected_active_witnesses, ... );

   // num_scheduled_witnesses 설정 (최소 1명)
   _wso.num_scheduled_witnesses = std::max< uint8_t >( active_witnesses.size(), 1 );

   // 증인 보상 정규화 인수 계산
   _wso.witness_pay_normalization_factor =
        _wso.top20_weight * num_elected
      + _wso.timeshare_weight * num_timeshare;
}
```

**핵심 포인트:**

1. **Line 156:** `expected_active_witnesses = std::min( STEEM_MAX_WITNESSES, widx.size() )`
   - 21명 미만이면 사용 가능한 모든 증인을 활용

2. **Line 242:** `num_scheduled_witnesses = std::max( active_witnesses.size(), 1 )`
   - 최소 1명의 증인 보장 (genesis 증인만 있어도 동작)

3. **Line 243-245:** 실제 증인 구성에 따라 보상 정규화 인수 조정

### 2.2 블록 생성 스케줄링

**파일:** [libraries/chain/database.cpp:1016-1022](../../libraries/chain/database.cpp#L1016-L1022)

```cpp
account_name_type database::get_scheduled_witness( uint32_t slot_num ) const
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   const witness_schedule_object& wso = get_witness_schedule_object();
   uint64_t current_aslot = dpo.current_aslot + slot_num;

   // 현재 슬롯을 증인 수로 나눈 나머지로 증인 선택
   return wso.current_shuffled_witnesses[ current_aslot % wso.num_scheduled_witnesses ];
}
```

**스케줄링 메커니즘:**

- 증인은 매 `num_scheduled_witnesses` 블록마다 셔플됩니다
- 21명 미만일 때는 더 빈번하게 로테이션됩니다

**예제:**
- **21명의 증인:** 각 증인은 21블록마다 1블록 생성
- **5명의 증인:** 각 증인은 5블록마다 1블록 생성
- **1명의 증인(genesis):** 모든 블록 생성

### 2.3 증인 셔플링

**파일:** [libraries/chain/witness_schedule.cpp:266](../../libraries/chain/witness_schedule.cpp#L266)

```cpp
// 매 라운드마다 증인 순서를 무작위로 셔플
_wso.current_shuffled_witnesses[i] = active_witnesses[i];
// ... (셔플 알고리즘)
```

이는 블록 생성 순서의 예측 가능성을 제거하여 보안을 강화합니다.

---

## 3. 제네시스 초기화

### 3.1 초기 상태 생성

**파일:** [libraries/chain/database.cpp:2315-2419](../../libraries/chain/database.cpp#L2315-L2419)

```cpp
void database::init_genesis( uint64_t init_supply )
{
   // 1. Genesis 증인 계정 생성
   for( int i = 0; i < STEEM_NUM_GENESIS_WITNESSES; ++i )  // STEEM_NUM_GENESIS_WITNESSES = 1
   {
      create< account_object >( [&]( account_object& a )
      {
         a.name = STEEM_GENESIS_WITNESS_NAME + ( i ? fc::to_string( i ) : std::string() );
         // a.name = "genesis"
         a.memo_key = init_public_key;
         a.balance  = asset( i ? 0 : init_supply, STEEM_SYMBOL );  // 모든 초기 공급량
      } );

      create< witness_object >( [&]( witness_object& w )
      {
         w.owner        = STEEM_GENESIS_WITNESS_NAME + ( i ? fc::to_string(i) : std::string() );
         w.signing_key  = init_public_key;
         w.schedule = witness_object::top20;  // top20로 시작
      } );
   }

   // 2. 전역 속성 초기화
   create< dynamic_global_property_object >( [&]( dynamic_global_property_object& p )
   {
      p.current_witness = STEEM_GENESIS_WITNESS_NAME;  // "genesis"
      p.time = STEEM_GENESIS_TIME;
      p.recent_slots_filled = fc::uint128::max_value();
      p.participation_count = 128;
      p.current_supply = asset( init_supply, STEEM_SYMBOL );
      p.virtual_supply = p.current_supply;
      p.maximum_block_size = STEEM_MAX_BLOCK_SIZE;
   } );

   // 3. 증인 스케줄 객체 생성
   create< witness_schedule_object >( [&]( witness_schedule_object& wso )
   {
      wso.current_shuffled_witnesses[0] = STEEM_GENESIS_WITNESS_NAME;  // genesis만 존재
   } );
}
```

### 3.2 제네시스 상수

**파일:** [libraries/protocol/include/steem/protocol/config.hpp](../../libraries/protocol/include/steem/protocol/config.hpp)

| 상수 | 값 | 설명 |
|------|-----|------|
| `STEEM_GENESIS_WITNESS_NAME` | "genesis" | 초기 증인 이름 |
| `STEEM_NUM_GENESIS_WITNESSES` | 1 | 초기 증인 수 |
| `STEEM_BLOCKS_PER_DAY` | 28,800 | 하루당 블록 수 (3초당 1블록) |

### 3.3 제네시스 블록 후 상태

제네시스 블록 생성 직후:

1. **증인:** "genesis" 1명만 존재
2. **초기 공급량:** 모두 genesis 계정에 할당
3. **블록 생성:** genesis 증인이 모든 블록 생성
4. **보상:** 베스팅 주식(VESTS)으로 지급

### 3.4 블록 생성 추적

**파일:** [libraries/chain/database.cpp:2644-2648](../../libraries/chain/database.cpp#L2644-L2648)

```cpp
/// modify current witness so we can track who produced this block
/// and pay witness rewards accordingly
modify( gprops, [&]( dynamic_global_property_object& dgp ){
   dgp.current_witness = next_block.witness;
});
```

이 코드는 현재 블록을 생성한 증인을 추적하여 보상을 정확하게 지급하기 위해 사용됩니다.

---

## 4. 보상 정규화 메커니즘

### 4.1 정규화 인수 계산

**파일:** [libraries/chain/include/steem/chain/witness_objects.hpp:172-175](../../libraries/chain/include/steem/chain/witness_objects.hpp#L172-L175)

```cpp
uint8_t top20_weight = 1;         // Top20 증인 가중치
uint8_t timeshare_weight = 5;     // Timeshare 증인 가중치
uint32_t witness_pay_normalization_factor = 25;  // 기본값 (21명 완전 구성 시)
```

**계산 공식:** (witness_schedule.cpp:243-245)

```cpp
witness_pay_normalization_factor =
    (top20_weight × num_elected) + (timeshare_weight × num_timeshare)
```

### 4.2 정규화 인수 예제

| 증인 구성 | 계산식 | 정규화 인수 |
|----------|--------|------------|
| 21명 완전 구성 (20 top20 + 1 timeshare) | (1×20) + (5×1) | 25 |
| 10명 (10 top20 + 0 timeshare) | (1×10) + (5×0) | 10 |
| 5명 (5 top20 + 0 timeshare) | (1×5) + (5×0) | 5 |
| 1명 (1 top20 + 0 timeshare, genesis) | (1×1) + (5×0) | 1 |

### 4.3 보상 계산 전체 과정

```cpp
// 1. 기본 블록 보상 계산 (인플레이션의 10%)
auto witness_reward = new_steem - content_reward - vesting_reward;

// 2. 21배 곱하기 (정규화를 위한 스케일링)
witness_reward *= STEEM_MAX_WITNESSES;  // × 21

// 3. 증인 타입별 가중치 적용
if( cwit.schedule == witness_object::timeshare )
   witness_reward *= wso.timeshare_weight;     // × 5
else if( cwit.schedule == witness_object::top20 )
   witness_reward *= wso.top20_weight;         // × 1

// 4. 정규화 인수로 나누기
witness_reward /= wso.witness_pay_normalization_factor;
```

### 4.4 정규화가 필요한 이유

정규화 메커니즘은 다음을 보장합니다:

1. **공평성:** 증인 수와 관계없이 총 인플레이션의 10%가 증인에게 분배
2. **유연성:** 1명부터 21명까지 모든 경우에 동작
3. **타입별 보상:** Timeshare 증인이 블록 생성 빈도가 낮은 것을 보상

**예제 계산:**

**시나리오 1: 1명의 증인 (genesis)**
```
기본 보상 = 100 STEEM
증인 보상 = 100 × 21 × 1 / 1 = 2100 STEEM
```

**시나리오 2: 21명의 증인 (20 top20 + 1 timeshare)**

Top20 증인:
```
기본 보상 = 100 STEEM
증인 보상 = 100 × 21 × 1 / 25 = 84 STEEM
```

Timeshare 증인:
```
기본 보상 = 100 STEEM
증인 보상 = 100 × 21 × 5 / 25 = 420 STEEM
```

검증:
```
총 분배 = (84 × 20) + (420 × 1) = 1680 + 420 = 2100 STEEM ✓
```

---

## 5. 실제 시나리오 예제

### 시나리오 1: 제네시스 (1명의 증인)

**상태:**
- 블록 번호: 1
- 증인: genesis (1명)
- 정규화 인수: 1

**블록 생성:**
- genesis가 모든 블록 생성

**보상:**
- 베스팅 주식(VESTS)으로 지급
- 보상 = `기본_보상 × 21 × 1 / 1 = 기본_보상 × 21`

**특징:**
- 단일 증인이 모든 블록 보상 수령
- 네트워크가 안정적으로 시작할 수 있도록 보장
- 베스팅 형태로 받아 장기 참여 유도

---

### 시나리오 2: 초기 성장 단계 (5명의 증인)

**상태:**
- 블록 번호: 100,000
- 증인: 5명 (모두 top20로 분류)
- 정규화 인수: 5

**블록 생성:**
- 각 증인이 5블록마다 1블록 생성
- 셔플링으로 순서 무작위화

**보상:**
- 베스팅 주식(VESTS)으로 지급
- 각 증인 보상 = `기본_보상 × 21 × 1 / 5 = 기본_보상 × 4.2`

**특징:**
- 5명이 총 보상 공평하게 분배
- 각 증인은 단독일 때보다 적게 받지만 총합은 동일
- 모든 보상이 베스팅으로 축적

---

### 시나리오 3: 성숙 단계 (21명의 증인)

**상태:**
- 블록 번호: 1,000,000
- 증인: 21명 (20 top20 + 1 timeshare)
- 정규화 인수: 25

**블록 생성:**
- Top20: 각각 21블록마다 1블록 생성
- Timeshare: 더 낮은 빈도로 생성

**보상:**
- 베스팅 주식(VESTS)으로 지급
- Top20 보상 = `기본_보상 × 21 × 1 / 25 = 기본_보상 × 0.84`
- Timeshare 보상 = `기본_보상 × 21 × 5 / 25 = 기본_보상 × 4.2`

**특징:**
- Timeshare 증인은 블록당 5배 보상으로 낮은 생성 빈도 보상
- 모든 증인이 베스팅 주식으로 받아 장기 참여 유도
- 안정적인 21명 증인 네트워크 운영

---

### 시나리오 4: 중간 성장 단계 (10명의 증인)

**상태:**
- 블록 번호: 500,000
- 증인: 10명 (모두 top20)
- 정규화 인수: 10

**블록 생성:**
- 각 증인이 10블록마다 1블록 생성

**보상:**
```
보상 형태: 베스팅 주식 (VESTS)
각 증인 보상 = 기본_보상 × 21 × 1 / 10 = 기본_보상 × 2.1
```

**특징:**
- 점진적으로 증인 수가 증가하는 과정
- 증인들은 베스팅 주식을 축적하여 투표력 획득
- 보상 총합은 항상 일정하게 유지

---

## 6. 불가역 블록(Irreversible Block) 결정

### 6.1 증인 투표 기반 합의

**파일:** [libraries/chain/database.cpp:3191-3228](../../libraries/chain/database.cpp#L3191-L3228)

```cpp
void database::update_last_irreversible_block()
{ try {
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   auto old_last_irreversible = dpo.last_irreversible_block_num;

   /**
    * Use witness voting to determine irreversibility (POW removed)
    */
   const witness_schedule_object& wso = get_witness_schedule_object();

   vector< const witness_object* > wit_objs;
   wit_objs.reserve( wso.num_scheduled_witnesses );
   for( int i = 0; i < wso.num_scheduled_witnesses; i++ )
      wit_objs.push_back( &get_witness( wso.current_shuffled_witnesses[i] ) );

   static_assert( STEEM_IRREVERSIBLE_THRESHOLD > 0, "irreversible threshold must be nonzero" );

   // 75% 합의를 위한 오프셋 계산
   size_t offset = ((STEEM_100_PERCENT - STEEM_IRREVERSIBLE_THRESHOLD) * wit_objs.size() / STEEM_100_PERCENT);

   std::nth_element( wit_objs.begin(), wit_objs.begin() + offset, wit_objs.end(),
      []( const witness_object* a, const witness_object* b )
      {
         return a->last_confirmed_block_num < b->last_confirmed_block_num;
      } );

   uint32_t new_last_irreversible_block_num = wit_objs[offset]->last_confirmed_block_num;

   if( new_last_irreversible_block_num > dpo.last_irreversible_block_num )
   {
      modify( dpo, [&]( dynamic_global_property_object& _dpo )
      {
         _dpo.last_irreversible_block_num = new_last_irreversible_block_num;
      } );
   }
} FC_CAPTURE_AND_RETHROW() }
```

**핵심 메커니즘:**

1. 모든 활성 증인의 마지막 확인 블록 번호를 수집
2. 75% 지점의 블록 번호를 불가역 블록으로 설정
3. 증인 수와 관계없이 동일한 알고리즘 적용

**이전 변경사항:** POW 제거와 함께 초기 30일 부트스트랩 기간의 보수적인 불가역 블록 결정 방식도 제거되었습니다. 제네시스부터 증인 투표 기반 합의를 사용합니다.

---

## 요약

Steem 블록체인의 증인 보상 시스템은 다음과 같이 설계되었습니다:

### 핵심 원칙

1. **유연한 증인 수 지원:** 1명부터 21명까지 모든 경우에 동작
2. **공평한 보상 분배:** 총 인플레이션의 10%를 증인들이 공평하게 나눔
3. **정규화 메커니즘:** 증인 수와 타입에 관계없이 일관된 총 보상
4. **베스팅 보상:** 제네시스부터 모든 증인 보상은 베스팅 주식으로 지급
5. **타입별 가중치:** Timeshare 증인 5배 보상으로 낮은 빈도 보상

### 단계별 진행

| 단계 | 증인 수 | 보상 형태 | 정규화 인수 | 특징 |
|------|--------|---------|------------|------|
| **제네시스** | 1 (genesis) | 베스팅 주식 | 1 | 모든 블록 생성 |
| **초기 성장** | 2-10 | 베스팅 주식 | 2-10 | 빈번한 로테이션 |
| **중간 성장** | 10-20 | 베스팅 주식 | 10-20 | 점진적 분산화 |
| **성숙** | 21 (20+1) | 베스팅 주식 | 25 | 안정적 스케줄 |

### POW 제거 변경사항

이전 버전에서는 POW(Proof of Work) 기능이 있었고, 초기 30일(블록 864,000) 동안 특별한 부트스트랩 로직이 있었습니다:

**제거된 기능:**
- POW 채굴 보상
- 초기 30일 유동 STEEM 보상 지급
- `STEEM_MINER_ACCOUNT` 특수 계정
- `STEEM_START_MINER_VOTING_BLOCK` 상수
- 보수적인 초기 불가역 블록 결정 방식

**현재 동작:**
- 제네시스부터 모든 증인 보상은 베스팅 주식(VESTS)으로 지급
- 불가역 블록은 항상 증인 투표 기반 합의(75% threshold)로 결정
- 단순화된 보상 시스템으로 장기 참여 유도

### 코드 위치 참조

- **보상 계산:** [libraries/chain/database.cpp:1813-1860](../../libraries/chain/database.cpp#L1813-L1860)
- **증인 스케줄링:** [libraries/chain/witness_schedule.cpp:84-271](../../libraries/chain/witness_schedule.cpp#L84-L271)
- **제네시스 초기화:** [libraries/chain/database.cpp:2315-2419](../../libraries/chain/database.cpp#L2315-L2419)
- **증인 보상 지급:** [libraries/chain/database.cpp:1902-1915](../../libraries/chain/database.cpp#L1902-L1915)
- **불가역 블록 결정:** [libraries/chain/database.cpp:3191-3228](../../libraries/chain/database.cpp#L3191-L3228)
- **상수 정의:** [libraries/protocol/include/steem/protocol/config.hpp](../../libraries/protocol/include/steem/protocol/config.hpp)

이 설계는 Steem 블록체인이 단일 증인으로 시작하여 점진적으로 분산화된 21명의 증인 네트워크로 성장할 수 있도록 보장합니다. POW 제거로 인해 시스템이 더욱 단순화되고, 제네시스부터 일관된 베스팅 보상 메커니즘을 사용하여 증인들의 장기적 네트워크 참여를 장려합니다.
