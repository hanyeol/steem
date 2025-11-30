# STEEM 베스팅(Vesting)과 보상(Rewards) 시스템

## 목차
1. [개요](#개요)
2. [베스팅 시스템](#베스팅-시스템)
3. [보상 풀 시스템](#보상-풀-시스템)
4. [보상 계산 메커니즘](#보상-계산-메커니즘)
5. [동적 글로벌 속성](#동적-글로벌-속성)
6. [코드 구현 상세](#코드-구현-상세)
7. [실제 사례](#실제-사례)

---

## 개요

STEEM 블록체인은 "Proof of Brain" 알고리즘을 통해 콘텐츠 생성자와 큐레이터에게 보상을 분배합니다. 이 시스템의 핵심은 **베스팅(Vesting)**과 **보상 풀(Reward Pool)**이며, 이들은 긴밀하게 연결되어 작동합니다.

### 핵심 개념

- **STEEM**: 블록체인의 기본 토큰 (유동성 있음)
- **VESTS (Vesting Shares)**: STEEM Power의 내부 표현 단위
- **STEEM Power**: 베스팅된 STEEM (유동성 없음, 영향력 부여)
- **보상 풀(Reward Pool)**: 콘텐츠 생성자와 큐레이터에게 분배될 STEEM 저장소

---

## 베스팅 시스템

### 1. 베스팅의 목적

베스팅 시스템은 다음 목적을 달성합니다:

1. **장기 참여 유도**: STEEM을 잠금으로써 사용자가 네트워크에 장기적으로 기여하도록 유도
2. **영향력 부여**: 베스팅된 STEEM은 투표력(Voting Power)으로 전환되어 콘텐츠에 대한 영향력 행사
3. **인플레이션 보상**: 베스팅된 STEEM 보유자는 인플레이션을 통해 추가 STEEM 획득

### 2. STEEM과 VESTS의 관계

#### 가격 결정 공식

```
vesting_share_price = total_vesting_shares / total_vesting_fund_steem
```

코드 구현 ([global_property_object.hpp:53-59](src/core/chain/include/steem/chain/global_property_object.hpp#L53-L59)):

```cpp
price get_vesting_share_price() const
{
   if ( total_vesting_fund_steem.amount == 0 || total_vesting_shares.amount == 0 )
      return price ( asset( 1000, STEEM_SYMBOL ), asset( 1000000, VESTS_SYMBOL ) );

   return price( total_vesting_shares, total_vesting_fund_steem );
}
```

#### 초기값

네트워크 초기 상태 ([config.hpp:35,63](src/core/protocol/include/steem/protocol/config.hpp#L35)):

**표시값**:
- `1 STEEM = 1,000 VESTS` (기본 비율)
- `STEEM_INIT_SUPPLY = 0` (제네시스 공급량)
- 시간이 지나면서 인플레이션과 보상으로 인해 이 비율이 변경됩니다

**내부값 (사토시 단위)**:

제네시스 블록 시작 시:
```cpp
// Dynamic Global Properties 초기 상태
total_vesting_fund_steem.amount = 0        // 0.000 STEEM (0 사토시)
total_vesting_shares.amount = 0            // 0.000000 VESTS (0 마이크로베스트)
total_reward_fund_steem.amount = 0         // 0.000 STEEM
total_reward_shares2 = 0                   // 청구권 없음
pending_rewarded_vesting_shares.amount = 0 // 0.000000 VESTS
pending_rewarded_vesting_steem.amount = 0  // 0.000 STEEM

current_supply.amount = 0                  // 0.000 STEEM
virtual_supply.amount = 0                  // 0.000 STEEM
current_sbd_supply.amount = 0              // 0.000 SBD
```

첫 베스팅이 발생하기 전까지는 기본 비율이 적용됩니다 ([global_property_object.hpp:55-56](src/core/chain/include/steem/chain/global_property_object.hpp#L55-L56)):
```cpp
if ( total_vesting_fund_steem.amount == 0 || total_vesting_shares.amount == 0 )
   return price( asset( 1000, STEEM_SYMBOL ), asset( 1000000, VESTS_SYMBOL ) );
   //            ^^^^                         ^^^^^^^^
   //            1.000 STEEM                  1.000000 VESTS
   //            (1,000 사토시)               (1,000,000 마이크로베스트)
```

#### 사토시 단위 표현

STEEM과 VESTS의 정밀도 ([asset_symbol.hpp:13-15](src/core/protocol/include/steem/protocol/asset_symbol.hpp#L13-L15)):
- **STEEM 정밀도**: 3 (= 10³ = 1,000)
  - 1 STEEM = 1,000 사토시 (최소 단위 = 0.001 STEEM)
- **VESTS 정밀도**: 6 (= 10⁶ = 1,000,000)
  - 1 VESTS = 1,000,000 마이크로베스트 (최소 단위 = 0.000001 VESTS)

**기본 비율을 최소 단위로 표현**:
```
1.000 STEEM = 1,000.000000 VESTS
1,000 사토시 = 1,000,000,000 마이크로베스트

비율: 1 사토시 STEEM = 1,000,000 마이크로베스트
      (1 STEEM의 0.001 = 1 VESTS)
```

**실제 내부 저장값**:
```cpp
// 코드에서 사용되는 실제 정수값
asset( 1000, STEEM_SYMBOL )     // = 1.000 STEEM (표시값)
asset( 1000000, VESTS_SYMBOL )  // = 1.000000 VESTS (표시값)

// 초기 비율 설정 (global_property_object.hpp:56)
price( asset( 1000, STEEM_SYMBOL ), asset( 1000000, VESTS_SYMBOL ) )
// = 1,000 / 1,000,000 = 0.001 STEEM/VESTS
// 역수 = 1,000 VESTS/STEEM
```

#### 동적 변화

베스팅 가격은 다음 요인에 의해 변경됩니다:
- 새로운 STEEM이 베스팅될 때
- 보상이 STEEM Power로 지급될 때
- 파워다운(Power Down)으로 VESTS가 STEEM으로 전환될 때

### 3. STEEM Power 생성 프로세스

사용자가 STEEM을 STEEM Power로 전환할 때 ([database.cpp:1111-1130](src/core/chain/database.cpp#L1111-L1130)):

```cpp
asset database::create_vesting( const account_object& to_account, asset liquid, bool to_reward_balance )
{
   auto calculate_new_vesting = [ liquid ] ( price vesting_share_price ) -> asset
   {
      /**
       *  total_vesting_shares / total_vesting_fund_steem 비율은
       *  사용자가 자금을 추가한 결과로 변경되어서는 안 됩니다
       *
       *  V / C  = (V+Vn) / (C+Cn)
       *
       *  단순화하면 Vn = (V * Cn ) / C
       *
       *  Cn이 liquid.amount와 같다면, 사용자가 받아야 할
       *  새로운 베스팅 셰어(Vn)를 계산해야 합니다.
       *
       *  64비트 숫자의 곱셈으로 인해 128비트 연산이 필요합니다.
       */
      asset new_vesting = liquid * ( vesting_share_price );
      return new_vesting;
   }
}
```

#### 계산 예시

현재 상태:
- `total_vesting_fund_steem = 100,000 STEEM`
- `total_vesting_shares = 100,000,000 VESTS`
- 비율: `1 STEEM = 1,000 VESTS`

사용자가 100 STEEM을 파워업(Power Up)하면:
- 새로운 VESTS = `100 STEEM * 1,000 = 100,000 VESTS`
- 업데이트된 `total_vesting_fund_steem = 100,100 STEEM`
- 업데이트된 `total_vesting_shares = 100,100,000 VESTS`
- 비율은 `1 STEEM = 1,000 VESTS`로 유지됨

### 4. 보상 베스팅 가격

미래의 보상을 고려한 가격 계산 ([global_property_object.hpp:61-65](src/core/chain/include/steem/chain/global_property_object.hpp#L61-L65)):

```cpp
price get_reward_vesting_share_price() const
{
   return price( total_vesting_shares + pending_rewarded_vesting_shares,
      total_vesting_fund_steem + pending_rewarded_vesting_steem );
}
```

이 가격은 아직 청구되지 않은 대기 중인 보상을 포함하여 계산됩니다.

### 5. 파워다운 (Power Down)

STEEM Power를 다시 유동성 STEEM으로 전환하는 과정:

- **기간**: 13주 (STEEM_VESTING_WITHDRAW_INTERVALS = 13)
- **간격**: 1주마다 (STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS = 604,800초)
- **방식**: 매주 1/13씩 STEEM으로 전환
- **취소 가능**: 파워다운 중에도 언제든 취소 가능

구성 상수 ([config.hpp:92-93](src/core/protocol/include/steem/protocol/config.hpp#L92-L93)):

```cpp
#define STEEM_VESTING_WITHDRAW_INTERVALS      13
#define STEEM_VESTING_WITHDRAW_INTERVAL_SECONDS (60*60*24*7) // 1 week per interval
```

---

## 보상 풀 시스템

### 1. 보상 풀의 구조

STEEM의 보상 시스템은 `reward_fund_object`를 통해 관리됩니다 ([steem_objects.hpp:257-276](src/core/chain/include/steem/chain/steem_objects.hpp#L257-L276)):

```cpp
class reward_fund_object : public object< reward_fund_object_type, reward_fund_object >
{
   reward_fund_id_type     id;
   reward_fund_name_type   name;                    // 풀 이름 (예: "post", "comment")
   asset                   reward_balance;           // 현재 사용 가능한 보상 STEEM
   fc::uint128_t           recent_claims;            // 최근 청구량 (감쇠 적용)
   time_point_sec          last_update;              // 마지막 업데이트 시간
   uint128_t               content_constant;         // 보상 곡선 상수
   uint16_t                percent_curation_rewards; // 큐레이션 보상 비율
   uint16_t                percent_content_rewards;  // 콘텐츠 보상 비율
   protocol::curve_id      author_reward_curve;      // 보상 곡선 타입
   protocol::curve_id      curation_reward_curve;    // 큐레이션 곡선 타입
};
```

### 2. 보상 풀의 종류

STEEM은 여러 보상 풀을 가질 수 있으며, 주요 풀은:

- **Post 보상 풀** (`STEEM_POST_REWARD_FUND_NAME = "post"`)
  - 메인 포스트에 대한 보상
- **Comment 보상 풀** (`STEEM_COMMENT_REWARD_FUND_NAME = "comment"`)
  - 댓글에 대한 보상

### 3. 보상 풀의 충전

보상 풀은 인플레이션을 통해 충전됩니다:

- **콘텐츠 보상 비율**: 인플레이션의 75% (STEEM_CONTENT_REWARD_PERCENT = 7,500)
- **베스팅 펀드 비율**: 인플레이션의 15% (STEEM_VESTING_FUND_PERCENT = 1,500)

구성 상수 ([config.hpp:117-118](src/core/protocol/include/steem/protocol/config.hpp#L117-L118)):

```cpp
#define STEEM_CONTENT_REWARD_PERCENT          (75*STEEM_1_PERCENT) // 75% of inflation
#define STEEM_VESTING_FUND_PERCENT            (15*STEEM_1_PERCENT) // 15% of inflation
```

### 4. Recent Claims와 감쇠(Decay)

`recent_claims`는 최근 보상 청구량을 추적하며, 시간이 지나면서 감쇠됩니다.

감쇠 기간: 15일 (STEEM_RECENT_RSHARES_DECAY_TIME)

코드 구현 ([database.cpp:1715-1734](src/core/chain/database.cpp#L1715-L1734)):

```cpp
void database::process_comment_cashout()
{
   // 각 보상 펀드의 recent rshares 감쇠
   for( auto itr = reward_idx.begin(); itr != reward_idx.end(); ++itr )
   {
      modify( *itr, [&]( reward_fund_object& rfo )
      {
         fc::microseconds decay_time = STEEM_RECENT_RSHARES_DECAY_TIME;

         // 지수 감쇠 공식
         rfo.recent_claims -= ( rfo.recent_claims * ( head_block_time() - rfo.last_update ).to_seconds() )
                              / decay_time.to_seconds();
         rfo.last_update = head_block_time();
      });
   }
}
```

#### 감쇠 공식

```
new_claims = old_claims - (old_claims * elapsed_time / decay_time)
```

이는 지수 감쇠를 선형 근사한 것입니다.

---

## 보상 계산 메커니즘

### 1. Reward Shares (rshares)

투표는 `rshares`(reward shares)로 변환됩니다:

- 투표력(Voting Power)
- STEEM Power 양
- 투표 강도 (1% ~ 100%)

### 2. 보상 곡선 (Reward Curve)

`rshares`는 보상 곡선을 통해 "청구 가능한 보상 셰어"로 변환됩니다.

#### 곡선 타입

STEEM은 4가지 보상 곡선을 지원합니다 ([reward.cpp:68-95](src/core/chain/util/reward.cpp#L68-L95)):

```cpp
uint128_t evaluate_reward_curve( const uint128_t& rshares,
                                  const protocol::curve_id& curve,
                                  const uint128_t& content_constant )
{
   uint128_t result = 0;

   switch( curve )
   {
      case protocol::quadratic:
         {
            // 2차 곡선: (rshares + s)^2 - s^2
            uint128_t rshares_plus_s = rshares + content_constant;
            result = rshares_plus_s * rshares_plus_s - content_constant * content_constant;
         }
         break;

      case protocol::quadratic_curation:
         {
            // 큐레이션 2차 곡선: rshares^2 / (2α + rshares)
            uint128_t two_alpha = content_constant * 2;
            result = uint128_t( rshares.lo, 0 ) / ( two_alpha + rshares );
         }
         break;

      case protocol::linear:
         // 선형: 입력 그대로
         result = rshares;
         break;

      case protocol::square_root:
         // 제곱근: sqrt(rshares)
         result = approx_sqrt( rshares );
         break;
   }

   return result;
}
```

#### 1) Quadratic (2차 곡선)

```
result = (rshares + s)^2 - s^2
```

여기서 `s = STEEM_CONTENT_CONSTANT = 2,000,000,000,000`

**특징**:
- 초기 rshares에서는 완만하게 증가
- rshares가 많아질수록 보상이 기하급수적으로 증가
- 고품질 콘텐츠에 더 많은 보상 집중

**예시**:
- rshares = 1,000,000일 때
- result = (1,000,000 + 2,000,000,000,000)^2 - (2,000,000,000,000)^2
- ≈ 4,000,000,000,000,000,000

#### 2) Linear (선형)

```
result = rshares
```

**특징**:
- 가장 단순한 형태
- rshares와 보상이 비례 관계
- 모든 콘텐츠에 공평한 분배

#### 3) Square Root (제곱근)

```
result = sqrt(rshares)
```

**특징**:
- rshares가 증가할수록 보상 증가율 감소
- 소수 고래의 영향력 제한
- 더 평등한 보상 분배

**예시**:
- rshares = 1,000,000일 때
- result = sqrt(1,000,000) = 1,000

#### 4) Quadratic Curation (큐레이션 2차)

```
result = rshares^2 / (2α + rshares)
```

여기서 `α = content_constant`

**특징**:
- 큐레이션 보상 전용
- 초기 투표자에게 더 많은 보상

### 3. total_reward_shares2

이 값은 모든 콘텐츠의 보상 청구권을 합산한 것입니다.

#### 업데이트 과정

1. 새로운 투표가 발생하면:
   ```
   curve_rshares = evaluate_reward_curve(rshares)
   total_reward_shares2 += curve_rshares
   ```

2. 콘텐츠가 보상을 받으면:
   ```
   total_reward_shares2 -= curve_rshares
   ```

#### "2"의 의미

변수명의 "2"는 역사적인 이유입니다:
- 초기에는 quadratic curve만 사용 (제곱 = ^2)
- 현재는 여러 곡선 지원하지만 호환성을 위해 이름 유지

### 4. 실제 보상 계산

콘텐츠의 보상이 지급될 때 ([reward.cpp:38-66](src/core/chain/util/reward.cpp#L38-L66)):

```cpp
uint64_t get_rshare_reward( const comment_reward_context& ctx )
{
   FC_ASSERT( ctx.rshares > 0 );
   FC_ASSERT( ctx.total_reward_shares2 > 0 );

   // 256비트 정밀도로 계산
   u256 rf(ctx.total_reward_fund_steem.amount.value);
   u256 total_claims = to256( ctx.total_reward_shares2 );

   // 이 콘텐츠의 청구권 계산
   u256 claim = to256( evaluate_reward_curve( ctx.rshares.value,
                                               ctx.reward_curve,
                                               ctx.content_constant ) );

   // 보상 가중치 적용 (예: 50% 지급 시 5000/10000)
   claim = ( claim * ctx.reward_weight ) / STEEM_100_PERCENT;

   // 비례 배분: (reward_fund * claim) / total_claims
   u256 payout_u256 = ( rf * claim ) / total_claims;

   uint64_t payout = static_cast< uint64_t >( payout_u256 );

   // 먼지 청소: 최소 지급액 미만이면 0
   if( is_comment_payout_dust( ctx.current_steem_price, payout ) )
      payout = 0;

   // 최대 SBD 한도 적용
   asset max_steem = to_steem( ctx.current_steem_price, ctx.max_sbd );
   payout = std::min( payout, uint64_t( max_steem.amount.value ) );

   return payout;
}
```

#### 계산 공식

```
payout = (total_reward_fund_steem * curve_rshares * reward_weight) / (total_reward_shares2 * STEEM_100_PERCENT)
```

#### 예시 계산

현재 상태:
- `total_reward_fund_steem = 1,000,000 STEEM`
- `total_reward_shares2 = 100,000,000,000,000`
- 콘텐츠의 `curve_rshares = 1,000,000,000,000`
- `reward_weight = 10,000` (100%)

계산:
```
payout = (1,000,000 * 1,000,000,000,000 * 10,000) / (100,000,000,000,000 * 10,000)
       = 10,000 STEEM
```

### 5. 저자와 큐레이터 분배

총 보상이 계산되면 저자와 큐레이터에게 분배됩니다:

- **큐레이터**: 보상의 25% (일반적으로)
- **저자**: 보상의 75%

저자 보상은 다시 분할됩니다:
- **STEEM Power**: 50%
- **SBD**: 50% (또는 STEEM, SBD 인쇄율에 따라)

### 6. 역경매 (Reverse Auction)

초기 30분 동안의 투표는 큐레이터 보상이 일부 저자에게 돌아갑니다:

- **기간**: 30분 (STEEM_REVERSE_AUCTION_WINDOW_SECONDS = 1,800초)
- **메커니즘**: 일찍 투표할수록 더 많은 보상이 저자에게 돌아감

구성 상수 ([config.hpp:99](src/core/protocol/include/steem/protocol/config.hpp#L99)):

```cpp
#define STEEM_REVERSE_AUCTION_WINDOW_SECONDS  (60*30) // 30 minutes
```

---

## 동적 글로벌 속성

### Dynamic Global Property Object

모든 베스팅과 보상 관련 상태는 `dynamic_global_property_object`에서 관리됩니다.

#### 베스팅 관련 속성

| 속성 | 타입 | 설명 | 초기값 |
|------|------|------|--------|
| `total_vesting_fund_steem` | `asset` | 전체 베스팅된 STEEM의 총량 | 0 STEEM |
| `total_vesting_shares` | `asset` | 존재하는 모든 VESTS의 총량 | 0 VESTS |

**의미**:
- 이 두 값의 비율이 STEEM ↔ VESTS 환율을 결정
- 모든 베스팅 작업은 이 두 값을 동시에 업데이트하여 비율 유지

#### 보상 풀 관련 속성

| 속성 | 타입 | 설명 | 초기값 |
|------|------|------|--------|
| `total_reward_fund_steem` | `asset` | 보상 풀에 있는 총 STEEM | 0 STEEM |
| `total_reward_shares2` | `fc::uint128` | 모든 청구 가능한 보상 셰어의 합 | 0 |

**의미**:
- `total_reward_fund_steem`: 현재 분배 가능한 보상의 총량
- `total_reward_shares2`: 모든 콘텐츠의 보상 청구권을 합산
- 실제 보상 = `(reward_fund * 내 청구권) / total_reward_shares2`

#### 대기 중인 보상 관련 속성

| 속성 | 타입 | 설명 | 초기값 |
|------|------|------|--------|
| `pending_rewarded_vesting_shares` | `asset` | 아직 지급되지 않은 VESTS | 0 VESTS |
| `pending_rewarded_vesting_steem` | `asset` | 대기 중인 VESTS를 뒷받침하는 STEEM | 0 STEEM |

**의미**:
- 보상이 청구될 때까지 대기 중인 VESTS
- 미래 환율 계산에 사용 (`get_reward_vesting_share_price()`)
- 실제 청구 시 `total_vesting_*`로 이동

### 업데이트 시나리오

#### 시나리오 1: 사용자가 STEEM을 파워업

**작업 전**:
```
total_vesting_fund_steem = 100,000 STEEM
total_vesting_shares = 100,000,000 VESTS
```

**사용자 작업**: 1,000 STEEM 파워업

**작업 후**:
```
total_vesting_fund_steem = 101,000 STEEM
total_vesting_shares = 101,000,000 VESTS
사용자 계정: +1,000,000 VESTS
```

#### 시나리오 2: 콘텐츠에 투표

**작업 전**:
```
total_reward_shares2 = 50,000,000,000,000
```

**투표 작업**: rshares = 1,000,000

**작업 후**:
```
curve_rshares = evaluate_reward_curve(1,000,000) = 1,000,000,000 (가정)
total_reward_shares2 = 50,001,000,000,000
```

#### 시나리오 3: 콘텐츠 보상 지급

**작업 전**:
```
total_reward_fund_steem = 1,000,000 STEEM
total_reward_shares2 = 100,000,000,000,000
total_vesting_fund_steem = 100,000 STEEM
total_vesting_shares = 100,000,000 VESTS
pending_rewarded_vesting_shares = 0 VESTS
pending_rewarded_vesting_steem = 0 STEEM
```

**보상 지급**: 콘텐츠 보상 100 STEEM (50 STEEM은 STEEM Power로, 50 STEEM은 SBD로)

**작업 중**:
```
1. 보상 풀에서 차감
   total_reward_fund_steem = 999,900 STEEM

2. 청구권 차감
   total_reward_shares2 -= 해당 콘텐츠의 curve_rshares

3. 50 STEEM을 VESTS로 전환하여 대기 큐에 추가
   pending_rewarded_vesting_shares = 50,000 VESTS
   pending_rewarded_vesting_steem = 50 STEEM
```

**작업 후** (사용자가 청구):
```
total_vesting_fund_steem = 100,050 STEEM
total_vesting_shares = 100,050,000 VESTS
pending_rewarded_vesting_shares = 0 VESTS
pending_rewarded_vesting_steem = 0 STEEM
사용자 계정: +50,000 VESTS
```

---

## 코드 구현 상세

### 주요 파일 위치

#### 1. 데이터 구조

- **[global_property_object.hpp](src/core/chain/include/steem/chain/global_property_object.hpp)**
  - `dynamic_global_property_object` 정의
  - 베스팅 가격 계산 함수

- **[steem_objects.hpp](src/core/chain/include/steem/chain/steem_objects.hpp)**
  - `reward_fund_object` 정의
  - 보상 풀 관련 객체

#### 2. 보상 계산

- **[util/reward.hpp](src/core/chain/include/steem/chain/util/reward.hpp)**
  - `comment_reward_context` 구조체
  - 보상 계산 함수 선언

- **[util/reward.cpp](src/core/chain/util/reward.cpp)**
  - `evaluate_reward_curve()` 구현
  - `get_rshare_reward()` 구현
  - 제곱근 근사 함수

#### 3. 데이터베이스 작업

- **[database.cpp](src/core/chain/database.cpp)**
  - `create_vesting()` - VESTS 생성
  - `process_comment_cashout()` - 보상 지급 처리
  - `adjust_total_payout()` - 지급 기록 업데이트
  - `pay_curators()` - 큐레이터 보상 분배

#### 4. 구성 상수

- **[protocol/config.hpp](src/core/protocol/include/steem/protocol/config.hpp)**
  - 모든 시스템 상수 정의
  - 인플레이션, 보상, 타이밍 관련 설정

### 핵심 함수 흐름

#### 베스팅 생성 흐름

```
transfer_to_vesting_operation
    ↓
transfer_to_vesting_evaluator::do_apply()
    ↓
database::create_vesting()
    ↓
[가격 계산]
    ↓
[global properties 업데이트]
    ↓
[사용자 계정 VESTS 증가]
```

#### 보상 지급 흐름

```
process_comment_cashout()
    ↓
[보상 풀 감쇠 처리]
    ↓
[캐시아웃 대상 콘텐츠 찾기]
    ↓
get_rshare_reward()
    ↓
[큐레이터 보상 계산]
pay_curators()
    ↓
[저자 보상 계산]
    ↓
[STEEM Power 변환]
create_vesting()
    ↓
adjust_total_payout()
```

### 중요한 검증 로직

#### 1. 오버플로우 방지

보상 계산 시 256비트 정수 사용:

```cpp
u256 rf(ctx.total_reward_fund_steem.amount.value);
u256 total_claims = to256( ctx.total_reward_shares2 );
u256 claim = to256( evaluate_reward_curve(...) );
u256 payout_u256 = ( rf * claim ) / total_claims;
```

#### 2. 최소 지급액 검증

먼지(dust) 지급을 방지:

```cpp
#define STEEM_MIN_PAYOUT_SBD (asset(20,SBD_SYMBOL))

inline bool is_comment_payout_dust( const price& p, uint64_t steem_payout )
{
   return to_sbd( p, asset( steem_payout, STEEM_SYMBOL ) ) < STEEM_MIN_PAYOUT_SBD;
}
```

SBD 가치로 0.020 SBD 미만이면 지급하지 않음

#### 3. 비율 일관성 유지

베스팅 생성/파괴 시 항상 비율 유지:

```cpp
// 생성 시
Vn = (V * Cn) / C

// 파괴 시
Cn = (C * Vn) / V
```

---

## 실제 사례

### 사례 1: 인기 포스트의 보상 계산

#### 초기 상태
```
total_reward_fund_steem: 800,000 STEEM
total_reward_shares2: 5,000,000,000,000,000
content_constant: 2,000,000,000,000
reward_curve: quadratic
```

#### 포스트 상태
```
총 rshares: 50,000,000,000
투표자 수: 500명
작성 후 경과: 7일 (캐시아웃 시점)
```

#### 계산 과정

**1단계: Curve rshares 계산**

```
curve_rshares = (50,000,000,000 + 2,000,000,000,000)^2 - (2,000,000,000,000)^2
              ≈ 204,100,000,000,000,000
```

**2단계: 보상 비례 배분**

```
claim = 204,100,000,000,000,000
total_claims = 5,000,000,000,000,000

payout = (800,000 * 204,100,000,000,000,000) / 5,000,000,000,000,000
       = 32,656 STEEM
```

**3단계: 큐레이터/저자 분배**

```
큐레이터 보상: 32,656 * 0.25 = 8,164 STEEM
저자 보상: 32,656 * 0.75 = 24,492 STEEM
```

**4단계: 저자 보상 분할**

```
STEEM Power: 24,492 * 0.50 = 12,246 STEEM → 약 12,246,000 VESTS
SBD: 24,492 * 0.50 = 12,246 SBD
```

**5단계: Global properties 업데이트**

```
total_reward_fund_steem: 800,000 - 32,656 = 767,344 STEEM
total_reward_shares2: 5,000,000,000,000,000 - 204,100,000,000,000,000
                    = 4,795,900,000,000,000
pending_rewarded_vesting_steem: +12,246 STEEM
pending_rewarded_vesting_shares: +12,246,000 VESTS
```

### 사례 2: 소규모 댓글의 보상

#### 초기 상태
```
total_reward_fund_steem: 800,000 STEEM
total_reward_shares2: 5,000,000,000,000,000
content_constant: 2,000,000,000,000
reward_curve: linear (댓글은 선형 곡선 사용 가정)
```

#### 댓글 상태
```
총 rshares: 100,000
투표자 수: 5명
```

#### 계산 과정

**1단계: Curve rshares 계산 (선형)**

```
curve_rshares = 100,000 (선형이므로 그대로)
```

**2단계: 보상 비례 배분**

```
payout = (800,000 * 100,000) / 5,000,000,000,000,000
       = 0.016 STEEM
```

**3단계: 먼지 검증**

```
0.016 STEEM ≈ 0.016 SBD < 0.020 SBD (STEEM_MIN_PAYOUT_SBD)
→ 보상 지급되지 않음 (payout = 0)
```

이 댓글은 최소 지급액에 미달하여 보상을 받지 못합니다.

### 사례 3: 대량 파워업

#### 초기 상태
```
total_vesting_fund_steem: 500,000,000 STEEM
total_vesting_shares: 500,000,000,000 VESTS
비율: 1 STEEM = 1,000 VESTS
```

#### 고래 작업
```
사용자가 10,000,000 STEEM 파워업
```

#### 계산 과정

**1단계: 새로운 VESTS 계산**

```
vesting_share_price = 500,000,000,000 / 500,000,000 = 1,000 VESTS/STEEM

new_vests = 10,000,000 * 1,000 = 10,000,000,000 VESTS
```

**2단계: Global properties 업데이트**

```
total_vesting_fund_steem: 500,000,000 + 10,000,000 = 510,000,000 STEEM
total_vesting_shares: 500,000,000,000 + 10,000,000,000 = 510,000,000,000 VESTS

새로운 비율: 510,000,000,000 / 510,000,000 = 1,000 VESTS/STEEM (동일 유지)
```

**3단계: 사용자 계정 업데이트**

```
사용자 balance: -10,000,000 STEEM
사용자 vesting_shares: +10,000,000,000 VESTS
```

비율이 정확히 유지되어 기존 보유자들이 불이익을 받지 않습니다.

### 사례 4: 13주 파워다운

#### 초기 상태
```
사용자 VESTS: 13,000,000 VESTS
total_vesting_fund_steem: 510,000,000 STEEM
total_vesting_shares: 510,000,000,000 VESTS
비율: 1 STEEM = 1,000 VESTS
```

#### 파워다운 시작
```
사용자가 전체 VESTS 파워다운 시작
```

#### 계산 과정

**매주 지급량 계산**

```
주당 VESTS = 13,000,000 / 13 = 1,000,000 VESTS
주당 STEEM = 1,000,000 / 1,000 = 1,000 STEEM
```

**1주차 지급 후**

```
total_vesting_fund_steem: 510,000,000 - 1,000 = 509,999,000 STEEM
total_vesting_shares: 510,000,000,000 - 1,000,000 = 509,999,000,000 VESTS
사용자 VESTS: 13,000,000 - 1,000,000 = 12,000,000 VESTS
사용자 STEEM: +1,000 STEEM

새로운 비율: 509,999,000,000 / 509,999,000 ≈ 1,000 VESTS/STEEM (근사적으로 유지)
```

**13주 후 (완료)**

```
total_vesting_fund_steem: 510,000,000 - 13,000 = 509,987,000 STEEM
total_vesting_shares: 510,000,000,000 - 13,000,000 = 509,987,000,000 VESTS
사용자 VESTS: 0 VESTS
사용자 STEEM: +13,000 STEEM
```

### 사례 5: 보상 풀 감쇠

#### 초기 상태 (Day 0)
```
recent_claims: 1,000,000,000,000,000
last_update: Day 0 00:00
```

#### Day 1 후 (활동 없음)
```
elapsed_time = 1 day = 86,400초
decay_time = 15 days = 1,296,000초

decay_amount = 1,000,000,000,000,000 * 86,400 / 1,296,000
             = 66,666,666,666,667

new_recent_claims = 1,000,000,000,000,000 - 66,666,666,666,667
                  = 933,333,333,333,333
```

**감쇠율**: 약 6.67% per day

#### Day 15 후 (활동 없음)
```
이론적으로는 e^(-1) ≈ 36.8% 남음
실제 선형 근사로는 0에 가까워짐
```

---

## 요약

### 핵심 개념 정리

1. **베스팅 시스템**
   - STEEM ↔ VESTS 환율은 동적으로 변화
   - 비율은 모든 작업에서 일관성 있게 유지
   - 13주 파워다운으로 유동성 확보

2. **보상 풀**
   - 인플레이션으로 충전
   - Recent claims는 15일 주기로 감쇠
   - 비례 배분으로 공정한 분배

3. **보상 곡선**
   - Quadratic: 고품질 콘텐츠에 집중
   - Linear: 평등한 분배
   - Square Root: 고래 영향력 제한
   - 선택은 하드포크와 커뮤니티 합의로 결정

4. **동적 글로벌 속성**
   - 모든 베스팅/보상 상태의 중앙 저장소
   - 실시간으로 업데이트
   - API를 통해 조회 가능

### 설계 철학

STEEM의 베스팅과 보상 시스템은 다음 원칙을 따릅니다:

1. **공정성**: 비례 배분으로 모든 참여자에게 공정한 기회
2. **투명성**: 모든 계산이 체인에서 검증 가능
3. **장기 인센티브**: 베스팅으로 장기 참여 유도
4. **유연성**: 다양한 보상 곡선으로 커뮤니티 요구 반영
5. **지속가능성**: 인플레이션 감소로 장기적 가치 유지

### 추가 참고 자료

- [dynamic-global-properties.md](dynamic-global-properties.md) - 동적 글로벌 속성 전체 목록
- [system-architecture.md](system-architecture.md) - 전체 시스템 아키텍처
- [reputation-system.md](reputation-system.md) - 평판 시스템
- [docs/development/create-operation.md](../development/create-operation.md) - 새로운 작업 추가 방법

---

**문서 버전**: 1.0
**마지막 업데이트**: 2025-11-21
**관련 코드 버전**: STEEM 0.0.0
