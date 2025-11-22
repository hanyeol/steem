# Curation Rewards

## 개요

큐레이션 보상(Curation Rewards)은 Steem 블록체인에서 콘텐츠에 투표한 사용자들에게 분배되는 보상입니다. 이 시스템은 양질의 콘텐츠를 조기에 발견하고 투표하는 사용자들에게 인센티브를 제공하여, 커뮤니티가 가치 있는 콘텐츠를 효과적으로 큐레이팅하도록 유도합니다.

## 주요 구성 요소

### 1. 보상 분배 비율

콘텐츠가 지급 시점(cashout)에 도달하면, 총 보상이 작성자(author)와 큐레이터(curator)에게 분배됩니다:

```
총 보상 = 작성자 보상 + 큐레이션 보상
```

큐레이션 보상 비율은 `reward_fund_object`의 `percent_curation_rewards` 필드에 저장되며, 일반적으로 총 보상의 일정 비율(예: 25%)로 설정됩니다.

**코드 참조**: [database.cpp:1671](../libraries/chain/database.cpp#L1671)

```cpp
share_type curation_tokens = ( ( reward_tokens * get_curation_rewards_percent( comment ) ) / STEEM_100_PERCENT ).to_uint64();
share_type author_tokens = reward_tokens.to_uint64() - curation_tokens;
```

### 2. 큐레이션 가중치 계산

각 투표의 큐레이션 가중치는 다음 요소들에 의해 결정됩니다:

#### 2.1 기본 가중치

투표의 기본 가중치는 reward curve를 사용하여 계산됩니다:

```
W(R_N) - W(R_N-1)
```

여기서:
- `W(R)` = 보상 커브 함수
- `R_N` = 현재 투표까지의 누적 vote_rshares
- `R_N-1` = 이전까지의 누적 vote_rshares

**코드 참조**: [steem_evaluator.cpp:1304-1306](../libraries/chain/steem_evaluator.cpp#L1304-L1306)

```cpp
uint64_t old_weight = util::evaluate_reward_curve( old_vote_rshares.value, curve, reward_fund.content_constant ).to_uint64();
uint64_t new_weight = util::evaluate_reward_curve( comment.vote_rshares.value, curve, reward_fund.content_constant ).to_uint64();
cv.weight = new_weight - old_weight;
```

#### 2.2 역경매 메커니즘 (Reverse Auction)

콘텐츠가 생성된 후 초기 30분 동안은 역경매 창(Reverse Auction Window)이 적용됩니다. 이 기간 동안의 투표는 시간에 비례하여 가중치가 감소합니다:

```
조정된 가중치 = 기본 가중치 × (경과 시간 / 30분)
```

**상수 정의**: [config.hpp:105](../libraries/protocol/include/steem/protocol/config.hpp#L105)
```cpp
#define STEEM_REVERSE_AUCTION_WINDOW_SECONDS  (60*30) /// 30 minutes
```

**코드 참조**: [steem_evaluator.cpp:1310-1316](../libraries/chain/steem_evaluator.cpp#L1310-L1316)

```cpp
uint128_t w(max_vote_weight);
uint64_t delta_t = std::min( uint64_t((cv.last_update - comment.created).to_seconds()),
                              uint64_t(STEEM_REVERSE_AUCTION_WINDOW_SECONDS) );

w *= delta_t;
w /= STEEM_REVERSE_AUCTION_WINDOW_SECONDS;
cv.weight = w.to_uint64();
```

#### 예시

- **0분차 투표**: 가중치의 0% 적용 (모든 보상이 작성자에게 반환)
- **15분차 투표**: 가중치의 50% 적용
- **30분차 이후 투표**: 가중치의 100% 적용

역경매로 인해 감소된 큐레이션 보상은 작성자에게 돌아갑니다.

### 3. 보상 곡선 (Reward Curves)

큐레이션 가중치 계산에는 다양한 보상 곡선이 사용될 수 있습니다:

#### 3.1 Quadratic Curation Curve

```
W(R) = R² / (2α + R)
```

여기서 `α` (alpha)는 `content_constant`입니다.

**코드 참조**: [reward.cpp:80-84](../libraries/chain/util/reward.cpp#L80-L84)

```cpp
case protocol::quadratic_curation:
{
   uint128_t two_alpha = content_constant * 2;
   result = uint128_t( rshares.lo, 0 ) / ( two_alpha + rshares );
}
break;
```

#### 3.2 기타 커브

- **Linear**: `W(R) = R`
- **Quadratic**: `W(R) = (R + S)² - S²` (작성자 보상에 주로 사용)
- **Square Root**: `W(R) = √R`

**전체 구현**: [reward.cpp:68-95](../libraries/chain/util/reward.cpp#L68-L95)

### 4. 개별 큐레이터 보상 분배

각 큐레이터는 자신의 가중치에 비례하여 총 큐레이션 보상을 받습니다:

```
큐레이터 보상 = (큐레이터 가중치 / 총 가중치) × 총 큐레이션 보상
```

**코드 참조**: [database.cpp:1618-1628](../libraries/chain/database.cpp#L1618-L1628)

```cpp
for( auto& item : proxy_set )
{
   uint128_t weight( item->weight );
   auto claim = ( ( max_rewards.value * weight ) / total_weight ).to_uint64();
   if( claim > 0 )
   {
      unclaimed_rewards -= claim;
      const auto& voter = get( item->voter );
      auto reward = create_vesting( voter, asset( claim, STEEM_SYMBOL ), true );

      push_virtual_operation( curation_reward_operation( voter.name, reward, c.author, to_string( c.permlink ) ) );
      // ...
   }
}
```

## 투표 처리 프로세스

### 1. Rshares 계산

투표 시 사용자의 voting power와 vesting shares를 기반으로 rshares가 계산됩니다:

**코드 참조**: [steem_evaluator.cpp:1190-1210](../libraries/chain/steem_evaluator.cpp#L1190-L1210)

```cpp
int64_t regenerated_power = (STEEM_100_PERCENT * elapsed_seconds) / STEEM_VOTE_REGENERATION_SECONDS;
int64_t current_power = std::min( int64_t(voter.voting_power + regenerated_power), int64_t(STEEM_100_PERCENT) );

int64_t used_power = ((current_power * abs_weight) / STEEM_100_PERCENT) * (60*60*24);
used_power = (used_power + max_vote_denom - 1) / max_vote_denom;

auto effective_vesting = _db.get_effective_vesting_shares( voter, VESTS_SYMBOL ).amount.value;
int64_t abs_rshares = ((uint128_t( effective_vesting ) * used_power) / (STEEM_100_PERCENT)).to_uint64();

abs_rshares -= STEEM_VOTE_DUST_THRESHOLD;
abs_rshares = std::max( int64_t(0), abs_rshares );
```

### 2. Comment 객체 업데이트

투표가 처리되면 `comment_object`가 업데이트됩니다:

**코드 참조**: [steem_evaluator.cpp:1245-1254](../libraries/chain/steem_evaluator.cpp#L1245-L1254)

```cpp
_db.modify( comment, [&]( comment_object& c ){
   c.net_rshares += rshares;
   c.abs_rshares += abs_rshares;
   if( rshares > 0 )
      c.vote_rshares += rshares;
   if( rshares > 0 )
      c.net_votes++;
   else
      c.net_votes--;
});
```

### 3. Comment Vote 객체 생성

각 투표는 `comment_vote_object`로 저장됩니다:

**데이터베이스 객체**: [comment_object.hpp:141-159](../libraries/chain/include/steem/chain/comment_object.hpp#L141-L159)

```cpp
class comment_vote_object : public object< comment_vote_object_type, comment_vote_object>
{
   id_type           id;
   account_id_type   voter;
   comment_id_type   comment;
   uint64_t          weight = 0;        // 큐레이션 가중치
   int64_t           rshares = 0;       // 투표 rshares
   int16_t           vote_percent = 0;  // 투표 비율 (-10000 ~ 10000)
   time_point_sec    last_update;       // 투표 시간
   int8_t            num_changes = 0;   // 투표 변경 횟수
};
```

## 지급 프로세스 (Cashout)

### 1. 지급 시점 결정

콘텐츠는 일정 시간 후 자동으로 지급됩니다. `cashout_time`이 현재 블록 시간에 도달하면 지급이 처리됩니다.

**코드 참조**: [database.cpp:1826-1836](../libraries/chain/database.cpp#L1826-L1836)

```cpp
while( current != cidx.end() && current->cashout_time <= head_block_time() )
{
   auto fund_id = get_reward_fund( *current ).id._id;
   ctx.total_reward_shares2 = funds[ fund_id ].recent_claims;
   ctx.total_reward_fund_steem = funds[ fund_id ].reward_balance;

   bool forward_curation_remainder = false;

   funds[ fund_id ].steem_awarded += cashout_comment_helper( ctx, *current, forward_curation_remainder );

   current = cidx.begin();
}
```

### 2. 보상 계산 및 분배

`cashout_comment_helper` 함수가 총 보상을 계산하고 큐레이터와 작성자에게 분배합니다:

**전체 프로세스**: [database.cpp:1652-1763](../libraries/chain/database.cpp#L1652-L1763)

1. **총 보상 계산**: Rshares를 기반으로 reward fund에서 보상 계산
2. **큐레이션 보상 분리**: 총 보상의 일정 비율을 큐레이션 토큰으로 분리
3. **큐레이터 지급**: `pay_curators` 함수로 각 큐레이터에게 분배
4. **잔여 처리**: 미청구 보상은 작성자에게 반환 (선택적)
5. **작성자 지급**: 수혜자(beneficiaries) 할당 후 남은 금액 지급

### 3. Virtual Operation 생성

지급 시 `curation_reward_operation` virtual operation이 생성됩니다:

**프로토콜 정의**: [steem_virtual_operations.hpp:23-33](../libraries/protocol/include/steem/protocol/steem_virtual_operations.hpp#L23-L33)

```cpp
struct curation_reward_operation : public virtual_operation
{
   account_name_type curator;
   asset             reward;
   account_name_type comment_author;
   string            comment_permlink;
};
```

## 큐레이션 자격 조건

큐레이션 보상을 받으려면 다음 조건을 모두 만족해야 합니다:

**코드 참조**: [steem_evaluator.cpp:1294-1297](../libraries/chain/steem_evaluator.cpp#L1294-L1297)

```cpp
bool curation_reward_eligible = rshares > 0 &&
                                (comment.last_payout == fc::time_point_sec()) &&
                                comment.allow_curation_rewards;

if( curation_reward_eligible )
   curation_reward_eligible = _db.get_curation_rewards_percent( comment ) > 0;
```

1. **양의 rshares**: 업보트만 해당 (다운보트는 제외)
2. **첫 지급**: `last_payout`이 설정되지 않은 경우 (재지급 불가)
3. **큐레이션 허용**: 작성자가 `allow_curation_rewards = true`로 설정
4. **보상 비율 양수**: Reward fund의 큐레이션 비율이 0보다 큼

## 특별한 경우

### 1. 큐레이션 비활성화

작성자는 `comment_options_operation`을 통해 큐레이션 보상을 비활성화할 수 있습니다:

**Comment 객체 필드**: [comment_object.hpp:106](../libraries/chain/include/steem/chain/comment_object.hpp#L106)

```cpp
bool allow_curation_rewards = true;
```

큐레이션이 비활성화되면 모든 보상이 작성자에게 돌아갑니다:

**코드 참조**: [database.cpp:1601-1605](../libraries/chain/database.cpp#L1601-L1605)

```cpp
if( !c.allow_curation_rewards )
{
   unclaimed_rewards = 0;
   max_rewards = 0;
}
```

### 2. 미청구 보상 (Unclaimed Rewards)

투표 가중치가 매우 작아 보상이 satoshi 단위로 0이 되는 경우, 해당 보상은 미청구 보상으로 남습니다:

**코드 참조**: [database.cpp:1620-1624](../libraries/chain/database.cpp#L1620-L1624)

```cpp
auto claim = ( ( max_rewards.value * weight ) / total_weight ).to_uint64();
if( claim > 0 )
{
   unclaimed_rewards -= claim;
   // ...
}
```

미청구 보상의 처리는 `forward_curation_remainder` 플래그에 따라 결정됩니다:
- `true`: 작성자에게 반환
- `false`: reward fund에 남김

### 3. 투표 변경

사용자는 투표를 최대 5회까지 변경할 수 있습니다:

**상수 정의**: [config.hpp:104](../libraries/protocol/include/steem/protocol/config.hpp#L104)
```cpp
#define STEEM_MAX_VOTE_CHANGES  5
```

투표 변경 시 큐레이션 가중치는 재계산되며, 이전 가중치는 0으로 설정됩니다.

### 4. 지급 후 투표

콘텐츠가 첫 지급을 받은 후 투표는 기록되지만 가중치는 0입니다:

**코드 참조**: [steem_evaluator.cpp:1160-1182](../libraries/chain/steem_evaluator.cpp#L1160-L1182)

```cpp
if( _db.calculate_discussion_payout_time( comment ) == fc::time_point_sec::maximum() )
{
   // 지급 완료 후 - 투표 기록만 하고 보상 계산 없음
   _db.create< comment_vote_object >( [&]( comment_vote_object& cvo )
   {
      cvo.voter = voter.id;
      cvo.comment = comment.id;
      cvo.vote_percent = o.weight;
      cvo.last_update = _db.head_block_time();
   });
   return;
}
```

## 통계 및 추적

### Comment 객체 필드

큐레이션 관련 통계는 `comment_object`에 저장됩니다:

**데이터 필드**: [comment_object.hpp:80-94](../libraries/chain/include/steem/chain/comment_object.hpp#L80-L94)

```cpp
share_type        net_rshares;           // 순 rshares (업보트 - 다운보트)
share_type        abs_rshares;           // 절대값 합계
share_type        vote_rshares;          // 양의 rshares만 합계 (큐레이션 가중치 계산용)
uint64_t          total_vote_weight;     // 총 큐레이션 가중치
asset             curator_payout_value;  // 지급된 큐레이션 보상 총액 (SBD 환산)
```

### Account 객체 필드

개별 계정의 큐레이션 보상 통계:

**데이터 필드**: [account_object.hpp](../libraries/chain/include/steem/chain/account_object.hpp)

```cpp
share_type        curation_rewards;  // 누적 큐레이션 보상 (LOW_MEM 모드가 아닐 때)
```

## 구현 참조

### 핵심 파일

1. **투표 처리**: [libraries/chain/steem_evaluator.cpp:1150-1400](../libraries/chain/steem_evaluator.cpp#L1150)
   - `vote_evaluator::do_apply()` - 투표 연산 처리

2. **큐레이터 지급**: [libraries/chain/database.cpp:1582-1643](../libraries/chain/database.cpp#L1582)
   - `database::pay_curators()` - 큐레이터에게 보상 분배

3. **지급 처리**: [libraries/chain/database.cpp:1652-1851](../libraries/chain/database.cpp#L1652)
   - `database::cashout_comment_helper()` - 총 보상 계산 및 분배
   - `database::process_comment_cashout()` - 지급 가능한 모든 콘텐츠 처리

4. **보상 계산**: [libraries/chain/util/reward.cpp](../libraries/chain/util/reward.cpp)
   - `evaluate_reward_curve()` - 보상 곡선 계산
   - `get_rshare_reward()` - Rshares를 보상으로 변환

5. **프로토콜 정의**:
   - [libraries/protocol/include/steem/protocol/steem_virtual_operations.hpp:23-33](../libraries/protocol/include/steem/protocol/steem_virtual_operations.hpp#L23) - `curation_reward_operation`
   - [libraries/chain/include/steem/chain/comment_object.hpp](../libraries/chain/include/steem/chain/comment_object.hpp) - 데이터 구조

## 주요 상수

| 상수 | 값 | 설명 |
|------|-----|------|
| `STEEM_REVERSE_AUCTION_WINDOW_SECONDS` | 1800 (30분) | 역경매 창 기간 |
| `STEEM_MAX_VOTE_CHANGES` | 5 | 최대 투표 변경 횟수 |
| `STEEM_MIN_VOTE_INTERVAL_SEC` | 3 | 최소 투표 간격 (초) |
| `STEEM_VOTE_DUST_THRESHOLD` | 50000000 | 투표 먼지 임계값 |
| `STEEM_VOTE_REGENERATION_SECONDS` | 432000 (5일) | 투표력 완전 재생 시간 |
| `STEEM_100_PERCENT` | 10000 | 100% 표현 (basis points) |

## 수식 요약

### 1. 총 큐레이션 보상

```
큐레이션 토큰 = 총 보상 × 큐레이션 비율 / 10000
```

### 2. 개별 큐레이터 보상

```
개별 보상 = 큐레이션 토큰 × 개별 가중치 / 총 가중치
```

### 3. 역경매 조정 가중치

```
조정 가중치 = 기본 가중치 × min(경과_초, 1800) / 1800
```

### 4. 큐레이션 가중치 (Quadratic Curation Curve)

```
W(R) = R² / (2α + R)
가중치 = W(현재_vote_rshares) - W(이전_vote_rshares)
```

## 예시 시나리오

### 시나리오 1: 정상적인 큐레이션

1. 포스트가 생성됨 (시간 = 0)
2. Alice가 30분 후 투표 → 가중치 100% 적용
3. Bob이 60분 후 투표 → 가중치 100% 적용
4. 7일 후 지급:
   - 총 보상: 10 STEEM
   - 큐레이션 보상 (25%): 2.5 STEEM
   - Alice 가중치: 600, Bob 가중치: 400, 총 가중치: 1000
   - Alice 보상: 2.5 × 600/1000 = 1.5 STEEM
   - Bob 보상: 2.5 × 400/1000 = 1.0 STEEM

### 시나리오 2: 역경매 영향

1. 포스트가 생성됨 (시간 = 0)
2. Charlie가 즉시 투표 → 가중치 0% 적용 (모든 보상이 작성자에게)
3. Dave가 15분 후 투표 → 가중치 50% 적용
4. Eve가 30분 후 투표 → 가중치 100% 적용
5. 7일 후 지급:
   - Charlie 보상: 0 STEEM (역경매로 인해 작성자에게 반환)
   - Dave 보상: 감소된 가중치에 비례
   - Eve 보상: 전체 가중치에 비례

## 관련 문서

- [Vote Operation Evaluation](vote-operation-evaluation.md) - 투표 연산 평가 프로세스
- [System Architecture](system-architecture.md) - 전체 시스템 아키텍처
- [Reward Distribution](../operations/reward-distribution.md) - 보상 분배 메커니즘

## 참고사항

1. 큐레이션 보상은 항상 **Vesting Shares (VESTS)** 형태로 지급됩니다.
2. 역경매 메커니즘은 봇의 조기 투표를 방지하고 실제 큐레이션을 장려합니다.
3. 가중치 계산은 64비트 정수 범위 내에서 오버플로우를 방지하도록 설계되었습니다.
4. 큐레이션 보상은 첫 지급 시에만 발생하며, 이후 투표는 보상을 받지 못합니다.
