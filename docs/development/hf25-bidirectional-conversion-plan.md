# HF25 양방향 전환 기능 구현 플랜

## 📋 프로젝트 개요

**목표**: Hive HF25의 양방향 전환(Bidirectional Conversion) 기능을 Steem 코드베이스에 이식

**범위**: 거래소 관련 기능만 선택적으로 구현
- ✅ 담보 기반 STEEM → SBD 전환 (collateralized conversion)
- ✅ 관련 가상 operation
- ❌ HF25의 다른 기능 (보상 곡선 변경, recurring transfer 등)

**하드포크 요구사항**: 새로운 operation 추가로 하드포크 필요

---

## 🎯 기능 명세

### 기존 기능 (유지)
- **SBD → STEEM 전환** (`convert_operation`)
  - 3.5일 대기
  - 중간값 가격 사용
  - 수수료 없음

### 신규 기능 (추가)
- **STEEM → SBD 담보 전환** (`collateralized_convert_operation`)
  - 즉시 50% 전환 + 50% 담보
  - 3.5일 후 담보 정산
  - 5% 수수료
  - 최소 가격 보호 (3.5일 최저가 사용)

---

## 📐 시스템 설계

### 1. 새로운 Operations

#### 1.1 collateralized_convert_operation

```cpp
/**
 * STEEM을 SBD로 전환하는 담보 기반 전환 operation
 *
 * - 50%는 즉시 SBD로 전환하여 지급
 * - 50%는 담보로 잠금 (3.5일)
 * - 5% 추가 수수료 (담보에서 차감)
 * - 최소 가격 보호: min(3일 평균, 최근 1시간 최저가)
 */
struct collateralized_convert_operation : public base_operation
{
   account_name_type owner;
   uint32_t          requestid = 0;          // 소유자별 고유 ID
   asset             amount;                 // 전환할 STEEM 금액

   void validate() const;
   void get_required_active_authorities( flat_set<account_name_type>& a ) const
   {
      a.insert(owner);
   }
};
```

**검증 규칙**:
```cpp
void collateralized_convert_operation::validate() const
{
   validate_account_name( owner );

   // STEEM만 허용 (SBD → STEEM 방향 불가)
   FC_ASSERT( is_asset_type( amount, STEEM_SYMBOL ),
              "Can only convert STEEM to SBD" );

   FC_ASSERT( amount.amount > 0,
              "Must convert some STEEM" );
}
```

#### 1.2 fill_collateralized_convert_request_operation (Virtual)

```cpp
/**
 * 담보 전환 완료를 알리는 가상 operation
 *
 * - 3.5일 후 담보 정산 시 생성
 * - 최종 전환 금액과 반환된 담보 금액 기록
 */
struct fill_collateralized_convert_request_operation : public virtual_operation
{
   fill_collateralized_convert_request_operation() {}
   fill_collateralized_convert_request_operation(
      const string& o,
      const uint32_t id,
      const asset& steem_in,
      const asset& sbd_out,
      const asset& collateral_returned
   )
      : owner(o)
      , requestid(id)
      , amount_in(steem_in)
      , amount_out(sbd_out)
      , excess_collateral(collateral_returned)
   {}

   account_name_type owner;
   uint32_t          requestid = 0;
   asset             amount_in;              // 입력 STEEM
   asset             amount_out;             // 출력 SBD
   asset             excess_collateral;      // 반환된 잉여 담보
};
```

---

### 2. 데이터베이스 객체

#### 2.1 collateralized_convert_request_object

```cpp
/**
 * 담보 전환 요청을 추적하는 데이터베이스 객체
 */
class collateralized_convert_request_object :
   public object< collateralized_convert_request_object_type,
                  collateralized_convert_request_object >
{
public:
   template< typename Constructor, typename Allocator >
   collateralized_convert_request_object( Constructor&& c, allocator< Allocator > a )
   {
      c( *this );
   }

   collateralized_convert_request_object() {}

   id_type           id;

   account_name_type owner;
   uint32_t          requestid = 0;

   asset             collateral_amount;      // 담보로 잠긴 STEEM
   asset             converted_amount;       // 이미 전환된 SBD (즉시 지급)

   time_point_sec    conversion_date;        // 담보 정산 날짜
};
```

