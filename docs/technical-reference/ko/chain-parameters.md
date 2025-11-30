# 체인 파라미터 및 Witness 거버넌스

이 문서는 Steem 블록체인이 유지하는 다양한 파라미터와 값, witness가 관리하는 방법, 그리고 witness가 이를 수정하는 방법을 설명합니다.

## 개요

Steem 블록체인은 **두 가지 범주의 파라미터**를 유지합니다:

1. **Dynamic Global Properties** - 현재 체인 상태를 반영하는 자동 계산 값
2. **Witness-Governed Parameters** - Witness 합의(중앙값 투표)로 설정되는 값

Witness는 거버넌스 파라미터에 대한 값을 제안하고, 모든 활성 witness의 **중앙값**이 활성 체인 파라미터가 됩니다.

## Dynamic Global Properties

### 개요

`dynamic_global_property_object`는 실시간 블록체인 상태를 저장합니다 ([global_property_object.hpp:22](../src/core/chain/include/steem/chain/global_property_object.hpp#L22)).

이러한 값은 블록 처리 중에 **자동으로 계산**되며 witness가 직접 수정할 수 없습니다.

### 블록 상태 속성

**현재 블록 정보:**
```cpp
uint32_t          head_block_number = 0;    // 현재 블록 높이
block_id_type     head_block_id;            // 현재 블록 해시
time_point_sec    time;                     // 현재 블록 타임스탬프
account_name_type current_witness;          // 현재 블록을 생성한 witness
```

**블록 스케줄링:**
```cpp
uint64_t current_aslot = 0;                 // genesis 이후 절대 슬롯 번호
                                            // = 총 슬롯 (놓친 것 포함)
```

**참여율 추적:**
```cpp
fc::uint128_t recent_slots_filled;          // 최근 블록 생성의 비트맵
                                            // 1 = 생성됨, 0 = 놓침
uint8_t participation_count = 0;            // 채워진 슬롯 수 / 128
                                            // 참여율 % 계산에 사용
```

**예제 - 참여율 계산:**
```cpp
// 128비트 비트맵, 각 비트 = 1 블록 슬롯
// participation_rate = (participation_count / 128) * 100%
// 마지막 128 슬롯 중 100개가 채워진 경우: 78% 참여율
```

### 공급량 및 경제

**STEEM 공급량:**
```cpp
asset virtual_supply = asset(0, STEEM_SYMBOL);    // 총 이론적 공급량
asset current_supply = asset(0, STEEM_SYMBOL);    // 실제 유통 공급량
asset confidential_supply = asset(0, STEEM_SYMBOL); // 블라인드 잔액 총액
```

**SBD (Steem Backed Dollars) 공급량:**
```cpp
asset current_sbd_supply = asset(0, SBD_SYMBOL);         // 존재하는 총 SBD
asset confidential_sbd_supply = asset(0, SBD_SYMBOL);    // 블라인드 SBD 잔액
```

**Vesting (Steem Power):**
```cpp
asset total_vesting_fund_steem = asset(0, STEEM_SYMBOL);  // 총 파워업된 STEEM
asset total_vesting_shares = asset(0, VESTS_SYMBOL);      // 총 vesting shares (SP)
```

**Vesting Share 가격 계산:**
```cpp
price get_vesting_share_price() const {
    if (total_vesting_fund_steem == 0 || total_vesting_shares == 0)
        return price(asset(1000, STEEM_SYMBOL), asset(1000000, VESTS_SYMBOL));

    return price(total_vesting_shares, total_vesting_fund_steem);
}
// 예: 100M STEEM → 100M VESTS인 경우, 1 STEEM = 1 VESTS
```

**보상 풀:**
```cpp
asset total_reward_fund_steem = asset(0, STEEM_SYMBOL);   // 분배 가능한 보상
fc::uint128 total_reward_shares2;                         // 분배 곡선을 위한 reward^2의 합
asset pending_rewarded_vesting_shares = asset(0, VESTS_SYMBOL); // 보류 중인 큐레이션 보상
asset pending_rewarded_vesting_steem = asset(0, STEEM_SYMBOL);  // 보류 중인 보상을 뒷받침하는 STEEM
```

### 합의 파라미터 (Witness 중앙값에서 파생)

`dynamic_global_property_object`의 이러한 값은 **witness 합의에서 파생**됩니다:

```cpp
uint16_t sbd_interest_rate = 0;              // SBD 저축 이자율 (베이시스 포인트)
                                             // Witness 중앙값에서 설정

uint32_t maximum_block_size = 0;             // 최대 블록 크기 (바이트)
                                             // Witness 중앙값에서 설정

uint16_t sbd_print_rate = STEEM_100_PERCENT; // SBD 발행율 (보상으로 지급되는 SBD %)
                                             // SBD 공급량에 따라 조정
```

### 비가역성

```cpp
uint32_t last_irreversible_block_num = 0;    // 되돌릴 수 없는 마지막 블록
                                             // 75% witness 확인에 도달하면 업데이트
```

**사용:**
- 영구적인 블록 결정
- 트랜잭션 최종성 보장에 사용
- 거래소 및 크로스체인 브릿지에 중요

### 투표 파라미터

```cpp
uint32_t vote_power_reserve_rate = STEEM_INITIAL_VOTE_POWER_RATE;  // 투표 재생성율
                                                                     // 기본값: 10 (하루 10번 완전 투표)

uint32_t delegation_return_period = STEEM_DELEGATION_RETURN_PERIOD_HF0; // Vesting 위임 쿨다운
                                                                          // 기본값: 30일
```

### Proof of Work (사용 중단)

```cpp
uint64_t total_pow = -1;                     // 누적된 총 PoW (레거시)
uint32_t num_pow_witnesses = 0;              // 현재 PoW witness (사용 중단)
```

## Witness-Governed 파라미터

### Chain Properties 구조

Witness는 `chain_properties`를 통해 블록체인 파라미터를 제안합니다 ([witness_objects.hpp:25](../src/core/chain/include/steem/chain/witness_objects.hpp#L25)):

```cpp
struct chain_properties {
    asset    account_creation_fee;     // 계정 생성 수수료 (STEEM)
    uint32_t maximum_block_size;       // 최대 블록 크기 (바이트)
    uint16_t sbd_interest_rate;        // SBD 저축 이자 (베이시스 포인트)
    uint32_t account_subsidy_limit;    // 계정 생성 속도 제한
};
```

### 파라미터 세부사항

#### 1. 계정 생성 수수료

```cpp
asset account_creation_fee = asset(STEEM_MIN_ACCOUNT_CREATION_FEE, STEEM_SYMBOL);
```

**목적:**
- 새 계정 생성 비용 (STEEM으로 지불)
- 새 계정의 vesting shares (SP)로 전환
- 스팸 방지 조치

**제약 조건:**
- 최소: `STEEM_MIN_ACCOUNT_CREATION_FEE` (프로토콜 정의)
- STEEM symbol로 지불해야 함

**Witness 투표:**
- 각 witness가 선호하는 수수료를 제안
- 21명의 활성 witness의 중앙값 사용
- 매 witness 스케줄 라운드마다 업데이트 (~63초)

#### 2. 최대 블록 크기

```cpp
uint32_t maximum_block_size = STEEM_MIN_BLOCK_SIZE_LIMIT * 2;  // 기본값: 128 KB
```

**목적:**
- 네트워크 대역폭 제어를 위한 블록 크기 제한
- 트랜잭션 처리량에 영향
- 용량과 탈중앙화 간의 균형

**제약 조건:**
- 최소: `STEEM_MIN_BLOCK_SIZE_LIMIT` (64 KB)
- 최대: `STEEM_SOFT_MAX_BLOCK_SIZE` (2 MB)

**검증 ([steem_evaluator.cpp:100](../src/core/chain/steem_evaluator.cpp#L100)):**
```cpp
FC_ASSERT(o.props.maximum_block_size <= STEEM_SOFT_MAX_BLOCK_SIZE,
          "Max block size cannot be more than 2MiB");
```

#### 3. SBD 이자율

```cpp
uint16_t sbd_interest_rate = STEEM_DEFAULT_SBD_INTEREST_RATE;  // 기본값: 0 베이시스 포인트
```

**목적:**
- 저축 중인 SBD에 지급되는 이자
- 베이시스 포인트로 표현 (1%의 1/100)
- SBD 보유 인센티브

**계산:**
- 100 베이시스 포인트 = 연 1% 이자
- 1000 베이시스 포인트 = 연 10% 이자

**예제:**
```
저축 중인 SBD: 1000 SBD
이자율: 500 베이시스 포인트 (연 5% APR)
연간 이자: 1000 * 0.05 = 50 SBD
```

#### 4. 계정 보조금 한도

```cpp
uint32_t account_subsidy_limit = 0;
```

**목적:**
- 보조된 계정 생성을 위한 속도 제한
- 계정 생성 풀 재생성 제어
- 계정 생성 스팸 방지

**메커니즘:**
- 높은 값 = 시간당 더 많은 계정
- Resource credit 시스템 사용
- 무료 계정과 남용 방지 간의 균형

### 중앙값 계산 프로세스

블록체인은 모든 활성 witness 제안의 **중앙값**을 사용합니다 ([witness_schedule.cpp:30](../src/core/chain/witness_schedule.cpp#L30)):

```cpp
void update_median_witness_props(database& db) {
    const witness_schedule_object& wso = db.get_witness_schedule_object();

    // 모든 활성 witness 가져오기
    vector<const witness_object*> active;
    for (int i = 0; i < wso.num_scheduled_witnesses; i++) {
        active.push_back(&db.get_witness(wso.current_shuffled_witnesses[i]));
    }

    // account_creation_fee로 정렬하고 중앙값 가져오기
    std::sort(active.begin(), active.end(),
        [](auto* a, auto* b) { return a->props.account_creation_fee < b->props.account_creation_fee; });
    asset median_account_creation_fee = active[active.size()/2]->props.account_creation_fee;

    // 다른 파라미터에 대해 반복...
    // maximum_block_size로 정렬
    // sbd_interest_rate로 정렬
    // account_subsidy_limit로 정렬

    // 중앙값으로 witness schedule object 업데이트
    db.modify(wso, [&](witness_schedule_object& _wso) {
        _wso.median_props.account_creation_fee = median_account_creation_fee;
        _wso.median_props.maximum_block_size = median_maximum_block_size;
        _wso.median_props.sbd_interest_rate = median_sbd_interest_rate;
        _wso.median_props.account_subsidy_limit = median_account_subsidy_limit;
    });

    // Dynamic global properties에 적용
    db.modify(db.get_dynamic_global_properties(), [&](auto& dgpo) {
        dgpo.maximum_block_size = median_maximum_block_size;
        dgpo.sbd_interest_rate = median_sbd_interest_rate;
    });
}
```

**왜 중앙값인가?**
- 단일 witness가 극단적인 값을 설정하는 것을 방지
- 변경하려면 과반수 합의(21명 중 11명 이상의 witness) 필요
- 파라미터 변경 시 부드러운 전환
- 이상값에 강함

## 중앙값 투표 이해하기: 11-Witness 규칙

### 수학적 기반

21명의 활성 witness에 대해 중앙값 계산은 다음과 같이 작동합니다:

```cpp
// witness_schedule.cpp의 코드
asset median_account_creation_fee = active[active.size()/2]->props.account_creation_fee;

// 분석:
// active.size() = 21
// active.size() / 2 = 10 (정수 나눗셈)
// 중앙값 = active[10] (11번째 요소, 0부터 시작하는 인덱스)
```

**배열 위치:**
```
인덱스:  [0][1][2][3][4][5][6][7][8][9][10][11][12][13][14][15][16][17][18][19][20]
                                      ↑
                                 중앙값 위치
                                (11번째 요소)
```

### 왜 11명의 Witness가 필요한가

중앙값을 변경하려면 **최소 11명의 witness가 동의**해야 합니다. 그 이유는 다음과 같습니다:

**시나리오 1: 10명의 witness가 변경 (불충분)**
```
초기 상태: 21명의 witness 모두 3 STEEM 제안
10명의 witness가 5 STEEM으로 변경

정렬 전: [5,5,5,5,5,5,5,5,5,5, 3,3,3,3,3,3,3,3,3,3,3]
정렬 후:  [3,3,3,3,3,3,3,3,3,3,3, 5,5,5,5,5,5,5,5,5,5]
인덱스:    0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
                                    ↑
                              여전히 3 STEEM
결과: 변경 없음 (10명의 witness는 불충분)
```

**시나리오 2: 11명의 witness가 변경 (충분!)**
```
초기 상태: 21명의 witness 모두 3 STEEM 제안
11명의 witness가 5 STEEM으로 변경

정렬 전: [5,5,5,5,5,5,5,5,5,5,5, 3,3,3,3,3,3,3,3,3,3]
정렬 후:  [3,3,3,3,3,3,3,3,3,3, 5,5,5,5,5,5,5,5,5,5,5]
인덱스:    0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
                                    ↑
                              이제 5 STEEM
결과: 변경됨! (11명의 witness가 전환점)
```

**시나리오 3: 극단적 이상값 무시**
```
20명의 witness가 3 STEEM 제안
1명의 witness가 1000 STEEM 제안 (극단값)

정렬 후:  [3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1000]
인덱스:    0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
                                    ↑
                              여전히 3 STEEM
결과: 이상값이 완전히 무시됨
```

### 점진적 파라미터 변경

이 메커니즘은 **부드럽고 점진적인 전환**을 가능하게 합니다:

```
단계 0: 초기 상태 (모두 3 STEEM)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
정렬됨: [3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3]
                            ↑ 중앙값 = 3

단계 1: 5명의 witness가 4 STEEM으로 변경
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
정렬됨: [3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4]
                            ↑ 중앙값 = 3 (아직 변경 없음)

단계 2: 10명의 witness가 4 STEEM
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
정렬됨: [3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4]
                            ↑ 중앙값 = 3 (여전히 변경 없음)

단계 3: 11명의 witness가 4 STEEM ⚡ 전환점
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
정렬됨: [3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4]
                            ↑ 중앙값 = 4 ✓ 변경됨!

단계 4: 15명의 witness가 4 STEEM
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
정렬됨: [3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4]
                            ↑ 중앙값 = 4 (안정)

단계 5: 21명의 witness 모두 4 STEEM
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
정렬됨: [4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4]
                            ↑ 중앙값 = 4 (합의 도달)
```

### 다양한 값 시나리오

**다양한 제안:**
```
Witness 제안: [1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 7, 8, 9, 10, 15, 100]
               0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18  19  20
                                                  ↑
                                             중앙값 = 5

중앙값은 자연스럽게 "중간 지점" 값을 찾습니다
```

### 보안 및 거버넌스 이점

**1. Byzantine Fault Tolerance**
```
총 witness: 21
중앙값 필요: 11명의 witness (52.4%)
허용 가능: 최대 10명의 악의적 witness (47.6%)
```

**2. 공격 저항성**
```
공격: 단일 witness가 0 또는 MAX_VALUE 제안
결과: ❌ 효과 없음 (11명의 witness 필요)

공격: 10명의 witness 공모
결과: ❌ 효과 없음 (11명의 witness 필요)

공격: 11명 이상의 witness 공모
결과: ⚠️  성공 (하지만 과반수 필요)
       → 스테이크홀더가 악의적 witness를 투표로 제거
```

**3. 탈중앙화 거버넌스**
```
✓ 단일 통제점 없음
✓ 소수가 변경을 강제할 수 없음
✓ 과반수 합의 필요
✓ 극단값 자동 거부
✓ 점진적 전환으로 커뮤니티 반응 가능
```

### 실제 적용

**파라미터 변경 타임라인:**
1. Witness가 새 값 제안 → 즉각적인 효과 없음
2. 더 많은 witness 합류 → 여전히 효과 없음 (11명 미만인 경우)
3. 11번째 witness가 변경 → **파라미터가 즉시 변경**
4. 커뮤니티가 변경 관찰 → 문제가 있으면 witness를 투표로 제거 가능
5. 더 많은 witness가 채택 → 중앙값 안정화

**예: 블록 크기를 64KB에서 128KB로 증가**
```
1주차: 3명의 witness가 128KB 제안  → 변경 없음 (중앙값은 64KB 유지)
2주차: 8명의 witness가 128KB 제안  → 변경 없음 (중앙값은 64KB 유지)
3주차: 11명의 witness가 128KB 제안 → ✓ 변경! (중앙값이 128KB가 됨)
4주차: 18명의 witness가 128KB 제안 → 128KB로 안정
```

이 점진적 프로세스는 다음을 허용합니다:
- 커뮤니티 토론 및 피드백
- 먼저 작은 변경 테스트
- 문제 발견 시 되돌리기 (11명 이상의 witness가 되돌리는 경우)
- 민주적 합의 구축

## Witness가 파라미터를 변경하는 방법

### 방법 1: witness_update_operation (레거시)

**Operation 구조:**
```cpp
struct witness_update_operation {
    account_name_type owner;              // Witness 계정
    string            url;                // Witness 정보 URL
    public_key_type   block_signing_key;  // 블록 서명 키
    chain_properties  props;              // 제안된 체인 파라미터
    asset             fee;                // Operation 수수료
};
```

**구현 ([steem_evaluator.cpp:83](../src/core/chain/steem_evaluator.cpp#L83)):**
```cpp
void witness_update_evaluator::do_apply(const witness_update_operation& o) {
    _db.get_account(o.owner);  // 소유자 존재 확인

    // 수수료 symbol 검증
    FC_ASSERT(o.props.account_creation_fee.symbol.is_canon());

    // 블록 크기 검증
    FC_ASSERT(o.props.maximum_block_size <= STEEM_SOFT_MAX_BLOCK_SIZE,
              "Max block size cannot be more than 2MiB");

    const auto& by_witness_name_idx = _db.get_index<witness_index>().indices().get<by_name>();
    auto wit_itr = by_witness_name_idx.find(o.owner);

    if (wit_itr != by_witness_name_idx.end()) {
        // 기존 witness 업데이트
        _db.modify(*wit_itr, [&](witness_object& w) {
            from_string(w.url, o.url);
            w.signing_key = o.block_signing_key;
            w.props = o.props;  // 체인 파라미터 제안 업데이트
        });
    } else {
        // 새 witness 생성
        _db.create<witness_object>([&](witness_object& w) {
            w.owner = o.owner;
            from_string(w.url, o.url);
            w.signing_key = o.block_signing_key;
            w.created = _db.head_block_time();
            w.props = o.props;
        });
    }
}
```

### 방법 2: witness_set_properties_operation (HF20+)

**현대적이고 유연한 operation** ([steem_evaluator.cpp:131](../src/core/chain/steem_evaluator.cpp#L131)):

```cpp
struct witness_set_properties_operation {
    account_name_type owner;
    flat_map<string, vector<char>> props;  // 키-값 속성
    extensions_type extensions;
};
```

**지원되는 속성:**
- `"key"` - 현재 서명 키 (인증에 필요)
- `"new_signing_key"` - 전환할 새 서명 키
- `"account_creation_fee"` - 제안된 계정 생성 수수료
- `"maximum_block_size"` - 제안된 최대 블록 크기
- `"sbd_interest_rate"` - 제안된 SBD 이자율
- `"account_subsidy_limit"` - 제안된 계정 보조금 한도
- `"sbd_exchange_rate"` - Price feed (다음 섹션 참조)
- `"url"` - Witness 정보 URL

**사용 예:**
```json
{
  "owner": "witness-account",
  "props": {
    "key": "STM_CURRENT_SIGNING_KEY",
    "account_creation_fee": "3.000 STEEM",
    "maximum_block_size": 65536,
    "sbd_interest_rate": 1000,
    "url": "https://witness.example.com"
  }
}
```

**구현:**
```cpp
void witness_set_properties_evaluator::do_apply(const witness_set_properties_operation& o) {
    // 현재 서명 키 확인
    auto itr = o.props.find("key");
    fc::raw::unpack_from_vector(itr->second, signing_key);
    FC_ASSERT(signing_key == witness.signing_key, "'key' does not match witness signing key");

    // 개별 속성 업데이트
    if (auto itr = o.props.find("account_creation_fee"); itr != o.props.end()) {
        fc::raw::unpack_from_vector(itr->second, props.account_creation_fee);
        account_creation_changed = true;
    }

    if (auto itr = o.props.find("maximum_block_size"); itr != o.props.end()) {
        fc::raw::unpack_from_vector(itr->second, props.maximum_block_size);
        max_block_changed = true;
    }

    // ... 다른 속성에 대해서도 유사

    // 변경사항 적용
    _db.modify(witness, [&](witness_object& w) {
        if (account_creation_changed) w.props.account_creation_fee = props.account_creation_fee;
        if (max_block_changed) w.props.maximum_block_size = props.maximum_block_size;
        if (sbd_interest_changed) w.props.sbd_interest_rate = props.sbd_interest_rate;
        // ...
    });
}
```

## Price Feed

### 목적

Witness는 다음을 위해 **STEEM/SBD price feed**를 게시합니다:
- SBD에서 STEEM으로의 전환
- 보상 계산
- 부채 비율 모니터링

### Feed 게시

**Operation: feed_publish_operation** ([steem_evaluator.cpp:1931](../src/core/chain/steem_evaluator.cpp#L1931))

```cpp
struct feed_publish_operation {
    account_name_type publisher;    // Witness 계정
    price exchange_rate;            // SBD/STEEM 가격
};
```

**Price 구조:**
```cpp
struct price {
    asset base;   // 예: "1.000 SBD"
    asset quote;  // 예: "3.500 STEEM"
};
// 의미: 1 SBD = 3.5 STEEM
```

**구현:**
```cpp
void feed_publish_evaluator::do_apply(const feed_publish_operation& o) {
    // 가격 형식 검증
    FC_ASSERT(is_asset_type(o.exchange_rate.base, SBD_SYMBOL) &&
              is_asset_type(o.exchange_rate.quote, STEEM_SYMBOL),
              "Price feed must be a SBD/STEEM price");

    const auto& witness = _db.get_witness(o.publisher);
    _db.modify(witness, [&](witness_object& w) {
        w.sbd_exchange_rate = o.exchange_rate;
        w.last_sbd_exchange_update = _db.head_block_time();
    });
}
```

### 중앙값 Price Feed

블록체인은 모든 witness feed에서 **중앙값 가격**을 계산합니다:

**Feed History Object:**
- 최근 price feed 추적
- 시간 윈도우에 걸쳐 중앙값 계산
- 전환 및 부채 비율에 사용

**중앙값 계산:**
1. 모든 witness price feed 수집
2. 가격 비율로 정렬
3. 중간값 선택 (중앙값)
4. `feed_history_object.current_median_history` 업데이트

**사용:**
```cpp
const auto& fhistory = _db.get_feed_history();
FC_ASSERT(!fhistory.current_median_history.is_null(),
          "Cannot convert SBD because there is no price feed");

// 전환에 중앙값 가격 사용
price median_price = fhistory.current_median_history;
```

## Witness Schedule Object

### 구조

`witness_schedule_object`는 witness 스케줄링 상태를 유지합니다 ([witness_objects.hpp:163](../src/core/chain/include/steem/chain/witness_objects.hpp#L163)):

```cpp
struct witness_schedule_object {
    id_type id;

    fc::uint128 current_virtual_time;                          // 스케줄링을 위한 현재 가상 시간
    uint32_t next_shuffle_block_num = 1;                       // 다음 셔플을 위한 블록 번호
    fc::array<account_name_type, STEEM_MAX_WITNESSES>         // 활성 witness 스케줄
        current_shuffled_witnesses;
    uint8_t num_scheduled_witnesses = 1;                       // 활성 witness 수

    // Witness 카테고리 가중치 (선택 알고리즘용)
    uint8_t top20_weight = 1;
    uint8_t timeshare_weight = 5;

    uint32_t witness_pay_normalization_factor = 25;

    chain_properties median_props;                             // Witness 제안의 중앙값
    version majority_version;                                  // 합의 버전

    uint8_t max_voted_witnesses = STEEM_MAX_VOTED_WITNESSES;
    uint8_t max_runner_witnesses = STEEM_MAX_RUNNER_WITNESSES;
    uint8_t hardfork_required_witnesses = STEEM_HARDFORK_REQUIRED_WITNESSES;
};
```

### 업데이트 빈도

**중앙값 속성이 업데이트되는 시점:**
- 매 witness 스케줄 라운드
- Witness 세트 변경 후
- ~63초 (21 블록 @ 3초/블록)

## 파라미터 변경 프로세스

### 단계별: 파라미터 변경 방법

**1. Witness가 새 값 제안**
```bash
# CLI wallet을 통해
update_witness "witness-account" "https://witness.com" \
    "STM_SIGNING_KEY" \
    '{"account_creation_fee":"5.000 STEEM", "maximum_block_size":131072, "sbd_interest_rate":500}' \
    true
```

**2. 트랜잭션 브로드캐스트 및 확인**
- Operation이 블록에 포함됨
- Witness object가 새 제안으로 업데이트됨
- 체인 파라미터에 즉각적인 영향 없음

**3. 중앙값 재계산 (다음 라운드)**
- 새 witness 스케줄 계산
- 모든 21명의 활성 witness 제안의 중앙값 계산
- Witness schedule object가 새 중앙값으로 업데이트됨

**4. 파라미터 활성화**
- 새 중앙값이 `dynamic_global_property_object`에 복사됨
- 후속 블록에 대해 효과 발생
- 더 많은 witness가 업데이트함에 따라 점진적으로 조정

### 예: 계정 생성 수수료 변경

**현재 상태:**
```
21명의 witness 제안:
Witness 1-10:  3.000 STEEM
Witness 11-20: 3.000 STEEM
Witness 21:    3.000 STEEM
중앙값: 3.000 STEEM ← 활성 파라미터
```

**Witness 1-11이 제안을 5.000 STEEM으로 변경:**
```
Witness 1-11:  5.000 STEEM
Witness 12-20: 3.000 STEEM
Witness 21:    3.000 STEEM
정렬됨: [3,3,3,3,3,3,3,3,3,3,3,5,5,5,5,5,5,5,5,5,5]
중앙값 (위치 11): 5.000 STEEM ← 새 활성 파라미터
```

**결과:**
- 파라미터를 변경하려면 11명 이상의 witness(과반수) 필요
- 부드러운 전환 (중앙값이 점진적으로 이동)
- 단일 witness가 극단값을 강제할 수 없음

## 파라미터 모니터링

### API를 통해

**Dynamic Global Properties 가져오기:**
```bash
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"database_api.get_dynamic_global_properties",
  "params":{},
  "id":1
}' http://localhost:8080
```

**Witness Schedule 가져오기:**
```bash
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"database_api.get_witness_schedule",
  "params":{},
  "id":1
}' http://localhost:8080
```

**특정 Witness 가져오기:**
```bash
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"database_api.get_witness",
  "params":{"owner":"witness-account"},
  "id":1
}' http://localhost:8080
```

### CLI Wallet을 통해

```bash
# Witness 정보 가져오기
>>> get_witness "witness-account"

# Global properties 가져오기
>>> info

# Witness schedule 가져오기
>>> get_witness_schedule
```

## 보안 고려사항

### 파라미터 조작

**보호 메커니즘:**
1. **중앙값 투표**: 변경하려면 11명 이상의 witness 필요
2. **하드 제한**: 프로토콜이 최소/최대 경계 강제
3. **점진적 변경**: 중앙값이 점진적으로 이동
4. **스테이크 투표**: Witness가 스테이크홀더에 의해 선출됨

**공격 시나리오:**
- **카르텔 공격**: 11명 이상의 witness가 공모
  - 완화: 스테이크홀더 투표가 악의적 witness 제거
- **극단값**: 단일 witness가 0 또는 최대값 제안
  - 완화: 중앙값이 이상값 무시
- **빠른 변경**: 빈번한 파라미터 업데이트
  - 완화: 중앙값이 이동하는 데 시간 소요

### 거버넌스 위험

**중앙화:**
- 소수의 엔티티가 많은 상위 witness를 통제하는 경우
- 파라미터 변경을 조율할 수 있음
- 스테이크홀더 투표가 핵심 방어

**조율:**
- Witness가 "리더" witness를 따를 수 있음
- 제안의 다양성 감소
- 커뮤니티 토론이 독립성 장려

## Witness를 위한 모범 사례

### 파라미터 제안

1. **커뮤니티 요구사항 조사**
   - 블록체인 메트릭 모니터링
   - 커뮤니티와 소통
   - 기술적 영향 고려

2. **점진적 조정**
   - 작은 증분 변경
   - 추가 변경 전 효과 관찰
   - 다른 witness와 합의 구축

3. **근거 문서화**
   - Witness 페이지에 근거 게시
   - 기술적 정당성 설명
   - 커뮤니티 피드백에 응답

4. **Price Feed**
   - 정기적으로 업데이트 (최소 매일)
   - 신뢰할 수 있는 가격 소스 사용
   - 여러 거래소 평균

5. **버전 투표**
   - Testnet에서 새 버전 테스트
   - Hardfork 활성화 조율
   - 네트워크 호환성 모니터링

### 모니터링

**추적할 메트릭:**
- 블록 크기 활용률
- 계정 생성 속도
- SBD peg 안정성
- 네트워크 참여율
- 트랜잭션 수수료 및 볼륨

**파라미터 조정 시점:**
- 블록이 크기 제한에 자주 근접
- 계정 생성 적체
- SBD가 peg에서 크게 벗어남
- 네트워크 혼잡 또는 스팸

## 관련 문서

- [Block Confirmation Process](block-confirmation.md) - Witness가 블록을 생성하는 방법
- [Witness Plugin](../development/plugin.md) - Witness 노드 실행
- [Genesis Launch](../operations/genesis-launch.md) - 초기 파라미터 설정
- [Node Types Guide](../operations/node-types-guide.md) - Witness 노드 구성

## 요약

**핵심 요점:**
- ✅ **두 가지 파라미터 유형**: Dynamic (자동 계산) 및 Governed (witness 합의)
- ✅ **중앙값 투표**: 파라미터를 변경하려면 21명 중 11명 이상의 witness 필요
- ✅ **네 가지 주요 파라미터**: 계정 생성 수수료, 블록 크기, SBD 이자, 계정 보조금
- ✅ **Price feed**: Witness가 전환을 위해 STEEM/SBD 가격 게시
- ✅ **점진적 변경**: Witness가 업데이트함에 따라 중앙값이 점진적으로 이동
- ✅ **스테이크홀더 감독**: 유권자가 파라미터를 남용하는 witness를 제거할 수 있음
- ✅ **두 가지 업데이트 방법**: 레거시 `witness_update` 및 현대적 `witness_set_properties` (HF20+)

**파라미터 변경 흐름:**
```
Witness 제안 → Tx 확인 → 중앙값 재계산 → 파라미터 활성화
                                   (63초마다)
```
