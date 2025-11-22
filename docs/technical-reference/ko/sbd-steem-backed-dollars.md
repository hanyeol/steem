# SBD (Steem Backed Dollars) 기술 참조 문서

## 목차

1. [개요](#개요)
2. [목적과 설계](#목적과-설계)
3. [SBD 발행 메커니즘](#sbd-발행-메커니즘)
4. [부채 비율과 발행률](#부채-비율과-발행률)
5. [SBD 변환](#sbd-변환)
6. [저축 계좌 이자](#저축-계좌-이자)
7. [가격 피드 메커니즘](#가격-피드-메커니즘)
8. [가상 공급량 계산](#가상-공급량-계산)
9. [구현 세부사항](#구현-세부사항)
10. [경제적 영향](#경제적-영향)
11. [모범 사례](#모범-사례)

---

## 개요

**SBD (Steem Backed Dollars)**는 Steem 블록체인의 부채 상품으로, $1 USD에 대한 소프트 페그(soft peg)를 유지하도록 설계되었습니다. SBD는 약 $1 상당의 STEEM에 대한 청구권을 나타내며, 변환 메커니즘을 통해 현재 시장 가격으로 STEEM으로 전환할 수 있습니다.

### 주요 특성

- **심볼**: `SBD`
- **정밀도**: 소수점 3자리 (최소 단위 0.001 SBD)
- **목표 가치**: 약 $1 USD
- **담보**: 중앙값 가격 피드 기준으로 STEEM으로 전환 가능
- **부채 상품**: STEEM 공급량을 담보로 하는 블록체인 부채
- **이자 지급**: 저축 계좌에 보관 시 이자 지급 가능 (증인이 활성화한 경우)

### 설계 목표

1. **가격 안정성**: USD에 페그된 안정적인 교환 수단 제공
2. **낮은 변동성**: STEEM 가격 변동에 대한 노출 감소
3. **전환성**: STEEM으로의 전환 메커니즘 유지
4. **지속가능성**: 과도한 블록체인 부채 방지를 위한 SBD 공급 제한
5. **유동성**: 내부 시장 및 외부 거래소에서의 거래 허용

---

## 목적과 설계

### SBD가 존재하는 이유

SBD는 암호화폐 생태계의 여러 문제를 해결하기 위해 만들어졌습니다:

1. **가격 변동성**: STEEM 가격이 변동하여 콘텐츠와 서비스 가격 책정이 어려움
2. **상인 채택**: 상인들은 가격 책정을 위해 안정적인 통화를 선호
3. **예측 가능한 보상**: 콘텐츠 제작자는 보상의 USD 가치를 알고 싶어함
4. **가치 저장**: 사용자는 구매력을 유지하기 위해 안정적인 자산이 필요

### SBD의 페그 유지 방법

SBD는 $1 페그를 유지하기 위해 여러 메커니즘을 사용합니다:

#### 1. 변환 메커니즘

사용자는 증인들이 발행하는 **중앙값 가격 피드** 기준으로 SBD를 STEEM으로 전환할 수 있습니다:

```
변환: 1 SBD → $1 상당의 STEEM (중앙값 가격 기준)
```

**예시**:
- 중앙값 가격 피드: 1 STEEM = $0.50
- 사용자 변환: 1 SBD → 2 STEEM ($1 상당)

이는 $1에 **가격 하한선**을 만듭니다:
- SBD가 $1 미만으로 거래되면, 차익거래자들이 SBD를 구매하여 STEEM으로 전환해 이익을 얻음
- 이러한 구매 압력이 SBD 가격을 $1로 되돌림

#### 2. 높은 부채에서의 보상 감소

SBD 공급량이 STEEM 시가총액 대비 너무 커지면, 블록체인은 새로운 SBD 발행을 줄이거나 중단합니다 ([부채 비율](#부채-비율과-발행률) 참조).

#### 3. 내부 시장

블록체인은 STEEM/SBD 거래를 위한 탈중앙화 거래소를 제공하여 가격 발견과 차익거래를 가능하게 합니다.

### SBD vs 스테이블코인

SBD는 현대의 스테이블코인과 다릅니다:

| 특징 | SBD | USDT/USDC | DAI |
|------|-----|-----------|-----|
| **담보** | STEEM (변동성 암호화폐) | USD 준비금 | 암호화폐 담보 |
| **페그 메커니즘** | 변환 + 시장 | 상환 | 청산 |
| **공급 제한** | 있음 (부채 비율) | 없음 | 없음 |
| **이자** | 선택적 (증인 설정) | 없음 | DSR (저축률) |
| **탈중앙화** | 완전 | 중앙화 | 탈중앙화 |

**중요**: SBD는 **소프트 페그**이며, 하드 페그가 아닙니다. $1 위나 아래로 거래될 수 있습니다.

---

## SBD 발행 메커니즘

### SBD는 언제 생성되는가?

SBD는 **콘텐츠 보상 분배 시에만** 생성됩니다. 다음을 통해서는 생성되지 않습니다:
- 인플레이션
- 증인 보상
- 베스팅 펀드 이자
- 사용자 오퍼레이션

### 보상 분배 비율

포스트/댓글이 7일 보상 지급 기간 후 보상을 받을 때, 총 보상은 다음과 같이 분배됩니다:

**저자용** (총 보상의 75%):
- **50%는 STEEM Power로** (베스팅 셰어)
- **50%는 유동성 보상으로**:
  - `sbd_print_rate = 100%`인 경우: 전액 SBD로 지급
  - `sbd_print_rate < 100%`인 경우: SBD와 STEEM으로 분할

**큐레이터용** (총 보상의 25%):
- **100% STEEM Power로** (큐레이터에게는 SBD 없음)

### SBD 발행률 계산

발행되는 SBD 양은 `sbd_print_rate`에 따라 달라집니다:

**코드 구현** ([database.cpp:1075](libraries/chain/database.cpp#L1075)):

```cpp
const auto& median_price = get_feed_history().current_median_history;
const auto& gpo = get_dynamic_global_properties();

if (!median_price.is_null())
{
   auto to_sbd = (gpo.sbd_print_rate * steem.amount) / STEEM_100_PERCENT;
   auto to_steem = steem.amount - to_sbd;

   auto sbd = asset(to_sbd, STEEM_SYMBOL) * median_price;

   // to_sbd 만큼 SBD로 지급
   // to_steem 만큼 STEEM으로 지급
}
```

**공식**:
```
SBD 양 = (총 저자 유동성 보상 × sbd_print_rate / 100%) × 중앙값 가격
STEEM 양 = 총 저자 유동성 보상 - (총 저자 유동성 보상 × sbd_print_rate / 100%)
```

**예시 1: 전체 SBD 발행** (`sbd_print_rate = 100%`)
```
총 저자 보상: 100 STEEM
- 50 STEEM → STEEM Power
- 50 STEEM → 유동성:
  - to_sbd = 50 × 100% = 50 STEEM 상당
  - to_steem = 50 - 50 = 0 STEEM
  - 중앙값 가격: 1 STEEM = $0.50
  - 지급된 SBD: 50 STEEM × $0.50 = $25 = 25 SBD
```

**예시 2: 50% SBD 발행** (`sbd_print_rate = 50%`)
```
총 저자 보상: 100 STEEM
- 50 STEEM → STEEM Power
- 50 STEEM → 유동성:
  - to_sbd = 50 × 50% = 25 STEEM 상당
  - to_steem = 50 - 25 = 25 STEEM
  - 중앙값 가격: 1 STEEM = $0.50
  - 지급된 SBD: 25 STEEM × $0.50 = $12.50 = 12.5 SBD
  - 지급된 STEEM: 25 STEEM
```

**예시 3: SBD 발행 없음** (`sbd_print_rate = 0%`)
```
총 저자 보상: 100 STEEM
- 50 STEEM → STEEM Power
- 50 STEEM → 유동성:
  - 전액 STEEM으로 지급 (SBD 생성 없음)
```

---

## 부채 비율과 발행률

### 부채 비율 문제

SBD는 **부채 상품**입니다 - 각 SBD는 약 $1 상당의 STEEM에 대한 청구권을 나타냅니다. STEEM 시가총액 대비 SBD가 너무 많이 존재하면, 블록체인은 전환을 보장할 수 없습니다.

### 부채 비율 계산

```
부채 비율 = (현재 SBD 공급량 × 중앙값 STEEM 가격) / (가상 STEEM 공급량 × 중앙값 STEEM 가격)
         = 현재 SBD 공급량 / 가상 STEEM 공급량 × 100%
```

**단순화**:
```
부채 비율 = SBD 시가총액 / STEEM 시가총액
```

**예시**:
```
현재 SBD 공급량: 15,000,000 SBD
가상 STEEM 공급량: 400,000,000 STEEM
중앙값 가격: 1 STEEM = $0.50

SBD 시가총액: 15,000,000 SBD × $1 = $15,000,000
STEEM 시가총액: 400,000,000 STEEM × $0.50 = $200,000,000

부채 비율 = $15,000,000 / $200,000,000 = 7.5%
```

### 발행률 조정

블록체인은 부채 비율에 따라 `sbd_print_rate`를 자동으로 조정합니다:

**상수** ([config.hpp:189-190](libraries/protocol/include/steem/protocol/config.hpp#L189-L190)):
```cpp
#define STEEM_SBD_STOP_PERCENT    (5 * STEEM_1_PERCENT)  // 5%
#define STEEM_SBD_START_PERCENT   (2 * STEEM_1_PERCENT) // 2%
```

**발행률 규칙**:

| 부채 비율 | sbd_print_rate | 효과 |
|-----------|----------------|------|
| **< 2%** | **100%** | 전체 SBD 발행 (저자 유동성 보상의 50%) |
| **2% - 5%** | **선형 감소** | SBD 발행을 점진적으로 감소 |
| **≥ 5%** | **0%** | SBD 발행 중단 (전액 STEEM 보상) |

**선형 감소 공식** (2% ≤ 부채 비율 < 5%):
```
sbd_print_rate = ((5% - 부채 비율) / (5% - 2%)) × 100%
               = ((5% - 부채 비율) / 3%) × 100%
```

**예시**:

```
부채 비율 1%:  sbd_print_rate = 100% (전체 SBD)
부채 비율 2%:  sbd_print_rate = 100% (임계값 시작)
부채 비율 3%:  sbd_print_rate = 66.7% ((5-3)/3 = 67%)
부채 비율 4%:  sbd_print_rate = 33.3% ((5-4)/3 = 33%)
부채 비율 5%:  sbd_print_rate = 0%   (SBD 없음)
부채 비율 10%: sbd_print_rate = 0%   (SBD 없음)
```

### 이러한 임계값을 사용하는 이유?

**보수적인 부채 관리**:
- **2% 임계값**: SBD 생성을 줄이기 위한 조기 경고
- **5% 한도**: 과도한 부채 축적 방지
- **선형 감소**: 부드러운 전환으로 급격한 변화 방지

**역사적 맥락**:
- 높은 부채 비율에서는 전환 보장이 신뢰할 수 없게 됨
- 부채 비율이 100%를 초과하면 블록체인은 모든 전환을 이행할 수 없음
- 보수적인 한도가 장기적 지속가능성 보장

### 코드에서의 발행률 조정

발행률은 현재 부채 비율에 따라 블록 처리 중 업데이트됩니다. 계산은 블록체인의 유지보수 작업에서 발생합니다.

**결과**:
- `dynamic_global_property_object.sbd_print_rate`가 매 블록마다 업데이트됨
- 이 값은 보상 분배에 사용됨 ([database.cpp:1075](libraries/chain/database.cpp#L1075))

---

## SBD 변환

### 두 가지 유형의 변환

#### 1. SBD에서 STEEM으로 변환 (convert_operation)

사용자는 **중앙값 가격 피드** 기준으로 SBD를 STEEM으로 전환할 수 있습니다.

**오퍼레이션** ([steem_operations.hpp:590-595](libraries/protocol/include/steem/protocol/steem_operations.hpp#L590-L595)):
```cpp
struct convert_operation : public base_operation {
   account_name_type owner;
   uint32_t          requestid = 0;
   asset             amount;     // 전환할 SBD
};
```

**프로세스**:
1. 사용자가 SBD 금액과 함께 `convert_operation` 제출
2. SBD가 사용자 잔고에서 즉시 제거됨
3. **3.5일** 후 전환 완료 (STEEM_CONVERSION_DELAY)
4. 사용자는 **전환 시점의 중앙값 가격**을 기준으로 STEEM을 받음

**예시**:
```
사용자 변환: 100 SBD
전환 시 중앙값 가격: 1 STEEM = $0.40

받는 STEEM = 100 SBD / STEEM당 $0.40 = 250 STEEM
```

**중요 사항**:
- **3.5일 지연**: 가격 피드 조작 방지
- **중앙값 가격**: 3.5일 증인 피드의 중앙값 사용
- **일방향**: 이 방법으로 STEEM을 SBD로 전환할 수 없음
- **가격 위험**: 전환 기간 중 STEEM 가격이 변할 수 있음

#### 2. STEEM에서 SBD로 변환 (간접 - 내부 시장)

STEEM을 SBD로 전환하는 **직접 오퍼레이션은 없습니다**. 사용자는 다음을 사용해야 합니다:
- 내부 시장을 사용하여 STEEM을 SBD로 거래
- 또는 외부 거래소 사용

### 전환 지연

**상수** ([config.hpp:146](libraries/protocol/include/steem/protocol/config.hpp#L146)):
```cpp
#define STEEM_CONVERSION_DELAY  (fc::days(3.5))
```

**왜 3.5일인가?**
- 가격 피드 조작에 대한 빠른 차익거래 방지
- 중앙값 가격이 안정화되도록 허용
- 가격 오라클 공격으로부터 블록체인 보호

### 전환 경제학

**가격 하한선**:
전환 메커니즘은 SBD에 대해 $1의 **가격 하한선**을 만듭니다:

```
시장에서 SBD < $1인 경우:
→ $1 미만으로 SBD 구매
→ $1 상당의 STEEM으로 전환
→ STEEM을 판매하여 이익
→ 이러한 구매 압력이 SBD를 $1로 되돌림
```

**가격 상한선 없음**:
SBD는 $1 **이상**으로 거래될 수 있는데, 이유는:
- STEEM에서 SBD를 생성하는 메커니즘 없음
- 제한된 공급량 (부채 비율 제한)
- 시장 수요가 공급을 초과할 수 있음

**역사적 사례**:
- 2017-2018: 암호화폐 강세장 동안 SBD가 $13까지 거래됨
- 전환 메커니즘은 하방 보호만 제공하며, 상방 보호는 제공하지 않음

---

## 저축 계좌 이자

### 저축 계좌 기능

사용자는 SBD (또는 STEEM)를 **저축 계좌**에 예치할 수 있습니다:
- 3일 인출 기간 필요
- 이자 지급 가능 (증인이 활성화한 경우)
- 계정 해킹에 대한 추가 보안 제공

### 이자율

**동적 속성**:
```cpp
uint16_t sbd_interest_rate = 0;  // 베이시스 포인트 (1%의 1/100)
```

**증인이 설정**:
- 각 증인이 이자율 제안
- 활성 증인 21명 전체 제안의 **중앙값**이 활성 이자율이 됨
- **베이시스 포인트**로 표현: 100 bp = 연 1%

**예시**:
```
sbd_interest_rate = 0     → 연 0% (이자 없음)
sbd_interest_rate = 100   → 연 1%
sbd_interest_rate = 1000  → 연 10%
sbd_interest_rate = 1500  → 연 15%
```

### 이자 계산

이자는 지속적으로 계산되며 복리로 적용됩니다:

**공식**:
```
초당 이자 = (잔고 × 이자율) / (100 × 100 × 연간 초 수)
          = (잔고 × 이자율) / (10000 × 31536000)
```

**예시** (연 10% = 1000 bp):
```
저축 중인 SBD: 1000 SBD
이자율: 1000 bp (연 10%)
기간: 1년

적립된 이자 = 1000 SBD × 10% = 100 SBD
```

### 현재 상태

**중요**: 최근 이력 기준으로, SBD 이자는 일반적으로 증인 합의에 의해 **0%로 설정**되어 있는데, 이유는:
- 부채 비율 우려
- SBD 공급 제한에 집중
- 이자 없음을 선호하는 시장 역학

---

## 가격 피드 메커니즘

### 목적

증인들은 다음에 사용되는 STEEM/SBD 환율을 확립하기 위해 **가격 피드**를 발행합니다:
- SBD 전환
- 보상 계산
- 부채 비율 모니터링
- 내부 시장 참조

### 가격 피드 구조

**오퍼레이션** ([steem_operations.hpp:668-672](libraries/protocol/include/steem/protocol/steem_operations.hpp#L668-L672)):
```cpp
struct feed_publish_operation : public base_operation {
   account_name_type publisher;    // 증인 계정
   price exchange_rate;            // STEEM/SBD 가격
};
```

**가격 객체**:
```cpp
struct price {
   asset base;   // 예: "1.000 SBD"
   asset quote;  // 예: "2.500 STEEM"
};
// 해석: 1 SBD = 2.5 STEEM
// 또는: 1 STEEM = $0.40
```

### 중앙값 가격 피드

블록체인은 모든 증인 가격 피드의 **중앙값**을 계산합니다:

**프로세스**:
1. 각 증인이 가격 피드 발행 (이상적으로는 매시간)
2. 블록체인이 최근 피드 이력 유지
3. 모든 활성 증인 피드에서 **중앙값** 가격 계산
4. 중앙값이 전환과 부채 비율에 사용됨

**왜 중앙값인가?**
- 단일 증인의 가격 조작 방지
- 이상값에 대한 저항성
- 중앙값을 이동시키려면 11명 이상의 증인 필요 (다수 합의)
- 평균보다 더 안정적

### 피드 이력

**객체**: `feed_history_object`
- 최근 가격 피드 추적
- 이동 중앙값 유지
- 증인이 발행할 때마다 업데이트

**접근**:
```cpp
const auto& feed_history = db.get_feed_history();
price median_price = feed_history.current_median_history;
```

### 가격 피드 모범 사례

**증인용**:
1. ✅ 적어도 매시간 피드 업데이트
2. ✅ 여러 신뢰할 수 있는 거래소 소스 사용
3. ✅ 가격 평균화 (변동성 감소)
4. ✅ $1로의 약간의 편향 (1% 제안)
5. ❌ 단일 소스 사용 금지
6. ❌ 오래된 데이터 발행 금지

**가격 피드 공식**:
```
중앙값 가격 = median(
   Binance_STEEM_USD,
   Upbit_STEEM_USD,
   Bittrex_STEEM_USD,
   ...
)
```

---

## 가상 공급량 계산

### 가상 공급량이란?

**가상 공급량**은 다음을 포함하여 블록체인의 모든 STEEM 기반 자산의 **총 가치**를 나타내는 계산된 값입니다:
- 유동성 STEEM
- 베스팅된 STEEM (STEEM Power)
- **SBD 부채** (STEEM 등가물로 전환)

### 계산

**공식**:
```cpp
virtual_supply = current_supply + (current_sbd_supply × median_price)
```

**구성 요소**:
- `current_supply`: 실제 유통 중인 STEEM 토큰
- `current_sbd_supply`: 존재하는 총 SBD
- `median_price`: SBD당 STEEM (USD 가격의 역수)

**예시**:
```
current_supply: 350,000,000 STEEM
current_sbd_supply: 15,000,000 SBD
median_price: 1 SBD = 2 STEEM (즉, 1 STEEM = $0.50)

virtual_supply = 350,000,000 + (15,000,000 × 2)
               = 350,000,000 + 30,000,000
               = 380,000,000 STEEM
```

### 가상 공급량이 중요한 이유

**1. 부채 비율 계산**:
```
debt_ratio = current_sbd_supply / virtual_supply
```

**2. 인플레이션 계산**:
인플레이션 보상은 가상 공급량을 기반으로 함 ([database.cpp:1829](libraries/chain/database.cpp#L1829)):
```cpp
auto new_steem = (props.virtual_supply.amount × current_inflation_rate)
                 / (STEEM_100_PERCENT × STEEM_BLOCKS_PER_YEAR);
```

**3. 경제 지표**:
- STEEM 기준 "총 가치" 나타냄
- APR 계산에 사용
- 시가총액 등가물

### 가상 공급량 업데이트

가상 공급량은 다음의 경우 업데이트됩니다:
- 새로운 STEEM 생성 (인플레이션)
- SBD 공급 변경 (생성/소멸)
- 전환 발생 (SBD ↔ STEEM)

**코드** ([database.cpp:2002-2003](libraries/chain/database.cpp#L2002-L2003)):
```cpp
modify(props, [&](dynamic_global_property_object& p) {
   p.current_supply += net_steem;
   p.current_sbd_supply -= net_sbd;
   p.virtual_supply += net_steem;
   p.virtual_supply -= net_sbd × get_feed_history().current_median_history;
});
```

---

## 구현 세부사항

### 데이터베이스 객체

#### 동적 글로벌 속성

`dynamic_global_property_object`의 SBD 관련 속성:

```cpp
asset current_sbd_supply = asset(0, SBD_SYMBOL);         // 총 SBD
asset confidential_sbd_supply = asset(0, SBD_SYMBOL);    // 블라인드 SBD
uint16_t sbd_print_rate = STEEM_100_PERCENT;             // 발행률 %
uint16_t sbd_interest_rate = 0;                          // 이자율 (bp)
asset virtual_supply = asset(0, STEEM_SYMBOL);           // 가상 공급량
```

#### 계정 잔고 필드

각 계정은 SBD 잔고 필드를 가집니다:

```cpp
struct account_object {
   asset sbd_balance = asset(0, SBD_SYMBOL);              // 유동성 SBD
   asset savings_sbd_balance = asset(0, SBD_SYMBOL);      // 저축 중인 SBD
   asset reward_sbd_balance = asset(0, SBD_SYMBOL);       // 대기 중인 SBD 보상
   // ...
};
```

### 오퍼레이션

#### convert_operation

**파일**: [libraries/protocol/include/steem/protocol/steem_operations.hpp:590](libraries/protocol/include/steem/protocol/steem_operations.hpp#L590)

```cpp
struct convert_operation : public base_operation {
   account_name_type owner;
   uint32_t          requestid = 0;
   asset             amount;

   void validate() const;
};
```

**검증**:
- `amount`는 SBD여야 함
- `amount`는 > 0이어야 함
- `requestid`는 계정당 고유해야 함

**평가자**: [libraries/chain/steem_evaluator.cpp](libraries/chain/steem_evaluator.cpp)

#### feed_publish_operation

**파일**: [libraries/protocol/include/steem/protocol/steem_operations.hpp:668](libraries/protocol/include/steem/protocol/steem_operations.hpp#L668)

```cpp
struct feed_publish_operation : public base_operation {
   account_name_type publisher;
   price exchange_rate;

   void validate() const;
};
```

**검증**:
- `publisher`는 활성 증인이어야 함
- `exchange_rate`는 SBD/STEEM 형식이어야 함
- 가격은 양수여야 함

### 주요 함수

#### SBD를 사용한 보상 분배

**파일**: [libraries/chain/database.cpp:1070-1090](libraries/chain/database.cpp#L1070-L1090)

```cpp
void database::cashout_comment_helper(const comment_object& comment) {
   const auto& median_price = get_feed_history().current_median_history;
   const auto& gpo = get_dynamic_global_properties();

   if (!median_price.is_null()) {
      auto to_sbd = (gpo.sbd_print_rate × steem.amount) / STEEM_100_PERCENT;
      auto to_steem = steem.amount - to_sbd;

      auto sbd = asset(to_sbd, STEEM_SYMBOL) × median_price;

      if (to_reward_balance) {
         adjust_reward_balance(to_account, sbd);
         adjust_reward_balance(to_account, asset(to_steem, STEEM_SYMBOL));
      } else {
         adjust_balance(to_account, sbd);
         adjust_balance(to_account, asset(to_steem, STEEM_SYMBOL));
      }
   }
}
```

#### 전환 처리

**파일**: [libraries/chain/database.cpp:1984-2005](libraries/chain/database.cpp#L1984-L2005)

```cpp
void database::process_conversions() {
   const auto& conversion_idx = get_index<convert_request_index>()
      .indices().get<by_conversion_date>();

   while (!conversion_idx.empty() &&
          conversion_idx.begin()->conversion_date <= head_block_time()) {
      const auto& conversion = *conversion_idx.begin();
      const auto& owner = get_account(conversion.owner);

      // 중앙값 가격을 기준으로 지급할 STEEM 계산
      asset steem_to_give = conversion.amount × get_feed_history().current_median_history;

      // 잔고와 공급량 업데이트
      adjust_balance(owner, steem_to_give);

      const auto& props = get_dynamic_global_properties();
      modify(props, [&](dynamic_global_property_object& p) {
         p.current_supply += steem_to_give;
         p.current_sbd_supply -= conversion.amount;
         p.virtual_supply += steem_to_give;
         p.virtual_supply -= conversion.amount × get_feed_history().current_median_history;
      });

      remove(conversion);
   }
}
```

### 상수 참조

**파일**: [libraries/protocol/include/steem/protocol/config.hpp](libraries/protocol/include/steem/protocol/config.hpp)

```cpp
// SBD 상수
#define STEEM_SBD_STOP_PERCENT           (5 * STEEM_1_PERCENT)   // 500 bp = 5%
#define STEEM_SBD_START_PERCENT          (2 * STEEM_1_PERCENT)   // 200 bp = 2%
#define STEEM_MIN_PAYOUT_SBD             (asset(20, SBD_SYMBOL)) // 0.020 SBD

// 전환 상수
#define STEEM_CONVERSION_DELAY           (fc::days(3.5))
#define STEEM_CONVERSION_DELAY_PRE_HF_16 (fc::days(7))

// 기본 이자
#define STEEM_DEFAULT_SBD_INTEREST_RATE  0
```

---

## 경제적 영향

### 부채로서의 SBD

**핵심 통찰**: SBD는 블록체인 **부채**이며, 자산이 아닙니다.

**의미**:
1. **부채**: 각 SBD는 STEEM 공급에 대한 청구권
2. **지속 불가능한 성장**: 무제한 SBD 생성은 시스템을 불안정하게 만듦
3. **보수적 제한**: 부채 비율 상한선이 블록체인 지급능력 보호
4. **전환 위험**: 높은 부채 비율은 전환을 신뢰할 수 없게 만듦

### 가격 역학

#### SBD > $1일 때 (프리미엄)

**원인**:
- 안정적인 자산에 대한 높은 수요
- 제한된 공급 (부채 비율 제한)
- 투기 / FOMO
- STEEM에서 SBD를 생성하는 메커니즘 없음

**효과**:
- 보상의 USD 가치가 더 높음
- 저자가 프리미엄의 혜택
- 내부 시장 차익거래 기회
- 외부 시장이 유동성 제공

**역사적**: 강세장 동안 SBD가 $13까지 거래된 적 있음

#### SBD < $1일 때 (할인)

**원인**:
- 낮은 수요 / 매도 압력
- STEEM 담보에 대한 우려
- 일반적인 암호화폐 약세장
- 전환 메커니즘이 하한선 제공해야 함

**효과**:
- 차익거래 기회: SBD 구매, STEEM으로 전환
- 구매 압력이 페그 회복해야 함
- 지속되면 약한 STEEM 펀더멘털을 나타낼 수 있음

**드물게**: 전환 메커니즘이 보통 지속적인 할인 방지

#### SBD가 항상 $1을 유지할 수 없는 이유

**상방 위험**:
- 주문형 SBD 생성 방법 없음 (보상에서만)
- 부채 비율이 총 공급 제한
- 시장 수요가 가용 공급 초과 가능

**하방 보호**:
- 전환이 $1 상당의 STEEM 보장
- SBD < $1일 때 차익거래가 페그 회복

**결과**: SBD는 다음과 같은 **소프트 페그**:
- $1의 강한 하한선 (전환)
- 약한 상한선 (생성 메커니즘 없음)

### 보상에 대한 영향

**높은 SBD 발행률** (100%):
- 저자가 25% 유동성 SBD, 25% 유동성 STEEM 받음 (가격 조정 시)
- USD 기준으로 더 안정적인 보상
- SBD 공급 증가

**낮은 SBD 발행률** (0%):
- 저자가 50% 유동성 STEEM 받음, SBD 없음
- 더 변동성 있는 보상 (STEEM 가격 변동)
- SBD 공급 감소

**트레이드오프**:
- 안정성 vs 공급 통제
- 저자 선호 vs 블록체인 건전성

### 부채 지속가능성

**건전한 부채 비율** (< 2%):
- 전체 SBD 발행
- 전환 완전히 담보됨
- 시스템 지속가능

**경고 구역** (2% - 5%):
- SBD 발행 감소
- 여전히 지속가능
- 주의 신호

**위험 구역** (> 5%):
- 새로운 SBD 생성 없음
- 기존 SBD는 여전히 담보됨
- 부채 감소에 집중

**재앙 시나리오** (> 100%):
- 모든 전환을 이행할 수 없음
- 시스템 신뢰 파괴
- Steem 역사상 발생한 적 없음

---

## 모범 사례

### 사용자용

**SBD 보유**:
1. ✅ 안정적인 가치 저장에 좋음 (페그 상태일 때)
2. ✅ 내부 시장에서 거래 가능
3. ⚠️ $1 이상 거래 가능 (프리미엄 위험)
4. ⚠️ 보상으로 항상 받을 수 있는 것은 아님 (부채 비율)

**SBD 전환**:
1. ✅ SBD < $1일 때 차익거래 이익을 위해 사용
2. ⚠️ 3.5일 지연 - 가격 위험
3. ⚠️ SBD > $1일 때 전환하지 말 것 (프리미엄 손실)
4. ✅ 전환율은 제출 시점에 고정됨

**저축 계좌**:
1. ✅ 추가 보안 (3일 인출)
2. ✅ 이자 받을 수 있음 (증인 설정 확인)
3. ⚠️ 이자가 현재 대부분의 네트워크에서 비활성화됨
4. ⚠️ 3일 락업 기간

### 증인용

**가격 피드**:
1. ✅ 적어도 매시간 업데이트
2. ✅ 여러 거래소 소스 사용
3. ✅ 가격 평균화 (변동성 감소)
4. ✅ $1로의 약간의 편향 (1% 제안)
5. ❌ 단일 소스 사용 금지
6. ❌ 오래된 데이터 발행 금지

**이자율**:
1. ✅ 이자 설정 전 부채 비율 고려
2. ✅ 다른 증인과 조율
3. ⚠️ 높은 이자는 SBD 공급 증가
4. ⚠️ 현재 0 이자 권장됨

**파라미터 모니터링**:
1. 부채 비율 추세 추적
2. SBD 시장 가격 모니터링
3. 발행률 기대치 조율
4. 커뮤니티 피드백에 대응

### 개발자용

**애플리케이션에서 SBD 사용**:
1. ✅ SBD 가용성을 가정하기 전에 `sbd_print_rate` 확인
2. ✅ 중앙값 가격에 `get_feed_history()` 사용
3. ✅ 보상 표시에서 SBD와 STEEM 모두 처리
4. ⚠️ SBD = $1로 가정하지 말 것 (시장 가격 확인)
5. ⚠️ UX에서 전환 지연 고려

**API 쿼리**:
```bash
# 현재 SBD 지표 조회
get_dynamic_global_properties

# 중앙값 가격 피드 조회
get_feed_history

# 증인 가격 피드 조회
get_witnesses

# 계정 SBD 잔고 조회
get_accounts ["username"]
```

---

## 관련 문서

### 핵심 문서
- [베스팅과 보상 시스템](vesting-and-rewards-detailed.md) - 보상 분배 방법
- [내부 시장](internal-market.md) - STEEM/SBD 거래
- [체인 파라미터](chain-parameters.md) - 증인 거버넌스
- [동적 글로벌 속성](dynamic-global-properties.md) - 실시간 블록체인 상태
- [Process Funds 함수](process-funds-function.md) - 인플레이션과 보상 분배

### 운영 가이드
- [오퍼레이션 생성](../development/create-operation.md) - 오퍼레이션 작동 방법
- [CLI 지갑 가이드](../getting-started/cli-wallet-guide.md) - 지갑에서 SBD 사용

### 외부 리소스
- [Steem 백서](https://steem.com/steem-whitepaper.pdf) - 원래 SBD 설계
- [Steem 블루 페이퍼](https://steem.com/SteemWhitePaper.pdf) - 경제 모델

---

## 요약

**핵심 사항**:

1. **목적**: SBD는 $1 페그를 유지하도록 설계된 부채 기반 스테이블코인
2. **생성**: 콘텐츠 보상에서만 생성, 금액은 `sbd_print_rate`로 결정
3. **부채 비율**: STEEM 시가총액의 2-5%로 SBD 공급 자동 제한
4. **전환**: 중앙값 가격으로 일방향 SBD→STEEM 전환 (3.5일 지연)
5. **가격 하한선**: 전환 메커니즘이 강력한 $1 하한선 제공
6. **가격 상한선 없음**: SBD가 $1 이상 거래 가능 (역사적으로 $13까지 도달)
7. **이자**: 저축에 대한 선택적 이자 (증인 설정, 현재 약 0%)
8. **가격 피드**: 증인이 가격 발행, 중앙값이 전환에 사용됨
9. **가상 공급량**: SBD 부채 포함, 인플레이션과 부채 비율 계산에 사용
10. **지속가능성**: 보수적인 부채 제한이 장기적 생존 가능성 보장

**SBD 발행률 규칙**:
```
부채 < 2%:   100% SBD 발행
부채 2-5%:   선형 감소
부채 ≥ 5%:   0% SBD 발행 (STEEM만)
```

**SBD는 다음의 균형을 맞추는 정교한 스테이블코인 메커니즘**:
- ✅ 가격 안정성 (전환 하한선)
- ✅ 공급 지속가능성 (부채 비율 제한)
- ✅ 보상 예측가능성 (USD 기준)
- ⚠️ 소프트 페그 ($1 이상 거래 가능)
- ⚠️ 전환 지연 (3.5일)

---

**문서 버전**: 1.0
**마지막 업데이트**: 2025-11-22
**코드 버전**: STEEM 0.0.0