#### 2.2 인덱스 구조

```cpp
struct by_owner;
struct by_conversion_date;

typedef multi_index_container<
   collateralized_convert_request_object,
   indexed_by<
      // ID로 검색
      ordered_unique< tag< by_id >,
         member< collateralized_convert_request_object,
                 collateralized_convert_request_id_type,
                 &collateralized_convert_request_object::id >
      >,

      // 전환 날짜 순으로 정렬 (처리용)
      ordered_unique< tag< by_conversion_date >,
         composite_key< collateralized_convert_request_object,
            member< collateralized_convert_request_object,
                    time_point_sec,
                    &collateralized_convert_request_object::conversion_date >,
            member< collateralized_convert_request_object,
                    collateralized_convert_request_id_type,
                    &collateralized_convert_request_object::id >
         >
      >,

      // 소유자+requestid로 검색
      ordered_unique< tag< by_owner >,
         composite_key< collateralized_convert_request_object,
            member< collateralized_convert_request_object,
                    account_name_type,
                    &collateralized_convert_request_object::owner >,
            member< collateralized_convert_request_object,
                    uint32_t,
                    &collateralized_convert_request_object::requestid >
         >
      >
   >,
   allocator< collateralized_convert_request_object >
> collateralized_convert_request_index;
```

---

### 3. Evaluator 구현

#### 3.1 collateralized_convert_evaluator

```cpp
class collateralized_convert_evaluator : public evaluator< collateralized_convert_evaluator >
{
public:
   typedef collateralized_convert_operation operation_type;

   void do_apply( const collateralized_convert_operation& o );
};
```

**구현 로직**:

```cpp
void collateralized_convert_evaluator::do_apply( const collateralized_convert_operation& o )
{
   // 1. 잔고 확인
   FC_ASSERT( _db.get_balance( o.owner, o.amount.symbol ) >= o.amount,
              "Account does not have sufficient STEEM for conversion." );

   // 2. 가격 피드 확인
   const auto& fhistory = _db.get_feed_history();
   FC_ASSERT( !fhistory.current_median_history.is_null(),
              "Cannot convert STEEM because there is no price feed." );

   // 3. STEEM 차감
   _db.adjust_balance( o.owner, -o.amount );

   // 4. 금액 계산
   // - 전체 금액의 50%는 즉시 전환
   // - 나머지 50%는 담보로 잠금
   // - 5% 수수료 추가 (담보에서 차감)

   asset total_steem = o.amount;
   asset immediate_steem = total_steem / 2;         // 50% 즉시
   asset collateral_steem = total_steem - immediate_steem;  // 50% 담보

   // 5% 수수료 계산 (담보에서 차감)
   asset fee = collateral_steem * 5 / 100;          // 5%
   collateral_steem -= fee;

   // 5. 즉시 전환 부분 처리
   // 최소 가격 사용: min(3.5일 평균, 최근 1시간 최저가)
   price conversion_price = get_conversion_price();
   asset immediate_sbd = immediate_steem * conversion_price;

   // 즉시 SBD 지급
   _db.adjust_balance( o.owner, immediate_sbd );

   // 6. 담보 전환 요청 생성
   auto conversion_delay = STEEM_CONVERSION_DELAY;  // 3.5일

   _db.create< collateralized_convert_request_object >(
      [&]( collateralized_convert_request_object& obj )
      {
         obj.owner = o.owner;
         obj.requestid = o.requestid;
         obj.collateral_amount = collateral_steem;
         obj.converted_amount = immediate_sbd;
         obj.conversion_date = _db.head_block_time() + conversion_delay;
      }
   );

   // 7. 공급량 조정
   const auto& props = _db.get_dynamic_global_properties();
   _db.modify( props, [&]( dynamic_global_property_object& p )
   {
      // 수수료는 소각
      p.current_supply -= fee;
      p.virtual_supply -= fee;

      // 즉시 전환 부분: STEEM 감소, SBD 증가
      p.current_supply -= immediate_steem;
      p.current_sbd_supply += immediate_sbd;
      p.virtual_supply -= immediate_steem;
      p.virtual_supply += immediate_sbd * conversion_price;
   });
}
```

