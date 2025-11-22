# process_funds() 함수 레퍼런스

## 개요

`process_funds()` 함수는 매 블록마다 호출되어 Steem 인플레이션 메커니즘을 관리하고 새로 생성된 토큰을 다양한 이해관계자에게 분배하는 핵심 블록체인 유지보수 함수입니다. 블록체인의 통화 정책을 구현하여 인플레이션 보상을 계산하고 분배합니다.

**위치:** [libraries/chain/database.cpp:1813](../../libraries/chain/database.cpp#L1813)

## 목적

이 함수는 다음을 처리합니다:
1. 블록 높이에 기반한 현재 인플레이션 비율 계산
2. 인플레이션 스케줄에 따라 새로운 STEEM 토큰 생성
3. 인플레이션 보상을 세 가지 카테고리로 분배:
   - 콘텐츠 보상 (작성자 및 큐레이터용)
   - 베스팅 펀드 (STEEM Power 보유자용)
   - 증인 보상 (블록 생산자용)

## 인플레이션 모델

### 초기 매개변수

Steem 블록체인은 높게 시작하여 시간이 지남에 따라 점진적으로 감소하는 **감소형 인플레이션 모델**을 사용합니다:

- **시작 인플레이션 비율:** 연 9.78% (블록 7,000,000에서) - `STEEM_INFLATION_RATE_START_PERCENT = 978`
- **종료 인플레이션 비율:** 연 0.95% (최저 비율) - `STEEM_INFLATION_RATE_STOP_PERCENT = 95`
- **감소율:** 250,000 블록마다 0.01% - `STEEM_INFLATION_NARROWING_PERIOD = 250000`
- **기간:** 약 20.5년
- **완료:** 블록 220,750,000

**역사적 참고:** 코드 주석에는 "9.5%"로 표기되어 있지만, 실제 구현값은 978 베이시스 포인트로 **9.78%**입니다. 이 값은 2016년 11월 16일 커밋 147d50e2에서 고정 인플레이션(9.5%)을 감소형 인플레이션 모델로 변경할 때 설정되었으며, 이후 변경된 적이 없습니다.

### 인플레이션 비율 계산

```cpp
int64_t start_inflation_rate = STEEM_INFLATION_RATE_START_PERCENT; // 978 (9.78%)
int64_t inflation_rate_adjustment = head_block_num() / STEEM_INFLATION_NARROWING_PERIOD;
int64_t inflation_rate_floor = STEEM_INFLATION_RATE_STOP_PERCENT; // 95 (0.95%)

int64_t current_inflation_rate = std::max(
    start_inflation_rate - inflation_rate_adjustment,
    inflation_rate_floor
);
```

인플레이션 비율은 선형적으로 감소합니다:
- **250,000 블록**마다 (~29일), 비율이 **0.01%** 감소
- 비율은 절대 **0.95%** (최저값) 아래로 내려가지 않음

### 블록당 토큰 생성

```cpp
auto new_steem = (props.virtual_supply.amount * current_inflation_rate)
                 / (STEEM_100_PERCENT * STEEM_BLOCKS_PER_YEAR);
```

여기서:
- `props.virtual_supply` = 현재 STEEM의 총 가상 공급량
- `current_inflation_rate` = 현재 인플레이션 퍼센트 (베이시스 포인트)
- `STEEM_BLOCKS_PER_YEAR` = 10,512,000 (365일 × 24시간 × 60분 × 20블록/분)
- `STEEM_100_PERCENT` = 10,000 (100.00%를 나타냄)

## 보상 분배

새로 생성된 STEEM은 고정된 비율에 따라 분배됩니다:

### 분배 내역

| 카테고리 | 비율 | 상수 | 설명 |
|----------|-----------|----------|-------------|
| **콘텐츠 보상** | 75% | `STEEM_CONTENT_REWARD_PERCENT` | 작성자, 큐레이터, 댓글 보상용 |
| **베스팅 펀드** | 15% | `STEEM_VESTING_FUND_PERCENT` | STEEM Power 이자용 |
| **증인 보상** | 10% | (나머지) | 블록 생산자용 |

### 계산 코드

```cpp
// 75% 콘텐츠 작성자 및 큐레이터에게
auto content_reward = (new_steem * STEEM_CONTENT_REWARD_PERCENT) / STEEM_100_PERCENT;
content_reward = pay_reward_funds(content_reward);

// 15% 베스팅 펀드 (STEEM Power)에게
auto vesting_reward = (new_steem * STEEM_VESTING_FUND_PERCENT) / STEEM_100_PERCENT;

// 나머지 10% 증인에게
auto witness_reward = new_steem - content_reward - vesting_reward;
```

## 콘텐츠 보상 분배

콘텐츠 보상은 `pay_reward_funds()` 함수를 통해 추가 분배됩니다:

```cpp
share_type database::pay_reward_funds(share_type reward)
{
   const auto& reward_idx = get_index<reward_fund_index, by_id>();
   share_type used_rewards = 0;

   for (auto itr = reward_idx.begin(); itr != reward_idx.end(); ++itr)
   {
      // 펀드의 비율에 기반하여 비례 보상 계산
      auto r = (reward * itr->percent_content_rewards) / STEEM_100_PERCENT;

      // 보상 펀드의 잔액에 추가
      modify(*itr, [&](reward_fund_object& rfo)
      {
         rfo.reward_balance += asset(r, STEEM_SYMBOL);
      });

      used_rewards += r;
      FC_ASSERT(used_rewards <= reward); // 안전성 검사
   }

   return used_rewards;
}
```

이 함수는:
1. 모든 보상 펀드 객체를 순회
2. 각 펀드에 콘텐츠 보상의 일정 비율을 할당
3. 각 펀드의 잔액을 업데이트
4. 분배된 총 금액을 반환

## 증인 보상 계산

증인 보상은 상위 증인과 백업 증인 간의 공정한 분배를 보장하기 위해 증인 유형에 따라 **가중치가 적용**됩니다.

### 증인 유형

1. **상위 20 증인** - 지분 가중 투표로 선출
2. **타임쉐어/백업 증인** - 상위 20 외부에 스케줄된 증인

### 가중치 기반 계산

```cpp
const auto& cwit = get_witness(props.current_witness);
witness_reward *= STEEM_MAX_WITNESSES; // 21을 곱함

if (cwit.schedule == witness_object::timeshare)
   witness_reward *= wso.timeshare_weight;      // 기본값: 5
else if (cwit.schedule == witness_object::top20)
   witness_reward *= wso.top20_weight;          // 기본값: 1

witness_reward /= wso.witness_pay_normalization_factor;  // 기본값: 25
```

### 기본 가중치 구성

[witness_objects.hpp:171-173](../../libraries/chain/include/steem/chain/witness_objects.hpp#L171-L173)에서:

```cpp
uint8_t  top20_weight = 1;
uint8_t  timeshare_weight = 5;
uint32_t witness_pay_normalization_factor = 25;
```

### 보상 계산 분석

**상위 20 증인**의 경우:
```
witness_reward = (base_reward * 21 * 1) / 25
               = base_reward * 0.84
```

**타임쉐어/백업 증인**의 경우:
```
witness_reward = (base_reward * 21 * 5) / 25
               = base_reward * 4.2
```

이는 백업 증인이 **블록당 상위 20 증인보다 5배 더 많이** 받지만, 훨씬 적은 블록을 생산하기 때문에 **전체적으로는 상위 20 증인이 더 많이** 받게 되어, 네트워크 보안에 기여하는 상위 증인에게 더 많은 보상을 제공하면서도 백업 증인들도 참여할 인센티브를 유지합니다.

## 전역 상태 업데이트

모든 보상을 계산한 후, 함수는 전역 블록체인 상태를 업데이트합니다:

```cpp
modify(props, [&](dynamic_global_property_object& p)
{
   p.total_vesting_fund_steem += asset(vesting_reward, STEEM_SYMBOL);
   p.current_supply           += asset(new_steem, STEEM_SYMBOL);
   p.virtual_supply           += asset(new_steem, STEEM_SYMBOL);
});
```

### 업데이트된 속성

1. **`total_vesting_fund_steem`** - STEEM Power 이자를 위한 베스팅 보상 누적
2. **`current_supply`** - 총 유통 STEEM 공급량
3. **`virtual_supply`** - 인플레이션 계산에 사용되는 가상 공급량 (SBD 부채 포함)

## 증인 베스팅 생성

마지막으로, 증인 보상은 STEEM Power (베스팅 주식)로 변환되어 블록 생산자에게 지급됩니다:

```cpp
const auto& producer_reward = create_vesting(
    get_account(cwit.owner),
    asset(witness_reward, STEEM_SYMBOL)
);

push_virtual_operation(
    producer_reward_operation(cwit.owner, producer_reward)
);
```

이는:
1. 증인을 위한 베스팅 주식을 생성
2. 인덱싱/추적 목적으로 가상 오퍼레이션을 발행

## 계산 예시

가정:
- 가상 공급량: 1,000,000,000 STEEM
- 블록 높이: 10,000,000
- 현재 인플레이션: 9.68% (978 - 40 = 938 베이시스 포인트)

### 단계 1: 블록당 인플레이션 계산
```
new_steem = (1,000,000,000 × 938) / (10,000 × 10,512,000)
          = 938,000,000,000 / 105,120,000,000
          ≈ 8.92 STEEM per block
```

### 단계 2: 보상 분배
```
content_reward  = 8.92 × 0.75 = 6.69 STEEM  (보상 펀드로)
vesting_reward  = 8.92 × 0.15 = 1.34 STEEM  (베스팅 펀드로)
witness_reward  = 8.92 - 6.69 - 1.34 = 0.89 STEEM (증인에게)
```

### 단계 3: 증인 지급액 계산
**상위 20 증인의 경우:**
```
witness_reward = (0.89 × 21 × 1) / 25 = 0.7476 STEEM (블록당)
```

**Timeshare 증인의 경우:**
```
witness_reward = (0.89 × 21 × 5) / 25 = 3.738 STEEM (블록당)
```

### 단계 4: 전역 상태 업데이트
```
total_vesting_fund_steem += 1.34 STEEM
current_supply           += 8.92 STEEM
virtual_supply           += 8.92 STEEM
```

## 주요 상수

| 상수 | 값 | 설명 |
|----------|-------|-------------|
| `STEEM_INFLATION_RATE_START_PERCENT` | 978 | 시작 인플레이션 비율 (9.78%) |
| `STEEM_INFLATION_RATE_STOP_PERCENT` | 95 | 최저 인플레이션 비율 (0.95%) |
| `STEEM_INFLATION_NARROWING_PERIOD` | 250,000 | 비율 감소 사이의 블록 수 |
| `STEEM_CONTENT_REWARD_PERCENT` | 7,500 | 인플레이션의 75% |
| `STEEM_VESTING_FUND_PERCENT` | 1,500 | 인플레이션의 15% |
| `STEEM_BLOCKS_PER_YEAR` | 10,512,000 | 연간 블록 수 (3초 간격) |
| `STEEM_MAX_WITNESSES` | 21 | 최대 활성 증인 수 |
| `STEEM_100_PERCENT` | 10,000 | 100.00%를 나타냄 |

## 실행 컨텍스트

이 함수는 다음과 같이 호출됩니다:
- **시점:** 블록 처리 중 매 블록마다
- **호출자:** `database::apply_block()` → `process_funds()`
- **빈도:** 3초마다 (한 블록)
- **스레드 안전성:** 데이터베이스 쓰기 잠금으로 보호됨

## 관련 함수

- [`pay_reward_funds()`](../../libraries/chain/database.cpp#L1955) - 콘텐츠 보상을 보상 펀드에 분배
- [`create_vesting()`](../../libraries/chain/database.cpp) - STEEM을 STEEM Power로 변환
- [`process_conversions()`](../../libraries/chain/database.cpp#L1984) - SBD 전환 처리
- [`process_savings_withdraws()`](../../libraries/chain/database.cpp#L1860) - 세이빙스 출금 처리

## 역사적 참고사항

### 인플레이션 모델 변경 이력

1. **초기 설계 (2016년 11월 이전)**
   - 고정 인플레이션: 9.5% (`STEEMIT_INFLATION_RATE_PERCENT = 95 * STEEMIT_1_TENTH_PERCENT`)
   - 함수 주석에 언급된 **102% 인플레이션**은 매우 초기 버전의 내용으로, 오래 전에 폐기되었습니다

2. **현재 설계 (2016년 11월 16일 이후 - 커밋 147d50e2)**
   - 감소형 인플레이션 모델 도입
   - 시작: **9.78%** (`STEEM_INFLATION_RATE_START_PERCENT = 978`)
   - 종료: **0.95%** (`STEEM_INFLATION_RATE_STOP_PERCENT = 95`)
   - 250,000 블록마다 0.01%씩 감소
   - **주의:** 코드 주석에 "9.5%"로 표기되어 있으나, 실제 값은 9.78%입니다

3. **상수명 변경 이력**
   - 2017년 9월: `STEEMIT_` → `STEEM_` 접두사 변경 (값 변경 없음)
   - 2017년 9월: 디렉토리 구조 변경 `steemit/` → `steem/` (값 변경 없음)

### 코드 위치

**파일:** [libraries/protocol/include/steem/protocol/config.hpp:113-115](../../libraries/protocol/include/steem/protocol/config.hpp#L113-L115)

```cpp
#define STEEM_INFLATION_RATE_START_PERCENT    (978) // Fixes block 7,000,000 to 9.5%
#define STEEM_INFLATION_RATE_STOP_PERCENT     (95) // 0.95%
#define STEEM_INFLATION_NARROWING_PERIOD      (250000) // Narrow 0.01% every 250k blocks
```

**계산:**
```
978 basis points / 10000 = 9.78% (not 9.5% as the comment suggests)
```

## 참고 자료

- [증인 보상 시스템](./witness-rewards-bootstrap.md)
- [체인 파라미터 레퍼런스](./chain-parameters.md)
- [베스팅 시스템](./vesting-system.md)
- [보상 펀드 문서](./reward-funds.md)

## 상세 동작 플로우

### 1. 인플레이션 비율 결정 단계

```
블록 높이 확인
    ↓
250,000으로 나누어 감소 단계 계산
    ↓
시작 비율에서 감소량 차감
    ↓
최저 비율(0.95%)과 비교하여 더 큰 값 선택
    ↓
현재 인플레이션 비율 확정
```

### 2. 토큰 생성 및 분배 단계

```
가상 공급량 × 현재 인플레이션 비율 / (100% × 연간 블록 수)
    ↓
블록당 생성될 STEEM 계산 완료
    ↓
75% → 콘텐츠 보상 펀드들에 비례 분배
15% → 베스팅 펀드에 누적
10% → 현재 블록 생산 증인에게 (가중치 적용)
```

### 3. 데이터베이스 업데이트 단계

```
콘텐츠 보상 → reward_fund_object.reward_balance 증가
    ↓
베스팅 보상 → dynamic_global_property_object.total_vesting_fund_steem 증가
    ↓
전체 공급량 → current_supply 및 virtual_supply 증가
    ↓
증인 보상 → create_vesting()로 증인 계정에 SP 지급
    ↓
가상 오퍼레이션 발행 (producer_reward_operation)
```

## 가중치 시스템의 필요성

증인 보상에 가중치가 적용되는 이유:

1. **상위 20 증인**: 라운드당 규칙적으로 블록 생산 (21블록 중 20블록)
2. **백업 증인 (Timeshare)**: 라운드당 한 번만 블록 생산 (21블록 중 1블록)

### 블록당 보상 (기본 보상 0.89 STEEM 가정)

**가중치 없이는:**
- 모든 증인: 블록당 0.89 STEEM

라운드당 총 보상:
- 상위 20 증인 (각): 20블록 × 0.89 = 17.8 STEEM
- 백업 증인 (각): 1블록 × 0.89 = 0.89 STEEM
- **비율**: 20:1 (너무 큰 격차!)

**가중치 적용 후:**
- 상위 20 증인: 블록당 0.7476 STEEM (가중치 1)
- 백업 증인: 블록당 3.738 STEEM (가중치 5)

라운드당 총 보상:
- 상위 20 증인 (각): 20블록 × 0.7476 = **14.952 STEEM**
- 백업 증인 (각): 1블록 × 3.738 = **3.738 STEEM**
- **비율**: 약 4:1 (합리적인 격차)

### 결론

가중치 시스템 덕분에:
- ✅ 상위 20 증인은 여전히 더 많은 보상을 받아 네트워크 보안에 기여할 인센티브 유지
- ✅ 백업 증인도 상위 증인의 약 25%를 받아 네트워크 참여 인센티브 확보
- ✅ 전체 증인 생태계의 건강성과 탈중앙화 유지

### 중요: 블록당 생성량 차이

주목할 점은 **블록마다 생성되는 총 STEEM이 다릅니다**:

```cpp
// 라인 1847: 실제 생성량 재계산
new_steem = content_reward + vesting_reward + witness_reward;
```

- **Top 20 블록**: 6.69 + 1.34 + 0.7476 = **8.7776 STEEM** 생성
- **Timeshare 블록**: 6.69 + 1.34 + 3.738 = **11.768 STEEM** 생성

**개별 블록에서의 비율:**
- Top 20 블록: 콘텐츠 76.2%, 베스팅 15.3%, 증인 8.5%
- Timeshare 블록: 콘텐츠 56.8%, 베스팅 11.4%, 증인 31.8%

**하지만 라운드 평균 (21블록):**
```
총 생성 = (20 × 8.7776) + (1 × 11.768) = 187.32 STEEM
평균 증인 보상 = (14.952 + 3.738) / 187.32 = 9.98% ≈ 10%
```

**결론: 장기 평균으로는 75% : 15% : 10% 비율이 유지됩니다.**

## 시간대별 인플레이션 변화

| 블록 높이 | 경과 시간 | 인플레이션 비율 | 설명 |
|----------|----------|---------------|------|
| 7,000,000 | 시작점 | 9.78% | 감소형 인플레이션 시작 |
| 10,000,000 | ~104일 | 9.66% | 12단계 감소 |
| 50,000,000 | ~4.1년 | 8.06% | 172단계 감소 |
| 100,000,000 | ~8.9년 | 6.06% | 372단계 감소 |
| 150,000,000 | ~13.6년 | 4.06% | 572단계 감소 |
| 200,000,000 | ~18.4년 | 2.06% | 772단계 감소 |
| 220,750,000 | ~20.5년 | 0.95% | 최종 도달 (최저 비율) |
| 그 이후 | ~20.5년+ | 0.95% | 영구 유지 |

## 보안 및 불변성 고려사항

### 오버플로우 방지

```cpp
// 주석: inflation_rate_adjustment는 <2^32이므로
// 아래 뺄셈은 int64_t 언더플로우가 발생하지 않음
int64_t current_inflation_rate = std::max(
    start_inflation_rate - inflation_rate_adjustment,
    inflation_rate_floor
);
```

### 정확성 검증

```cpp
FC_ASSERT(used_rewards <= reward); // 할당된 것보다 많이 지급하지 않도록 보장
```

### 원자성 보장

모든 데이터베이스 수정은 `modify()` 람다 내에서 수행되어 트랜잭션 원자성이 보장됩니다. 중간에 실패하면 모든 변경사항이 롤백됩니다.

## 디버깅 및 모니터링

`process_funds()` 함수의 동작을 추적하려면:

1. **로그 확인**: 증인 보상 계산 시 경고 메시지
   ```cpp
   wlog("Encountered unknown witness type for witness: ${w}", ("w", cwit.owner));
   ```

2. **가상 오퍼레이션 추적**: `producer_reward_operation` 이벤트 모니터링
3. **전역 속성 모니터링**: `get_dynamic_global_properties()` API로 공급량 변화 추적
4. **보상 펀드 잔액**: `reward_fund_object`의 `reward_balance` 필드 확인

## 성능 특성

- **시간 복잡도**: O(n) where n = 보상 펀드 개수 (일반적으로 1-2개로 매우 작음)
- **데이터베이스 쓰기**:
  - 1회 dynamic_global_property_object 수정
  - n회 reward_fund_object 수정
  - 1회 account_object 수정 (증인 베스팅)
- **메모리 할당**: 최소한 (기존 객체만 수정)
- **실행 시간**: 마이크로초 단위 (블록 처리의 극히 일부)
