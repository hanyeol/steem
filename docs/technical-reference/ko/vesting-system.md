# Vesting System (Steem Power)

## 개요

Steem 블록체인의 베스팅(Vesting) 시스템은 **Steem Power(SP)**의 핵심 메커니즘입니다. 사용자가 STEEM을 "파워업(Power Up)"하면 베스팅 주식(VESTS)으로 변환되어 장기 보유를 유도하고, 블록 인플레이션을 통해 자동으로 이자를 받게 됩니다.

이 문서는 베스팅 풀의 동작 원리, VESTS 가격 계산, 파워업/파워다운 메커니즘을 설명합니다.

## 목차

1. [베스팅 풀의 구조](#1-베스팅-풀의-구조)
2. [VESTS 가격 결정 메커니즘](#2-vests-가격-결정-메커니즘)
3. [파워업 (Power Up)](#3-파워업-power-up)
4. [파워다운 (Power Down)](#4-파워다운-power-down)
5. [인플레이션을 통한 자동 이자](#5-인플레이션을-통한-자동-이자)
6. [실제 시나리오 예제](#6-실제-시나리오-예제)

---

## 1. 베스팅 풀의 구조

### 1.1 핵심 구성 요소

**파일:** [src/core/chain/include/steem/chain/global_property_object.hpp:46-47](../../src/core/chain/include/steem/chain/global_property_object.hpp#L46-L47)

```cpp
struct dynamic_global_property_object
{
   asset total_vesting_fund_steem = asset( 0, STEEM_SYMBOL );  // 베스팅 풀의 STEEM
   asset total_vesting_shares     = asset( 0, VESTS_SYMBOL );  // 발행된 VESTS 총량
   // ...
};
```

**베스팅 풀의 2가지 핵심 변수:**

| 변수 | 설명 | 초기값 |
|------|------|--------|
| `total_vesting_fund_steem` | 베스팅 풀에 예치된 총 STEEM | 0 STEEM |
| `total_vesting_shares` | 발행된 베스팅 주식(VESTS) 총량 | 0 VESTS |

### 1.2 베스팅 풀의 3대 역할

1. **예치금 보관소**: 사용자들이 파워업한 STEEM을 보관
2. **가격 계산 기준**: VESTS ↔ STEEM 변환 비율 결정
3. **이자 지급 풀**: 블록 인플레이션의 15%를 받아 SP 보유자에게 분배

### 1.3 제네시스 초기 상태

**파일:** [src/core/chain/database.cpp:2369-2378](../../src/core/chain/database.cpp#L2369-L2378)

```cpp
void database::init_genesis( uint64_t init_supply )
{
   // ...
   create< dynamic_global_property_object >( [&]( dynamic_global_property_object& p )
   {
      p.current_witness = STEEM_GENESIS_WITNESS_NAME;
      p.time = STEEM_GENESIS_TIME;
      p.current_supply = asset( init_supply, STEEM_SYMBOL );
      p.virtual_supply = p.current_supply;
      // total_vesting_fund_steem과 total_vesting_shares는 명시적 초기화 없음
      // 기본값 0으로 시작
   });
}
```

**제네시스 블록 생성 직후:**
```
- total_vesting_fund_steem: 0 STEEM
- total_vesting_shares: 0 VESTS
- 베스팅 풀: 완전히 비어있음
```

---

## 2. VESTS 가격 결정 메커니즘

### 2.1 가격 계산 함수

**파일:** [src/core/chain/include/steem/chain/global_property_object.hpp:53-59](../../src/core/chain/include/steem/chain/global_property_object.hpp#L53-L59)

```cpp
price get_vesting_share_price() const
{
   // 베스팅 풀이 비어있으면 기본 비율 사용
   if ( total_vesting_fund_steem.amount == 0 || total_vesting_shares.amount == 0 )
      return price ( asset( 1000, STEEM_SYMBOL ), asset( 1000000, VESTS_SYMBOL ) );

   // 실제 베스팅 풀 비율로 계산
   return price( total_vesting_shares, total_vesting_fund_steem );
}
```

### 2.2 가격 계산 공식

**경우 1: 베스팅 풀이 비어있을 때 (초기 상태)**
```
기본 가격 = 1000 STEEM : 1,000,000 VESTS
즉, 1 STEEM = 1,000 VESTS
```

**경우 2: 베스팅 풀에 자금이 있을 때**
```
VESTS 가격 = total_vesting_fund_steem / total_vesting_shares

예:
total_vesting_fund_steem = 1,150,000 STEEM
total_vesting_shares = 1,000,000,000 VESTS
→ 1 VESTS = 1,150,000 / 1,000,000,000 = 0.00115 STEEM
→ 1 STEEM = 1,000,000,000 / 1,150,000 ≈ 869.57 VESTS
```

### 2.3 보상용 VESTS 가격

**파일:** [src/core/chain/include/steem/chain/global_property_object.hpp:61-65](../../src/core/chain/include/steem/chain/global_property_object.hpp#L61-L65)

```cpp
price get_reward_vesting_share_price() const
{
   return price( total_vesting_shares + pending_rewarded_vesting_shares,
                 total_vesting_fund_steem + pending_rewarded_vesting_steem );
}
```

보상 지급 시에는 보류 중인 보상까지 포함하여 가격을 계산합니다.

---

## 3. 파워업 (Power Up)

### 3.1 파워업 프로세스

**파일:** [src/core/chain/database.cpp:1134-1165](../../src/core/chain/database.cpp#L1134-L1165)

```cpp
asset database::create_vesting( const account_object& to_account, asset liquid, bool to_reward_balance )
{
   // 1. 현재 VESTS 가격 조회
   const auto& cprops = get_dynamic_global_properties();
   price vesting_share_price = to_reward_balance ?
      cprops.get_reward_vesting_share_price() :
      cprops.get_vesting_share_price();

   // 2. STEEM을 VESTS로 변환
   asset new_vesting = liquid * vesting_share_price;

   // 3. 사용자 계정에 VESTS 추가
   adjust_balance( to_account, new_vesting );

   // 4. 글로벌 베스팅 풀 업데이트
   modify( cprops, [&]( dynamic_global_property_object& props )
   {
      props.total_vesting_fund_steem += liquid;       // STEEM 추가
      props.total_vesting_shares += new_vesting;      // VESTS 발행
   });

   // 5. 증인 투표력 업데이트
   adjust_proxied_witness_votes( to_account, new_vesting.amount );

   return new_vesting;
}
```

### 3.2 파워업 예제

**시나리오: 사용자가 1000 STEEM 파워업**

```
Before (초기 상태):
├─ 베스팅 풀
│  ├─ total_vesting_fund_steem: 0 STEEM
│  └─ total_vesting_shares: 0 VESTS
├─ VESTS 가격: 1 STEEM = 1,000 VESTS (기본값)
└─ 사용자 잔액: 1000 STEEM

Processing:
├─ 1. VESTS 계산: 1000 STEEM × 1000 = 1,000,000 VESTS
├─ 2. 사용자 계정에 1,000,000 VESTS 추가
├─ 3. total_vesting_fund_steem += 1000 STEEM
└─ 4. total_vesting_shares += 1,000,000 VESTS

After:
├─ 베스팅 풀
│  ├─ total_vesting_fund_steem: 1,000 STEEM
│  └─ total_vesting_shares: 1,000,000 VESTS
├─ VESTS 가격: 1000 / 1,000,000 = 1 STEEM per 1000 VESTS (동일)
└─ 사용자 잔액: 1,000,000 VESTS (Steem Power)
```

---

## 4. 파워다운 (Power Down)

### 4.1 파워다운 메커니즘

Steem에서 파워다운은 **13주에 걸쳐 균등 분할**로 진행됩니다.

**상수 정의:**
```cpp
#define STEEM_VESTING_WITHDRAW_INTERVALS      13
#define STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS (60*60*24*7)  // 1주
```

### 4.2 파워다운 실행 코드

**파일:** [src/core/chain/database.cpp:1482-1510](../../src/core/chain/database.cpp#L1482-L1510)

```cpp
void database::process_vesting_withdrawals()
{
   // 파워다운 중인 계정 순회
   // ...

   // VESTS를 STEEM으로 변환
   auto converted_steem = asset( to_convert, VESTS_SYMBOL ) * cprops.get_vesting_share_price();

   // 사용자 계정 업데이트
   modify( from_account, [&]( account_object& a )
   {
      a.vesting_shares.amount -= to_withdraw;   // VESTS 차감
      a.balance += converted_steem;             // STEEM 지급
      a.withdrawn += to_withdraw;

      if( a.withdrawn >= a.to_withdraw || a.vesting_shares.amount == 0 )
      {
         // 파워다운 완료
         a.vesting_withdraw_rate.amount = 0;
         a.next_vesting_withdrawal = fc::time_point_sec::maximum();
      }
      else
      {
         // 다음 주 예약
         a.next_vesting_withdrawal += fc::seconds( STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS );
      }
   });

   // 글로벌 베스팅 풀 업데이트
   modify( cprops, [&]( dynamic_global_property_object& o )
   {
      o.total_vesting_fund_steem -= converted_steem;  // STEEM 제거
      o.total_vesting_shares.amount -= to_convert;    // VESTS 소각
   });
}
```

### 4.3 파워다운 예제

**시나리오: 사용자가 1,000,000 VESTS (전체) 파워다운**

```
Before:
├─ 베스팅 풀
│  ├─ total_vesting_fund_steem: 1,150 STEEM
│  └─ total_vesting_shares: 1,000,000 VESTS
├─ VESTS 가격: 1.15 STEEM per 1000 VESTS
└─ 사용자 보유: 1,000,000 VESTS

파워다운 설정:
├─ 총 인출: 1,000,000 VESTS
├─ 13주 분할: 1,000,000 / 13 ≈ 76,923 VESTS per week
└─ 매주 인출: 76,923 VESTS × 1.15 / 1000 ≈ 88.46 STEEM

1주차 인출:
├─ VESTS 소각: 76,923 VESTS
├─ STEEM 지급: 88.46 STEEM
├─ total_vesting_fund_steem: 1,150 - 88.46 = 1,061.54 STEEM
├─ total_vesting_shares: 1,000,000 - 76,923 = 923,077 VESTS
└─ 새 VESTS 가격: 1,061.54 / 923,077 ≈ 1.15 STEEM per 1000 VESTS

... (13주 반복)

After (13주 후):
├─ 베스팅 풀
│  ├─ total_vesting_fund_steem: 0 STEEM
│  └─ total_vesting_shares: 0 VESTS
└─ 사용자 잔액: 1,150 STEEM (원금 + 15% 수익)
```

---

## 5. 인플레이션을 통한 자동 이자

### 5.1 블록 인플레이션 분배

**파일:** [src/core/chain/database.cpp:1830-1854](../../src/core/chain/database.cpp#L1830-L1854)

```cpp
void database::process_funds()
{
   // 1. 현재 인플레이션율 계산
   int64_t current_inflation_rate = std::max(
      start_inflation_rate - inflation_rate_adjustment,
      inflation_rate_floor
   );

   // 2. 블록당 새로 생성되는 STEEM
   auto new_steem = ( props.virtual_supply.amount * current_inflation_rate ) /
                    ( int64_t( STEEM_100_PERCENT ) * int64_t( STEEM_BLOCKS_PER_YEAR ) );

   // 3. 보상 분배 비율
   auto content_reward = ( new_steem * STEEM_CONTENT_REWARD_PERCENT ) / STEEM_100_PERCENT;  // 75%
   auto vesting_reward = ( new_steem * STEEM_VESTING_FUND_PERCENT ) / STEEM_100_PERCENT;   // 15%
   auto witness_reward = new_steem - content_reward - vesting_reward;                       // 10%

   // 4. 베스팅 풀에 15% 추가 (핵심!)
   modify( props, [&]( dynamic_global_property_object& p )
   {
      p.total_vesting_fund_steem += asset( vesting_reward, STEEM_SYMBOL );  // STEEM만 증가
      p.current_supply           += asset( new_steem, STEEM_SYMBOL );
      p.virtual_supply           += asset( new_steem, STEEM_SYMBOL );
   });
   // 주목: total_vesting_shares는 증가하지 않음!
}
```

### 5.2 인플레이션 상수

**파일:** [src/core/protocol/include/steem/protocol/config.hpp](../../src/core/protocol/include/steem/protocol/config.hpp)

| 상수 | 값 | 설명 |
|------|-----|------|
| `STEEM_CONTENT_REWARD_PERCENT` | 75% | 콘텐츠 생성자 보상 |
| `STEEM_VESTING_FUND_PERCENT` | 15% | 베스팅 풀 (SP 보유자) 보상 |
| **증인 보상** | 10% | 나머지 (블록 생성자) |
| `STEEM_INFLATION_RATE_START_PERCENT` | 978 (9.78%) | 초기 인플레이션율 |
| `STEEM_INFLATION_RATE_STOP_PERCENT` | 95 (0.95%) | 최종 인플레이션율 |
| `STEEM_INFLATION_NARROWING_PERIOD` | 250,000 블록 | 0.01%씩 감소 주기 |

### 5.3 자동 이자의 원리

**핵심 메커니즘:**

```
매 블록마다:
1. 인플레이션의 15%가 베스팅 풀에 추가
2. total_vesting_fund_steem 증가 ↑
3. total_vesting_shares는 그대로 →
4. 결과: VESTS 가격 상승 ↑
```

**SP 보유자에게 유리한 이유:**

```
Before:
- 보유 VESTS: 1,000,000 VESTS
- VESTS 가격: 1.00 STEEM per 1000 VESTS
- 실제 가치: 1,000 STEEM

1년 후 (인플레이션 15% 누적):
- 보유 VESTS: 1,000,000 VESTS (변동 없음)
- VESTS 가격: 1.15 STEEM per 1000 VESTS (15% 상승)
- 실제 가치: 1,150 STEEM (15% 증가!)

→ 아무것도 하지 않아도 자동으로 15% 수익
```

---

## 6. 실제 시나리오 예제

### 시나리오 1: 제네시스 이후 첫 파워업

```
블록 1 (제네시스):
├─ total_vesting_fund_steem: 0 STEEM
├─ total_vesting_shares: 0 VESTS
└─ VESTS 기본 가격: 1 STEEM = 1,000 VESTS

블록 2:
├─ 증인(genesis)이 블록 생성 보상 받음
│  ├─ 증인 보상: 100 STEEM (베스팅으로 지급)
│  ├─ VESTS 발행: 100 × 1000 = 100,000 VESTS
│  ├─ total_vesting_fund_steem: 100 STEEM
│  └─ total_vesting_shares: 100,000 VESTS
├─ 인플레이션 15% 베스팅 풀 추가
│  ├─ 베스팅 보상: 15 STEEM
│  ├─ total_vesting_fund_steem: 100 + 15 = 115 STEEM
│  └─ total_vesting_shares: 100,000 VESTS (변동 없음)
└─ 새 VESTS 가격: 115 / 100,000 = 1.15 STEEM per 1000 VESTS

결과:
- genesis 계정 보유: 100,000 VESTS
- 실제 가치: 100,000 × 1.15 / 1000 = 115 STEEM
- 수익: 15 STEEM (15%)
```

### 시나리오 2: 다중 사용자 파워업 및 인플레이션

```
초기 상태:
├─ total_vesting_fund_steem: 115 STEEM
├─ total_vesting_shares: 100,000 VESTS
└─ VESTS 가격: 1.15 STEEM per 1000 VESTS

사용자 A가 1000 STEEM 파워업:
├─ VESTS 계산: 1000 / (1.15 / 1000) ≈ 869,565 VESTS
├─ total_vesting_fund_steem: 115 + 1000 = 1,115 STEEM
├─ total_vesting_shares: 100,000 + 869,565 = 969,565 VESTS
└─ 새 가격: 1,115 / 969,565 ≈ 1.15 STEEM per 1000 VESTS (유지)

1000 블록 후 (인플레이션 누적: 150 STEEM):
├─ total_vesting_fund_steem: 1,115 + 150 = 1,265 STEEM
├─ total_vesting_shares: 969,565 VESTS (변동 없음)
└─ 새 가격: 1,265 / 969,565 ≈ 1.30 STEEM per 1000 VESTS

보유자별 가치:
├─ genesis (100,000 VESTS): 100,000 × 1.30 / 1000 = 130 STEEM
│  └─ 수익: 30 STEEM (30%)
└─ 사용자 A (869,565 VESTS): 869,565 × 1.30 / 1000 ≈ 1,130 STEEM
   └─ 수익: 130 STEEM (13%)
```

### 시나리오 3: 파워다운과 인플레이션 동시 진행

```
초기 상태:
├─ total_vesting_fund_steem: 1,265 STEEM
├─ total_vesting_shares: 969,565 VESTS
├─ VESTS 가격: 1.30 STEEM per 1000 VESTS
└─ 사용자 A: 869,565 VESTS 보유

사용자 A가 전체 파워다운 시작:
├─ 13주 분할: 869,565 / 13 ≈ 66,890 VESTS/주
└─ 매주 인출: 66,890 × 1.30 / 1000 ≈ 86.96 STEEM

1주차:
├─ VESTS 소각: 66,890
├─ STEEM 지급: 86.96 STEEM
├─ total_vesting_fund_steem: 1,265 - 86.96 = 1,178.04 STEEM
├─ total_vesting_shares: 969,565 - 66,890 = 902,675 VESTS
├─ 블록 인플레이션 (7일 × 약 0.5 STEEM/일): +3.5 STEEM
├─ total_vesting_fund_steem: 1,178.04 + 3.5 = 1,181.54 STEEM
└─ 새 가격: 1,181.54 / 902,675 ≈ 1.31 STEEM per 1000 VESTS

13주 후 (완료):
├─ 총 인출 STEEM: 약 1,130 STEEM (인플레이션 고려)
├─ 베스팅 풀
│  ├─ total_vesting_fund_steem: 약 135 STEEM (genesis만 남음)
│  └─ total_vesting_shares: 100,000 VESTS
└─ genesis의 가치는 계속 증가 중
```

---

## 요약

### 핵심 메커니즘

1. **베스팅 풀 초기화**
   - 제네시스: 0 STEEM, 0 VESTS
   - 기본 가격: 1 STEEM = 1,000 VESTS

2. **동적 성장**
   - 파워업: 풀에 STEEM 추가 + VESTS 발행
   - 인플레이션: 풀에만 STEEM 추가 (매 블록 15%)
   - 파워다운: 풀에서 STEEM 제거 + VESTS 소각

3. **자동 이자**
   - SP 보유 = VESTS 보유
   - 매 블록 인플레이션 → VESTS 가격 상승
   - 아무것도 하지 않아도 자동 수익

### 공식 요약

```cpp
// VESTS 가격
VESTS_price = total_vesting_fund_steem / total_vesting_shares

// 파워업 (STEEM → VESTS)
new_vests = steem_amount / VESTS_price

// 파워다운 (VESTS → STEEM)
steem_amount = vests_amount × VESTS_price

// 연간 예상 수익률 (인플레이션 15% 기준)
APR ≈ 15% (베스팅 풀만 증가, VESTS 총량 불변)
```

### 설계 의도

1. **장기 보유 유도**: 13주 파워다운으로 단기 투기 방지
2. **자동 이자**: 인플레이션을 SP 보유자에게 분배
3. **투표권**: SP = 증인/콘텐츠 투표력
4. **네트워크 안정성**: 장기 보유자가 거버넌스에 참여

### 코드 위치 참조

- **베스팅 풀 정의:** [src/core/chain/include/steem/chain/global_property_object.hpp:46-47](../../src/core/chain/include/steem/chain/global_property_object.hpp#L46-L47)
- **VESTS 가격 계산:** [src/core/chain/include/steem/chain/global_property_object.hpp:53-59](../../src/core/chain/include/steem/chain/global_property_object.hpp#L53-L59)
- **파워업 로직:** [src/core/chain/database.cpp:1134-1165](../../src/core/chain/database.cpp#L1134-L1165)
- **파워다운 로직:** [src/core/chain/database.cpp:1482-1510](../../src/core/chain/database.cpp#L1482-L1510)
- **인플레이션 분배:** [src/core/chain/database.cpp:1830-1854](../../src/core/chain/database.cpp#L1830-L1854)
- **상수 정의:** [src/core/protocol/include/steem/protocol/config.hpp](../../src/core/protocol/include/steem/protocol/config.hpp)

이 설계는 Steem 블록체인이 토큰 보유자에게 자동 이자를 제공하면서도 장기 참여를 유도하는 **"Staking as Earning"** 메커니즘을 구현합니다.