**가격 계산 헬퍼 함수**:

```cpp
price database::get_conversion_price() const
{
   const auto& fhistory = get_feed_history();
   price median_price = fhistory.current_median_history;

   // TODO: 최근 1시간 최저가도 고려
   // price recent_low = get_recent_low_price();
   // return std::min(median_price, recent_low);

   return median_price;
}
```

---

### 4. 담보 정산 처리

#### 4.1 process_collateralized_conversions 함수

**파일**: `src/core/chain/database.cpp`

```cpp
/**
 * 만료된 담보 전환 요청을 처리
 *
 * - 3.5일이 지난 요청 조회
 * - 담보를 현재 가격으로 SBD 전환
 * - 잉여 담보 반환
 * - 가상 operation 생성
 */
void database::process_collateralized_conversions()
{
   auto now = head_block_time();
   const auto& request_by_date =
      get_index< collateralized_convert_request_index >()
         .indices()
         .get< by_conversion_date >();

   auto itr = request_by_date.begin();

   const auto& fhistory = get_feed_history();
   if( fhistory.current_median_history.is_null() )
      return;

   asset net_sbd( 0, SBD_SYMBOL );
   asset net_steem_burned( 0, STEEM_SYMBOL );
   asset net_steem_returned( 0, STEEM_SYMBOL );

   while( itr != request_by_date.end() && itr->conversion_date <= now )
   {
      // 1. 필요한 SBD 계산
      // 목표: 전체 SBD = 즉시 전환된 SBD * 2
      asset target_sbd = itr->converted_amount * 2;
      asset remaining_sbd = target_sbd - itr->converted_amount;

      // 2. 담보로 필요한 STEEM 계산
      price conversion_price = fhistory.current_median_history;
      asset required_steem = remaining_sbd * conversion_price;

      // 3. 담보와 비교
      asset collateral = itr->collateral_amount;
      asset steem_to_burn;
      asset steem_to_return;
      asset sbd_to_issue;

      if( required_steem <= collateral )
      {
         // 담보가 충분: 필요한 만큼만 소각, 나머지 반환
         steem_to_burn = required_steem;
         steem_to_return = collateral - required_steem;
         sbd_to_issue = remaining_sbd;
      }
      else
      {
         // 담보 부족: 전체 담보 소각, SBD 일부만 발행
         steem_to_burn = collateral;
         steem_to_return = asset( 0, STEEM_SYMBOL );
         sbd_to_issue = collateral / conversion_price;
      }

      // 4. SBD 지급
      adjust_balance( itr->owner, sbd_to_issue );

      // 5. 잉여 담보 반환
      if( steem_to_return.amount > 0 )
         adjust_balance( itr->owner, steem_to_return );

      // 6. 누적
      net_sbd += sbd_to_issue;
      net_steem_burned += steem_to_burn;
      net_steem_returned += steem_to_return;

      // 7. 가상 operation 생성
      push_virtual_operation(
         fill_collateralized_convert_request_operation(
            itr->owner,
            itr->requestid,
            steem_to_burn + steem_to_return,  // 총 입력 STEEM
            itr->converted_amount + sbd_to_issue,  // 총 출력 SBD
            steem_to_return  // 반환된 담보
         )
      );

      // 8. 요청 삭제
      remove( *itr );
      itr = request_by_date.begin();
   }

   // 9. 전역 공급량 업데이트
   const auto& props = get_dynamic_global_properties();
   modify( props, [&]( dynamic_global_property_object& p )
   {
      p.current_supply -= net_steem_burned;
      p.current_supply += net_steem_returned;  // 반환은 다시 추가
      p.current_sbd_supply += net_sbd;

      p.virtual_supply -= net_steem_burned;
      p.virtual_supply += net_steem_returned;
      p.virtual_supply += net_sbd * fhistory.current_median_history;
   });
}
```

#### 4.2 apply_block에 통합

**파일**: `src/core/chain/database.cpp`의 `apply_block` 함수

```cpp
// 기존 코드
process_conversions();                    // SBD → STEEM 전환 처리

// 추가 코드
process_collateralized_conversions();     // STEEM → SBD 담보 전환 처리
```

---

### 5. 상수 정의

**파일**: `src/core/protocol/include/steem/protocol/config.hpp`

```cpp
// 담보 전환 수수료 (5%)
#define STEEM_COLLATERALIZED_CONVERSION_FEE_PERCENT   500  // 5% = 500 / 10000

// 담보 전환 지연 시간 (기존 STEEM_CONVERSION_DELAY와 동일)
#define STEEM_COLLATERALIZED_CONVERSION_DELAY   STEEM_CONVERSION_DELAY  // 3.5일
```

---

### 6. API 추가

#### 6.1 Database API

**파일**: `src/plugins/apis/database_api/include/steem/plugins/database_api/database_api.hpp`

```cpp
// 담보 전환 요청 목록 조회
DECLARE_API_METHOD( list_collateralized_conversion_requests,
                    list_collateralized_conversion_requests_args,
                    list_collateralized_conversion_requests_return )

// 특정 계정의 담보 전환 요청 조회
DECLARE_API_METHOD( find_collateralized_conversion_requests,
                    find_collateralized_conversion_requests_args,
                    find_collateralized_conversion_requests_return )
```

#### 6.2 API Arguments

**파일**: `src/plugins/apis/database_api/include/steem/plugins/database_api/database_api_args.hpp`

```cpp
struct api_collateralized_convert_request_object
{
   api_collateralized_convert_request_object(
      const collateralized_convert_request_object& o
   )
      : id( o.id )
      , owner( o.owner )
      , requestid( o.requestid )
      , collateral_amount( o.collateral_amount )
      , converted_amount( o.converted_amount )
      , conversion_date( o.conversion_date )
   {}

   collateralized_convert_request_id_type id;
   account_name_type owner;
   uint32_t          requestid;
   asset             collateral_amount;
   asset             converted_amount;
   time_point_sec    conversion_date;
};

struct list_collateralized_conversion_requests_args
{
   fc::variant       start;
   uint32_t          limit;
   sort_order_type   order;
};

struct list_collateralized_conversion_requests_return
{
   vector< api_collateralized_convert_request_object > requests;
};

struct find_collateralized_conversion_requests_args
{
   account_name_type account;
};

typedef list_collateralized_conversion_requests_return
   find_collateralized_conversion_requests_return;
```

---

## 📂 파일 구조 및 수정 목록

### Phase 1: Protocol Layer (새로운 operations)

#### 1.1 Operations 정의
- **파일**: `src/core/protocol/include/steem/protocol/steem_operations.hpp`
- **작업**:
  - [ ] `collateralized_convert_operation` 구조체 추가
  - [ ] `FC_REFLECT` 매크로 추가

#### 1.2 Virtual Operations 정의
- **파일**: `src/core/protocol/include/steem/protocol/steem_virtual_operations.hpp`
- **작업**:
  - [ ] `fill_collateralized_convert_request_operation` 구조체 추가
  - [ ] `FC_REFLECT` 매크로 추가

#### 1.3 Operations Validation
- **파일**: `src/core/protocol/steem_operations.cpp`
- **작업**:
  - [ ] `collateralized_convert_operation::validate()` 구현

#### 1.4 Operations 목록에 추가
- **파일**: `src/core/protocol/include/steem/protocol/operations.hpp`
- **작업**:
  - [ ] `operation` typedef에 `collateralized_convert_operation` 추가 (끝에)
  - [ ] Virtual operation variant에 `fill_collateralized_convert_request_operation` 추가

#### 1.5 상수 추가
- **파일**: `src/core/protocol/include/steem/protocol/config.hpp`
- **작업**:
  - [ ] `STEEM_COLLATERALIZED_CONVERSION_FEE_PERCENT` 정의
  - [ ] `STEEM_COLLATERALIZED_CONVERSION_DELAY` 정의

---

### Phase 2: Chain Layer (데이터베이스 및 로직)

#### 2.1 데이터베이스 객체
- **파일**: `src/core/chain/include/steem/chain/steem_objects.hpp`
- **작업**:
  - [ ] `collateralized_convert_request_object` 클래스 추가
  - [ ] `collateralized_convert_request_index` typedef 추가
  - [ ] `FC_REFLECT` 및 `CHAINBASE_SET_INDEX_TYPE` 추가

#### 2.2 Object Types
- **파일**: `src/core/chain/include/steem/chain/steem_object_types.hpp`
- **작업**:
  - [ ] `collateralized_convert_request_object_type` enum 추가
  - [ ] `collateralized_convert_request_id_type` typedef 추가

#### 2.3 Evaluator 정의
- **파일**: `src/core/chain/include/steem/chain/steem_evaluator.hpp`
- **작업**:
  - [ ] `STEEM_DEFINE_EVALUATOR( collateralized_convert )` 추가

#### 2.4 Evaluator 구현
- **파일**: `src/core/chain/steem_evaluator.cpp`
- **작업**:
  - [ ] `collateralized_convert_evaluator::do_apply()` 구현

#### 2.5 Database 처리 로직
- **파일**: `src/core/chain/database.cpp`
- **작업**:
  - [ ] `process_collateralized_conversions()` 함수 추가
  - [ ] `get_conversion_price()` 헬퍼 함수 추가
  - [ ] `apply_block()`에 `process_collateralized_conversions()` 호출 추가

#### 2.6 Database Header
- **파일**: `src/core/chain/include/steem/chain/database.hpp`
- **작업**:
  - [ ] `process_collateralized_conversions()` 함수 선언 추가
  - [ ] `get_conversion_price()` 함수 선언 추가

---

### Phase 3: API Layer

#### 3.1 Database API - Header
- **파일**: `src/plugins/apis/database_api/include/steem/plugins/database_api/database_api.hpp`
- **작업**:
  - [ ] `list_collateralized_conversion_requests` API 선언
  - [ ] `find_collateralized_conversion_requests` API 선언

#### 3.2 Database API - Arguments
- **파일**: `src/plugins/apis/database_api/include/steem/plugins/database_api/database_api_args.hpp`
- **작업**:
  - [ ] `api_collateralized_convert_request_object` 추가
  - [ ] `list_collateralized_conversion_requests_args` 추가
  - [ ] `list_collateralized_conversion_requests_return` 추가
  - [ ] `find_collateralized_conversion_requests_args` 추가
  - [ ] `FC_REFLECT` 매크로 추가

#### 3.3 Database API - Implementation
- **파일**: `src/plugins/apis/database_api/database_api.cpp`
- **작업**:
  - [ ] `list_collateralized_conversion_requests` 구현
  - [ ] `find_collateralized_conversion_requests` 구현

---

### Phase 4: Testing

#### 4.1 Unit Tests
- **파일**: `tests/tests/operation_tests.cpp`
- **작업**:
  - [ ] `collateralized_convert_operation` validation 테스트
  - [ ] 기본 전환 테스트

#### 4.2 Time-based Tests
- **파일**: `tests/tests/operation_time_tests.cpp`
- **작업**:
  - [ ] 3.5일 대기 메커니즘 테스트
  - [ ] 담보 정산 테스트
  - [ ] 가격 변동 시나리오 테스트
  - [ ] 수수료 계산 테스트
  - [ ] 잉여 담보 반환 테스트

#### 4.3 Integration Tests
- **새 파일**: `tests/tests/collateralized_conversion_tests.cpp`
- **작업**:
  - [ ] 엔드투엔드 전환 테스트
  - [ ] API 테스트
  - [ ] 공급량 조정 테스트
  - [ ] 에지 케이스 테스트

---

### Phase 5: Documentation

#### 5.1 Operation 문서
- **파일**: `docs/development/operations/collateralized-convert.md`
- **작업**:
  - [ ] Operation 명세 작성
  - [ ] 사용 예시 작성
  - [ ] 파라미터 설명

#### 5.2 API 문서
- **파일**: `docs/technical-reference/internal-market.md`
- **작업**:
  - [ ] 양방향 전환 섹션 추가
  - [ ] API 예시 추가
  - [ ] 가격 계산 메커니즘 설명

---

## 🔢 구현 순서 (Phases)

### Phase 1: 기초 구조 (1-2주)
**목표**: Operation 정의 및 기본 검증

1. [ ] Protocol layer에 operations 추가
2. [ ] Validation 로직 구현
3. [ ] 기본 unit test 작성

**완료 조건**: Operation이 컴파일되고 basic validation 테스트 통과

---

### Phase 2: 데이터베이스 Layer (2-3주)
**목표**: 데이터 저장 및 기본 처리

1. [ ] Database objects 정의
2. [ ] Evaluator 구현 (즉시 전환 부분)
3. [ ] 기본 전환 테스트

**완료 조건**: 즉시 전환 부분(50%)이 정상 작동

---

### Phase 3: 담보 정산 로직 (2-3주)
**목표**: 3.5일 후 담보 처리

1. [ ] `process_collateralized_conversions()` 구현
2. [ ] 가격 계산 로직 구현
3. [ ] 담보 반환 로직 구현
4. [ ] Time-based 테스트

**완료 조건**: 전체 전환 프로세스가 정상 작동

---

### Phase 4: API 및 통합 (1-2주)
**목표**: 외부 접근 가능

1. [ ] Database API 구현
2. [ ] API 테스트
3. [ ] 통합 테스트

**완료 조건**: API로 전환 요청 조회 가능

---

### Phase 5: 최적화 및 문서화 (1-2주)
**목표**: 프로덕션 준비

1. [ ] 성능 최적화
2. [ ] 에지 케이스 테스트
3. [ ] 문서 작성
4. [ ] 코드 리뷰

**완료 조건**: 프로덕션 배포 준비 완료

---

## ⚠️ 주의사항 및 고려사항

### 1. 하드포크 요구사항

**새로운 Operation 추가**:
- `collateralized_convert_operation`을 `operation` variant **끝에** 추가해야 함
- 기존 operation 순서 변경 금지
- 하드포크 날짜/버전 지정 필요

**권장 하드포크 번호**:
- Steem HF23 이후이므로 **HF24** 또는 커스텀 번호 사용
- Hive와 구분하기 위해 다른 이름 사용 권장 (예: "Steem HF24 - Exchange Enhancement")

### 2. 가격 피드 의존성

**현재 구현**:
- 3.5일 중간값만 사용
- "최근 1시간 최저가" 로직 없음

**개선 방안**:
```cpp
// Option 1: 간단한 구현 (중간값만 사용)
price conversion_price = fhistory.current_median_history;

// Option 2: Hive 방식 (추가 구현 필요)
price median = fhistory.current_median_history;
price recent_low = get_recent_low_price();  // 별도 구현 필요
price conversion_price = std::min(median, recent_low);
```

**권장**: Phase 1-3에서는 Option 1 사용, Phase 4에서 Option 2 추가

### 3. 공급량 관리

**주의사항**:
- STEEM 소각 시 `current_supply` 감소
- SBD 발행 시 `current_sbd_supply` 증가
- `virtual_supply` 동시 조정 필수
- 10% 시장 상한선 체크 (기존 로직 재사용)

### 4. 테스트 시나리오

**필수 테스트 케이스**:
1. [ ] 정상 전환 (담보 충분)
2. [ ] 가격 하락 (담보 부족)
3. [ ] 가격 상승 (담보 남음)
4. [ ] 수수료 계산
5. [ ] 중복 requestid 체크
6. [ ] 잔고 부족
7. [ ] 피드 없음
8. [ ] 동시 다중 전환
9. [ ] API 조회
10. [ ] 공급량 일관성

### 5. 호환성

**Steem 기존 기능 유지**:
- ✅ `convert_operation` (SBD → STEEM) 계속 작동
- ✅ 기존 API 유지
- ✅ 기존 데이터 마이그레이션 불필요

**새로운 노드 요구사항**:
- 하드포크 이전 블록: 기존 방식
- 하드포크 이후 블록: 양방향 전환 지원

### 6. 보안 고려사항

**잠재적 공격 벡터**:
1. **담보 부족 공격**: 가격 급등으로 담보 손실
   - **방어**: 5% 수수료가 버퍼 역할

2. **시장 조작**: 대량 전환으로 시장 교란
   - **방어**: 10% 시장 상한선 유지

3. **Sybil 공격**: 다중 계정으로 시스템 남용
   - **방어**: Active key 필요, 수수료 부담

### 7. 성능 최적화

**고려사항**:
- `process_collateralized_conversions()`는 매 블록마다 실행
- 인덱스 최적화: `by_conversion_date`로 빠른 조회
- 대량 전환 요청 시 처리 시간 모니터링

---

## 📊 예상 작업량

| Phase | 작업 내용 | 예상 기간 | 난이도 |
|-------|----------|----------|--------|
| Phase 1 | Protocol Layer | 1-2주 | 중 |
| Phase 2 | Database Layer | 2-3주 | 중상 |
| Phase 3 | 담보 정산 로직 | 2-3주 | 상 |
| Phase 4 | API 및 통합 | 1-2주 | 중 |
| Phase 5 | 최적화 및 문서 | 1-2주 | 중 |
| **합계** | | **7-12주** | |

**병렬 작업 가능**:
- 문서화는 각 Phase와 병행 가능
- 테스트 작성은 구현과 병행 가능

---

## 🎓 학습 자료

### Hive 코드 참조
```bash
# Hive 저장소 클론
git clone https://gitlab.syncad.com/hive/hive.git
cd hive

# HF25 관련 파일 찾기
git log --all --grep="collateralized" --oneline
git log --all --grep="HF25" --oneline
git log --all --grep="Equilibrium" --oneline

# 관련 파일 확인
find . -name "*collateralized*"
grep -r "collateralized_convert_operation" .
```

### 참고 문서
- [Hive HF25 공지](https://hive.blog/hive/@hiveio/hive-hardfork-25-is-on-the-way-hive-to-reach-equilibrium-on-june-30th-2021)
- [HiveSQL - Collateralized Converts](https://docs.hivesql.io/technical-informations/operations/txcollateralizedconverts-hf25)
- [Steem CLAUDE.md](../CLAUDE.md) - 프로젝트 가이드

---

## 📝 체크리스트

### 시작하기 전
- [ ] Hive 저장소 클론 및 코드 분석
- [ ] 현재 Steem 코드베이스의 전환 기능 완전 이해
- [ ] 테스트 환경 구축 (`BUILD_STEEM_TESTNET=ON`)
- [ ] 하드포크 번호 및 날짜 결정

### Phase 1 완료
- [ ] Operations 컴파일 성공
- [ ] Validation 테스트 통과
- [ ] 코드 리뷰 완료

### Phase 2 완료
- [ ] Database objects 생성/조회 가능
- [ ] Evaluator 기본 동작 확인
- [ ] 즉시 전환 테스트 통과

### Phase 3 완료
- [ ] 담보 정산 로직 동작
- [ ] Time-based 테스트 통과
- [ ] 공급량 조정 검증

### Phase 4 완료
- [ ] API 정상 작동
- [ ] 통합 테스트 통과
- [ ] 성능 벤치마크 완료

### Phase 5 완료
- [ ] 모든 문서 작성 완료
- [ ] 최종 코드 리뷰 통과
- [ ] 배포 준비 완료

---

## 🚀 다음 단계

1. **이 플랜 리뷰 및 승인**
2. **개발 환경 구축**
   ```bash
   cd /Users/hanyeol/Projects/hanyeol/steem
   git checkout -b feature/hf24-bidirectional-conversion
   mkdir build-testnet && cd build-testnet
   cmake -DBUILD_STEEM_TESTNET=ON -DCMAKE_BUILD_TYPE=Debug ..
   ```
3. **Phase 1 시작**: Protocol Layer 구현

---

## 💬 질문 및 논의사항

1. **하드포크 번호**: HF24로 할지, 다른 번호로 할지?
2. **가격 계산**: 중간값만 사용할지, Hive처럼 최근 최저가도 고려할지?
3. **수수료 비율**: 5%가 적절한지?
4. **테스트넷 배포**: 메인넷 배포 전 테스트넷에서 얼마나 테스트할지?
5. **Hive 코드 직접 이식 vs 재구현**: 라이선스 호환성 확인 필요

---

**작성일**: 2025-11-12
**작성자**: Claude
**버전**: 1.0
**상태**: Draft - Review 필요
