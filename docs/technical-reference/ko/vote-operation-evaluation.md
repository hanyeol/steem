# Vote Operation 평가

이 문서는 Steem 블록체인의 핵심 투표 로직을 구현하는 `vote_evaluator::do_apply` 함수에 대한 상세한 기술 설명을 제공합니다.

## 목차

- [개요](#개요)
- [Vote Operation 구조](#vote-operation-구조)
- [핵심 상수](#핵심-상수)
- [평가 흐름](#평가-흐름)
- [Voting Power 메커니즘](#voting-power-메커니즘)
- [Reward Shares (rshares) 이해하기](#reward-shares-rshares-이해하기)
- [Reward Share 계산](#reward-share-계산)
- [큐레이션 보상 계산](#큐레이션-보상-계산)
- [투표 변경](#투표-변경)
- [엣지 케이스 및 특수 조건](#엣지-케이스-및-특수-조건)
- [구현 세부사항](#구현-세부사항)
- [관련 코드](#관련-코드)

## 개요

vote evaluator는 Steem 블록체인의 vote operation을 처리하며, 업보트, 다운보트, 투표 변경을 다룹니다. 이는 다음을 관리합니다:

- Voting power 소비 및 재생
- 스테이크와 voting power를 기반으로 한 reward share (rshares) 계산
- 역경매 메커니즘을 사용한 큐레이션 보상 가중치 계산
- 투표 추적 및 변경 제한
- 댓글 보상 시스템과의 통합

**소스 위치**: [libraries/chain/steem_evaluator.cpp:1150](libraries/chain/steem_evaluator.cpp#L1150)

## Vote Operation 구조

```cpp
struct vote_operation : public base_operation
{
   account_name_type    voter;      // 투표하는 계정
   account_name_type    author;     // 댓글 작성자
   string               permlink;   // 댓글 permlink
   int16_t              weight = 0; // 투표 가중치 (-10000 ~ 10000)

   void validate() const;
   void get_required_posting_authorities(flat_set<account_name_type>& a) const
   {
      a.insert(voter);
   }
};
```

**필드:**
- `voter`: 투표하는 계정 (posting 권한 필요)
- `author`: 투표 대상 댓글/게시물의 작성자
- `permlink`: 댓글/게시물의 영구 링크 식별자
- `weight`: -10000 (100% 다운보트) ~ +10000 (100% 업보트), 0은 투표 제거

## 핵심 상수

| 상수 | 값 | 설명 |
|----------|-------|-------------|
| `STEEM_MIN_VOTE_INTERVAL_SEC` | 3초 | 동일 계정의 투표 간 최소 시간 |
| `STEEM_VOTE_REGENERATION_SECONDS` | 432,000 (5일) | 100% voting power 재생 시간 |
| `STEEM_VOTE_DUST_THRESHOLD` | 50,000,000 | 투표가 유효하기 위한 최소 rshares (50 VESTS) |
| `STEEM_UPVOTE_LOCKOUT` | 12시간 | 지급 전 업보트가 잠기는 시간 |
| `STEEM_MAX_VOTE_CHANGES` | 5 | 투표를 변경할 수 있는 최대 횟수 |
| `STEEM_REVERSE_AUCTION_WINDOW_SECONDS` | 1,800 (30분) | 큐레이션 보상의 역경매 시간 |
| `STEEM_100_PERCENT` | 10,000 | 베이시스 포인트로 표현한 100% |

### STEEM_MIN_VOTE_INTERVAL_SEC

**값**: `3`초
**소스**: [libraries/protocol/include/steem/protocol/config.hpp:106](libraries/protocol/include/steem/protocol/config.hpp#L106)

**목적**: 스팸 및 남용 방지를 위한 속도 제한

**구현**:
```cpp
int64_t elapsed_seconds = (_db.head_block_time() - voter.last_vote_time).to_seconds();
FC_ASSERT(elapsed_seconds >= STEEM_MIN_VOTE_INTERVAL_SEC,
    "Can only vote once every 3 seconds.");
```

**세부사항**:
- 동일 계정의 연속 투표 간 최소 3초 간격 강제
- 모든 투표 유형에 적용 (신규 투표, 투표 변경, 업보트, 다운보트)
- 연사포 투표 공격 방지 및 블록체인 스팸 감소
- 타이머는 모든 투표마다 재설정됨

**예시**:
```
시간 0:00 - Alice가 게시물 A에 투표 ✓
시간 0:02 - Alice가 게시물 B에 투표 시도 ✗ (2초만 경과)
시간 0:03 - Alice가 게시물 B에 투표 ✓ (3초 경과)
```

---

### STEEM_VOTE_REGENERATION_SECONDS

**값**: `432,000`초 (5일)
**소스**: [libraries/protocol/include/steem/protocol/config.hpp:103](libraries/protocol/include/steem/protocol/config.hpp#L103)

**목적**: Voting power 재생률 제어

**구현**:
```cpp
int64_t regenerated_power = (STEEM_100_PERCENT * elapsed_seconds) / STEEM_VOTE_REGENERATION_SECONDS;
int64_t current_power = std::min(int64_t(voter.voting_power + regenerated_power),
                                 int64_t(STEEM_100_PERCENT));
```

**세부사항**:
- Voting power는 5일에 걸쳐 0%에서 100%로 선형 재생
- 재생률: **하루 20%** 또는 **시간당 0.833%**
- 재생은 연속적이며 초 단위로 계산
- 100%로 제한 - 초과 재생은 폐기됨
- 사용자가 시간에 걸쳐 투표를 분산하도록 장려하기 위해 설계됨

**재생 타임라인**:

| 경과 시간 | 재생된 Voting Power |
|--------------|--------------------------|
| 1시간 | 0.833% |
| 12시간 | 10% |
| 1일 | 20% |
| 2.5일 | 50% |
| 5일 | 100% |

**실용적 예시**:
```
0일 00:00 - 사용자가 100% voting power 보유
0일 12:00 - 사용자가 10번의 full-power 투표, power가 ~80%로 하락
1일 12:00 - Power가 ~100%로 재생 (80% + 20%)
```

---

### STEEM_VOTE_DUST_THRESHOLD

**값**: `50,000,000` raw 단위 (50 VESTS, VESTS는 소수점 6자리이므로)
**소스**: [libraries/protocol/include/steem/protocol/config.hpp:107](libraries/protocol/include/steem/protocol/config.hpp#L107)

**목적**: 경제적으로 의미 없는 투표 필터링

**구현**:
```cpp
int64_t abs_rshares = ((uint128_t(effective_vesting) * used_power) / STEEM_100_PERCENT).to_uint64();
abs_rshares -= STEEM_VOTE_DUST_THRESHOLD;
abs_rshares = std::max(int64_t(0), abs_rshares);

FC_ASSERT(abs_rshares > 0, "Cannot vote with 0 rshares.");
```

**세부사항**:
- 모든 투표의 계산된 rshares에서 차감됨
- 결과 rshares가 ≤ 0이면 투표가 거부됨
- 미미한 경제적 가치를 가진 마이크로 투표로부터 블록체인 블로트 방지
- Vesting shares와 voting power에서 rshares를 계산한 **후** 적용됨
- 업보트와 다운보트 모두에 동일한 임계값 적용

**필요한 최소 스테이크**:

100% 투표 가중치(~2% voting power 소비)로 유효한 투표를 하려면 약 **250,000,000 VESTS**의 effective vesting shares가 필요합니다.

**예시 계산**:
```
사용자가 1,000,000 VESTS effective vesting 보유
100% 투표 가중치, used_power = 2,000

abs_rshares = (1,000,000 * 2,000) / 10,000 = 200,000
abs_rshares -= 50,000,000 = -49,800,000
abs_rshares = max(0, -49,800,000) = 0

투표 거부됨: "Cannot vote with 0 rshares."
```

**이것이 중요한 이유**:
- 미미한 스테이크를 가진 계정의 스팸 방지
- 블록체인의 저장 요구사항 감소
- 사용자가 투표에 참여하기 위해 파워 업(STEEM 스테이킹)하도록 장려

---

### STEEM_UPVOTE_LOCKOUT

**값**: `12시간` (43,200초)
**소스**: [libraries/protocol/include/steem/protocol/config.hpp:25](libraries/protocol/include/steem/protocol/config.hpp#L25)

**목적**: 지급 전 막판 투표 조작 방지

**구현**:
```cpp
if (rshares > 0)  // 업보트 또는 증가된 업보트에만 적용
{
    FC_ASSERT(_db.head_block_time() < comment.cashout_time - STEEM_UPVOTE_LOCKOUT,
        "Cannot increase payout within last twelve hours before payout.");
}
```

**세부사항**:
- 지급을 **증가**시키는 업보트 및 투표 변경에만 적용됨
- 다운보트는 lockout 기간에도 **항상 허용**됨
- 다음의 경우 트리거됨:
  - 새로운 업보트 생성 (rshares > 0)
  - 기존 투표를 더 높은 rshares로 변경
- 다음의 경우 트리거되지 않음:
  - 다운보트
  - 업보트 감소
  - 투표 제거 (가중치를 0으로 변경)

**타임라인 예시**:
```
게시물 생성: 0일 00:00
지급 시간: 7일 00:00
Lockout 시작: 6일 12:00 (지급 12시간 전)

6일 11:59 - 업보트 가능 ✓
6일 12:00 - 업보트 잠김 ✗
6일 12:01 - 여전히 다운보트 가능 ✓
7일 00:00 - 게시물 지급, 투표 비활성화
```

**근거**:
- 지급 직전 조직적 업보트 공격 방지
- 필요한 경우 커뮤니티가 다운보트로 대응할 시간 제공
- 투표 구매 계획의 효과 감소
- **비대칭 설계**: 업보트는 잠기지만 다운보트는 커뮤니티 보호를 위해 허용됨

---

### STEEM_MAX_VOTE_CHANGES

**값**: `5`
**소스**: [libraries/protocol/include/steem/protocol/config.hpp:104](libraries/protocol/include/steem/protocol/config.hpp#L104)

**목적**: 투표 조작 및 큐레이션 게이밍 제한

**구현**:
```cpp
FC_ASSERT(itr->num_changes < STEEM_MAX_VOTE_CHANGES,
    "Voter has used the maximum number of vote changes on this comment.");
```

**세부사항**:
- 각 계정은 특정 댓글에 대한 투표를 최대 5번까지 변경 가능
- 초기 투표는 제한에 **포함되지 않음**
- 후속 변경만 `num_changes` 증가
- 댓글별로 적용되며 전역적이지 않음
- 5번 변경 후 게시물 지급까지 투표가 잠김

**투표 변경 추적**:
```
초기 투표:     num_changes = 0 (포함 안 됨)
1차 변경:      num_changes = 1
2차 변경:      num_changes = 2
3차 변경:      num_changes = 3
4차 변경:      num_changes = 4
5차 변경:      num_changes = 5
6차 변경:      거부됨 ✗ "Voter has used the maximum number of vote changes"
```

**중요한 페널티**:
```cpp
// 투표 변경 시
cv.weight = 0;  // 변경된 투표에는 큐레이션 보상 없음
cv.num_changes += 1;
```

변경된 투표는 **큐레이션 보상 없음**을 받습니다. 이것이 주요 안티게이밍 메커니즘입니다.

**근거**:
- 사용자가 큐레이션 보상을 게이밍하기 위해 지속적으로 투표를 조정하는 것을 방지
- 진정한 실수를 수정할 수 있음 (5번 변경은 관대함)
- 유연성과 남용 방지 보호의 균형

---

### STEEM_REVERSE_AUCTION_WINDOW_SECONDS

**값**: `1,800`초 (30분)
**소스**: [libraries/protocol/include/steem/protocol/config.hpp:105](libraries/protocol/include/steem/protocol/config.hpp#L105)

**목적**: 봇 투표 억제 및 수동 큐레이션 보상

**구현**:
```cpp
uint128_t w(max_vote_weight);
uint64_t delta_t = std::min(
    uint64_t((cv.last_update - comment.created).to_seconds()),
    uint64_t(STEEM_REVERSE_AUCTION_WINDOW_SECONDS)
);

w *= delta_t;
w /= STEEM_REVERSE_AUCTION_WINDOW_SECONDS;
cv.weight = w.to_uint64();
```

**세부사항**:
- 큐레이션 보상 가중치는 투표 타이밍에 따라 선형적으로 스케일됨
- 게시물 생성 시(t=0) 큐레이션 가중치 0%
- 생성 30분 후 큐레이션 가중치 100%
- 페널티는 큐레이션 부분에만 적용되며 작성자 보상은 아님
- 역경매로 손실된 큐레이션 보상은 다른 큐레이터가 아닌 **작성자**에게 돌아감

**투표 시간별 큐레이션 가중치**:

| 게시물 생성 후 시간 | 큐레이션 가중치 | 작성자에게 |
|--------------------------|-----------------|----------------|
| 0초 | 0% | 100% |
| 5분 | 16.7% | 83.3% |
| 10분 | 33.3% | 66.7% |
| 15분 | 50% | 50% |
| 20분 | 66.7% | 33.3% |
| 25분 | 83.3% | 16.7% |
| 30분 이상 | 100% | 0% |

**공식**:
```
actual_curation_weight = max_weight × min(time_elapsed, 1800) / 1800
penalty_to_author = max_weight - actual_curation_weight
```

**이것이 존재하는 이유**:
1. **안티봇 메커니즘**: 즉시 투표하는 자동화 투표 봇은 감소된 보상을 받음
2. **품질 큐레이션 보상**: 사용자가 투표하기 전에 콘텐츠를 읽도록 장려
3. **작성자 혜택**: 작성자는 초기 투표에서 추가 보상을 받음
4. **수동 발견 장려**: 첫 번째 수동 큐레이터는 여전히 30분경에 투표하면 보상을 받을 수 있음

**전략적 투표**:
- **즉시 투표(0분)**: 강력하게 지지하는 작성자에게 좋음 (그들이 모든 큐레이션 보상을 받음)
- **15분에 투표**: 균형 - 50% 큐레이션 보상
- **30분 이상에 투표**: 최대 큐레이션 보상, 하지만 더 많은 투표자와 경쟁

**실제 예시**:
```
게시물 게시: 오전 10:00
봇이 오전 10:00에 투표: 0% 큐레이션 가중치 획득
사람이 읽고 오전 10:15에 투표: 50% 큐레이션 가중치 획득
오전 10:30 투표자: 100% 큐레이션 가중치 획득, 하지만 경쟁으로 인한 작은 몫
```

**트레이드오프**:
- 일찍 투표 → 작성자를 더 돕고, 개인 큐레이션 보상 낮음
- 30분 이상에 투표 → 개인 큐레이션 최대, 하지만 보상 풀에 대한 더 많은 경쟁

---

### STEEM_100_PERCENT

**값**: `10,000` (베이시스 포인트)
**소스**: [libraries/protocol/include/steem/protocol/config.hpp:116](libraries/protocol/include/steem/protocol/config.hpp#L116)

**목적**: 프로토콜 전체에서 표준 백분율 표현

**코드베이스 전체 사용**:
```cpp
// 투표 가중치 검증
FC_ASSERT(abs(weight) <= STEEM_100_PERCENT, "Weight is not a STEEMIT percentage");

// Voting power 계산
int64_t current_power = std::min(int64_t(voter.voting_power + regenerated_power),
                                 int64_t(STEEM_100_PERCENT));

// Rshares 계산
int64_t abs_rshares = ((uint128_t(effective_vesting) * used_power) / STEEM_100_PERCENT).to_uint64();
```

**세부사항**:
- 100%를 10,000 단위로 표현 (베이시스 포인트)
- 1% = 100 단위
- 0.01% = 1 단위 (최소 세밀도)
- 다음에 일관되게 사용:
  - 투표 가중치 (-10000 ~ +10000)
  - Voting power (0 ~ 10000)
  - 프로토콜 전체의 모든 백분율 계산

**부동소수점 대신 베이시스 포인트를 사용하는 이유**:
1. **결정론**: 부동소수점 연산은 플랫폼 간 다른 결과를 낼 수 있음
2. **정밀도**: 정수 수학은 정확하고 재현 가능함
3. **합의**: 모든 노드는 동일한 결과를 계산해야 함
4. **성능**: 정수 연산이 부동소수점보다 빠름

**일반적인 변환**:

| 백분율 | 베이시스 포인트 |
|------------|--------------|
| 100% | 10,000 |
| 50% | 5,000 |
| 10% | 1,000 |
| 1% | 100 |
| 0.1% | 10 |
| 0.01% | 1 |

**예시 계산**:
```cpp
// 사용자가 50% 가중치로 투표
vote_weight = 5000;  // STEEM_100_PERCENT의 50%

// 현재 voting power는 80%
voting_power = 8000;  // STEEM_100_PERCENT의 80%

// 사용된 power 비율 계산
used_power = (voting_power * abs(vote_weight)) / STEEM_100_PERCENT;
// = (8000 * 5000) / 10000 = 4000
```

## 평가 흐름

vote evaluator는 다음 실행 흐름을 따릅니다:

```
1. 데이터베이스에서 comment 및 voter 계정 객체 검색
2. 투표자 자격 확인 (투표권을 포기하지 않음)
3. 댓글에 투표가 허용되는지 확인 (업보트의 경우)
4. 만료된 게시물 처리 (지급 시간 = 최대)
   └─> 투표 메타데이터만 기록, 모든 보상 계산 건너뜀
5. 투표 타이밍 검증 (3초 최소 간격)
6. Voting power 재생 및 현재 power 계산
7. 투표 가중치를 기반으로 voting power 소비 계산
8. Effective vesting shares 및 absolute rshares 계산
9. 분기: 신규 투표 vs. 투표 변경
   ├─> 신규 투표: comment_vote_object 생성, comment rshares 업데이트
   └─> 투표 변경: 기존 투표 수정, comment rshares 조정
10. 큐레이션 보상 가중치 계산 (자격이 있는 경우)
11. 투표자의 voting power 및 마지막 투표 시간 업데이트
12. 댓글 통계 업데이트 (net_votes, net_rshares 등)
```

## Voting Power 메커니즘

### Voting Power 표현

Voting power는 0에서 10,000 (0%에서 100%) 범위이며 시간에 따라 선형적으로 재생됩니다.

### 재생 공식

```cpp
int64_t elapsed_seconds = (current_time - last_vote_time).to_seconds();
int64_t regenerated_power = (STEEM_100_PERCENT * elapsed_seconds) / STEEM_VOTE_REGENERATION_SECONDS;
int64_t current_power = min(voter.voting_power + regenerated_power, STEEM_100_PERCENT);
```

**예시:**
- 마지막 투표 이후 86,400초 (1일) 경과한 경우:
  - `regenerated_power = (10,000 * 86,400) / 432,000 = 2,000` (20%)
  - Voting power는 하루에 20% 재생됨

### Voting Power 소비

```cpp
int64_t abs_weight = abs(vote_weight);
int64_t used_power = ((current_power * abs_weight) / STEEM_100_PERCENT) * (60*60*24);
int64_t max_vote_denom = vote_power_reserve_rate * STEEM_VOTE_REGENERATION_SECONDS;
used_power = (used_power + max_vote_denom - 1) / max_vote_denom;
```

**핵심 포인트:**
- 100% 투표 가중치는 약 2%의 voting power를 소비함
- `vote_power_reserve_rate` (dynamic global property)가 소비를 조정함
- `(60*60*24)` 인수 (86,400)는 레거시 구현과의 하위 호환성을 보장함
- 나눗셈은 올림되어 power가 0인 투표를 방지함

**기본 vote_power_reserve_rate = 50인 경우 예시:**
- 100% 가중치 투표: ~2% voting power 소비
- 50% 가중치 투표: ~1% voting power 소비
- 10% 가중치 투표: ~0.2% voting power 소비

## Reward Shares (rshares) 이해하기

### Reward Shares란?

**Reward Shares (rshares)**는 Steem 블록체인에서 투표 영향력의 기본 단위입니다. 이는 투표자의 스테이크(VESTS)와 voting power 소비로부터 계산된 게시물의 보상에 대한 투표의 영향력 양을 나타냅니다.

### 핵심 개념

#### rshares (Reward Shares)
- **정의**: 게시물에 대한 서명된 투표 영향력
- **범위**: 양수(업보트) 또는 음수(다운보트) 가능
- **목적**: 게시물의 순 보상 가치 결정
- **계산**: 투표자의 effective vesting shares와 사용된 voting power를 기반으로 함

```cpp
int64_t rshares = vote_weight < 0 ? -abs_rshares : abs_rshares;
```

**부호 규약**:
- **양수 rshares**: 업보트 (게시물 보상 증가)
- **음수 rshares**: 다운보트 (게시물 보상 감소)
- **0 rshares**: 투표 제거 또는 미미한 스테이크

#### abs_rshares (Absolute Reward Shares)
- **정의**: 항상 양수인 투표 영향력의 절대 크기
- **목적**: 방향에 관계없이 총 투표 활동 추적
- **사용**: 큐레이션 보상 및 투표 활동 메트릭 계산에 사용됨

```cpp
int64_t abs_rshares = ((uint128_t(effective_vesting) * used_power) / STEEM_100_PERCENT).to_uint64();
abs_rshares -= STEEM_VOTE_DUST_THRESHOLD;
abs_rshares = std::max(int64_t(0), abs_rshares);
```

### 주요 차이점

| 측면 | rshares | abs_rshares |
|--------|---------|-------------|
| **부호** | 양수 또는 음수 가능 | 항상 양수 (절대값) |
| **목적** | 게시물 지급 방향 결정 | 투표 활동 크기 측정 |
| **사용** | `net_rshares`, `vote_rshares` | `abs_rshares`, 큐레이션 계산 |
| **누적** | 상쇄 가능 (업 vs. 다운) | 항상 누적 |

### Comment Object 필드

`comment_object`는 여러 rshares 관련 필드를 추적합니다:

#### net_rshares
```cpp
comment.net_rshares += rshares;  // 양수 또는 음수 가능
```

- **정의**: 실제 지급을 결정하는 순 reward shares
- **계산**: 모든 업보트 rshares의 합계에서 모든 다운보트 rshares를 뺀 값
- **목적**: 게시물 보상을 결정하는 최종 값
- **음수 가능**: 예 (심하게 다운보트된 게시물은 음수 net_rshares를 가짐)

**예시**:
```
초기: net_rshares = 0
Alice 업보트 (+1,000,000 rshares): net_rshares = 1,000,000
Bob 업보트 (+500,000 rshares): net_rshares = 1,500,000
Charlie 다운보트 (-300,000 rshares): net_rshares = 1,200,000
```

#### abs_rshares
```cpp
comment.abs_rshares += abs_rshares;  // 항상 양수 추가
```

- **정의**: 게시물에 대한 총 절대 투표 활동
- **계산**: 모든 투표의 절대값의 합계
- **목적**: 총 커뮤니티 참여 측정
- **음수 가능**: 아니오 (항상 양수로 누적)

**예시** (위와 동일한 투표):
```
초기: abs_rshares = 0
Alice 업보트 (1,000,000): abs_rshares = 1,000,000
Bob 업보트 (500,000): abs_rshares = 1,500,000
Charlie 다운보트 (300,000 절대값): abs_rshares = 1,800,000
```

#### vote_rshares
```cpp
if (rshares > 0)
    comment.vote_rshares += rshares;  // 업보트만
```

- **정의**: 큐레이션 보상 계산을 위한 누적 업보트 rshares
- **계산**: 양수 (업보트) rshares만의 합계
- **목적**: 큐레이션 보상 분배 결정
- **제외**: 다운보트 (업보트만 카운트)

**예시** (위와 동일한 투표):
```
초기: vote_rshares = 0
Alice 업보트 (+1,000,000): vote_rshares = 1,000,000
Bob 업보트 (+500,000): vote_rshares = 1,500,000
Charlie 다운보트: vote_rshares = 1,500,000 (변경 없음, 다운보트는 카운트 안 됨)
```

#### children_abs_rshares
```cpp
root_comment.children_abs_rshares += abs_rshares;  // 루트 게시물용
```

- **정의**: 모든 답글에 대한 총 절대 투표 활동
- **계산**: 모든 자식 댓글의 abs_rshares의 합계
- **목적**: 전체 토론 스레드에 대한 참여 추적
- **적용 대상**: 루트 게시물만 (모든 답글로부터 누적)

### 실용적 예시

#### 예시 1: 순수 업보트 시나리오
```
게시물이 3개의 업보트를 받음:
- 투표 1: +1,000,000 rshares
- 투표 2: +750,000 rshares
- 투표 3: +250,000 rshares

결과:
net_rshares = 2,000,000 (모든 양수의 합계)
abs_rshares = 2,000,000 (절대값의 합계)
vote_rshares = 2,000,000 (업보트만의 합계)
```

#### 예시 2: 혼합 투표 시나리오
```
게시물이 업보트와 다운보트를 받음:
- 투표 1: +2,000,000 rshares (업보트)
- 투표 2: +1,000,000 rshares (업보트)
- 투표 3: -800,000 rshares (다운보트)
- 투표 4: +500,000 rshares (업보트)

결과:
net_rshares = 2,700,000 (2M + 1M - 800K + 500K)
abs_rshares = 4,300,000 (2M + 1M + 800K + 500K, 모두 양수)
vote_rshares = 3,500,000 (2M + 1M + 500K, 업보트만)
```

#### 예시 3: 심하게 다운보트된 게시물
```
게시물이 업보트보다 더 많은 다운보트를 받음:
- 투표 1: +500,000 rshares (업보트)
- 투표 2: -1,000,000 rshares (다운보트)
- 투표 3: -800,000 rshares (다운보트)

결과:
net_rshares = -1,300,000 (500K - 1M - 800K, 음수)
abs_rshares = 2,300,000 (500K + 1M + 800K)
vote_rshares = 500,000 (업보트만 카운트됨)

지급: 0 STEEM (음수 net_rshares → 보상 없음)
```

### 보상 계산 영향

다양한 rshares 필드는 보상 분배에서 뚜렷한 목적을 제공합니다:

1. **게시물 작성자 보상**: 보상 곡선을 적용한 후 `net_rshares`를 기반으로 함
   ```cpp
   fc::uint128_t reward_rshares = std::max(comment.net_rshares.value, int64_t(0));
   reward_rshares = util::evaluate_reward_curve(reward_rshares);
   ```

2. **큐레이션 보상**: `vote_rshares` (업보트만)를 기반으로 함
   ```cpp
   uint64_t weight = util::evaluate_reward_curve(
       comment.vote_rshares.value, curve, content_constant);
   ```

3. **활동 메트릭**: `abs_rshares`를 기반으로 함
   - 트렌딩 알고리즘에 사용됨
   - 총 커뮤니티 참여 측정
   - 업보트와 다운보트 모두 포함

### 왜 여러 rshares 필드인가?

rshares를 여러 필드로 분리하는 것은 중요한 목적을 제공합니다:

1. **net_rshares**: 실제 지급 결정 (음수 가능)
2. **abs_rshares**: 총 활동 측정 (항상 양수)
3. **vote_rshares**: 큐레이션 보상 계산 (업보트만)
4. **children_abs_rshares**: 토론 참여 추적

이 설계는 다음을 허용합니다:
- **공정한 다운보트**: 다운보트는 큐레이션 이력을 제거하지 않고 보상을 감소시킴
- **활동 추적**: 혼합 투표에서도 총 참여가 표시됨
- **큐레이션 보호**: 큐레이터는 업보트를 기반으로 보상받으며 이후 다운보트에 영향을 받지 않음
- **스레드 메트릭**: 루트 게시물은 전체 토론의 참여를 추적함

## Reward Share 계산

### Absolute Reward Shares (rshares)

```cpp
auto effective_vesting = db.get_effective_vesting_shares(voter, VESTS_SYMBOL).amount.value;
int64_t abs_rshares = ((uint128_t(effective_vesting) * used_power) / STEEM_100_PERCENT).to_uint64();
abs_rshares -= STEEM_VOTE_DUST_THRESHOLD;
abs_rshares = max(0, abs_rshares);
```

**구성요소:**
1. **Effective Vesting Shares**: 소유 VESTS + 위임 받은 VESTS - 위임한 VESTS
2. **Used Power**: 실제 소비된 voting power (이전 계산에서)
3. **Dust Threshold 차감**: 더스트 투표를 필터링하기 위해 0.05 VESTS 상당 제거
4. **부호 적용**: 업보트 = 양수 rshares, 다운보트 = 음수 rshares

**예시:**
- 투표자가 1,000,000 VESTS effective vesting을 가짐
- 100% 투표 가중치, 2,000 (10,000의 20%) used_power 소비
- `abs_rshares = (1,000,000 * 2,000) / 10,000 - 50,000,000 = 200,000,000 - 50,000,000 = 150,000,000`
- 업보트인 경우: `rshares = +150,000,000`
- 다운보트인 경우 (weight = -10000): `rshares = -150,000,000`

### Comment에 대한 영향

rshares는 댓글의 보상 계산에 영향을 미칩니다:

```cpp
comment.net_rshares += rshares;        // 순 reward shares (업보트 - 다운보트)
comment.abs_rshares += abs_rshares;    // 총 절대 투표 활동
comment.vote_rshares += rshares;       // 업보트만
comment.net_votes += (rshares > 0) ? 1 : -1;  // 투표 카운트
```

**필드:**
- `net_rshares`: 지급을 결정하는 순 reward shares
- `abs_rshares`: 총 투표 활동 (업과 다운 모두)
- `vote_rshares`: 업보트 rshares의 합계 (큐레이션 계산용)
- `net_votes`: 사용자에게 표시되는 순 투표 카운트

## 큐레이션 보상 계산

큐레이션 보상은 아직 지급되지 않은 게시물에 대한 업보트에만 계산됩니다.

### 자격 확인

```cpp
bool curation_reward_eligible = rshares > 0
    && (comment.last_payout == fc::time_point_sec())
    && comment.allow_curation_rewards;

if (curation_reward_eligible)
    curation_reward_eligible = db.get_curation_rewards_percent(comment) > 0;
```

### 가중치 계산

가중치는 큐레이터의 큐레이션 보상 몫을 결정합니다:

```cpp
// 보상 곡선을 기반으로 가중치 계산
const auto& reward_fund = db.get_reward_fund(comment);
auto curve = reward_fund.curation_reward_curve;
uint64_t old_weight = util::evaluate_reward_curve(
    old_vote_rshares.value, curve, reward_fund.content_constant).to_uint64();
uint64_t new_weight = util::evaluate_reward_curve(
    comment.vote_rshares.value, curve, reward_fund.content_constant).to_uint64();
cv.weight = new_weight - old_weight;
```

**가중치는 이 투표의 총 큐레이션 보상 풀에 대한 한계 기여를 나타냅니다.**

### 역경매 메커니즘

최대 큐레이션 보상을 위해 즉시 투표하는 투표 봇을 방지하기 위해:

```cpp
uint128_t w(max_vote_weight);
uint64_t delta_t = min(
    (cv.last_update - comment.created).to_seconds(),
    STEEM_REVERSE_AUCTION_WINDOW_SECONDS
);

w *= delta_t;
w /= STEEM_REVERSE_AUCTION_WINDOW_SECONDS;
cv.weight = w.to_uint64();
```

**작동 방식:**
- 첫 30분 이내의 투표는 큐레이션 가중치가 감소함
- 가중치는 0% (생성 시)에서 100% (30분)까지 선형적으로 스케일됨
- 역경매로 손실된 보상은 다른 큐레이터가 아닌 작성자에게 돌아감

**예시:**
- 0분에 투표: 0% 큐레이션 가중치 (모두 작성자에게)
- 15분에 투표: 50% 큐레이션 가중치
- 30분 이상에 투표: 100% 큐레이션 가중치

## 투표 변경

사용자는 `STEEM_MAX_VOTE_CHANGES` (5)번까지 투표를 변경할 수 있습니다.

### 투표 변경 처리

기존 투표를 수정할 때:

```cpp
// 이전 투표의 효과 되돌림
comment.net_rshares -= itr->rshares;
comment.abs_rshares += abs_rshares;

// 새 투표 적용
comment.net_rshares += rshares;

// 방향 변경에 따라 net_votes 업데이트
if (rshares > 0 && itr->rshares < 0)
    comment.net_votes += 2;  // 다운보트 → 업보트
else if (rshares > 0 && itr->rshares == 0)
    comment.net_votes += 1;  // 중립 → 업보트
else if (rshares == 0 && itr->rshares > 0)
    comment.net_votes -= 1;  // 업보트 → 중립
// ... (더 많은 케이스)
```

### 투표 변경 제한

```cpp
FC_ASSERT(itr->num_changes < STEEM_MAX_VOTE_CHANGES,
    "Voter has used the maximum number of vote changes on this comment.");

FC_ASSERT(itr->vote_percent != o.weight,
    "You have already voted in a similar way.");
```

### 투표 변경 시 큐레이션 가중치

투표를 변경할 때 이전 큐레이션 가중치는 제거되고 **새로운 가중치는 할당되지 않습니다**:

```cpp
comment.total_vote_weight -= itr->weight;  // 이전 가중치 제거

// 투표 객체 업데이트
cv.weight = 0;  // 변경된 투표에는 큐레이션 보상 없음
cv.num_changes += 1;
```

**이것은 반복적으로 투표를 변경하여 큐레이션 보상을 게이밍하는 것을 방지합니다.**

## 엣지 케이스 및 특수 조건

### 1. 만료된 게시물

지급 시간이 지났을 때 (최대로 설정됨):

```cpp
if (db.calculate_discussion_payout_time(comment) == fc::time_point_sec::maximum())
{
#ifndef CLEAR_VOTES
    // 추적을 위해 투표를 기록하지만 모든 보상 계산을 건너뜀
    if (vote exists)
        update vote_percent and last_update
    else
        create minimal comment_vote_object
#endif
    return;  // 조기 종료
}
```

### 2. 지급 후 투표 제한

```cpp
if (itr != comment_vote_idx.end() && itr->num_changes == -1)
{
    FC_ASSERT(false, "Cannot vote again on a comment after payout.");
}
```

**`num_changes = -1`은 지급 후 더 이상 변경할 수 없는 투표를 표시합니다.**

### 3. 업보트 Lockout

```cpp
if (rshares > 0)
{
    FC_ASSERT(db.head_block_time() < comment.cashout_time - STEEM_UPVOTE_LOCKOUT,
        "Cannot increase payout within last twelve hours before payout.");
}
```

**지급 전 마지막 12시간 동안 막판 조작을 줄이기 위해 업보트를 방지합니다.**

**참고:** 이것은 다음의 경우에만 적용됩니다:
- 새로운 업보트 추가
- 더 강한 업보트로 변경 (rshares 증가)
- 다운보트는 제한되지 않음

### 4. 0 가중치 투표

```cpp
FC_ASSERT(o.weight != 0, "Vote weight cannot be 0.");
```

**신규 투표에만 적용됩니다. 0 가중치로 투표를 생성할 수 없지만 기존 투표를 0으로 변경하여 제거할 수 있습니다.**

### 5. 불충분한 Power

```cpp
FC_ASSERT(used_power <= current_power,
    "Account does not have enough power to vote.");
```

### 6. 최소 Rshares

```cpp
FC_ASSERT(abs_rshares > 0, "Cannot vote with 0 rshares.");
```

**Dust threshold 차감 후 투표는 의미 있는 rshares를 가져야 합니다.**

### 7. 투표자 자격

```cpp
FC_ASSERT(voter.can_vote, "Voter has declined their voting rights.");
```

### 8. 댓글이 투표를 허용함

```cpp
if (o.weight > 0)
    FC_ASSERT(comment.allow_votes, "Votes are not allowed on the comment.");
```

**업보트 (weight > 0)에만 확인됩니다. 댓글이 업보트를 비활성화했더라도 다운보트는 항상 허용됩니다.**

## 구현 세부사항

### 데이터베이스 수정

evaluator는 트랜잭션 내에서 여러 데이터베이스 수정을 수행합니다:

1. **투표자 계정 업데이트**
   ```cpp
   db.modify(voter, [&](account_object& a) {
       a.voting_power = current_power - used_power;
       a.last_vote_time = db.head_block_time();
   });
   ```

2. **댓글 업데이트**
   ```cpp
   db.modify(comment, [&](comment_object& c) {
       c.net_rshares += rshares;
       c.abs_rshares += abs_rshares;
       if (rshares > 0) {
           c.vote_rshares += rshares;
           c.net_votes++;
       } else {
           c.net_votes--;
       }
   });
   ```

3. **루트 댓글 업데이트** (게시물에 대한 댓글의 경우)
   ```cpp
   db.modify(root, [&](comment_object& c) {
       c.children_abs_rshares += abs_rshares;
   });
   ```

4. **Comment Vote Object** (신규 투표)
   ```cpp
   db.create<comment_vote_object>([&](comment_vote_object& cv) {
       cv.voter = voter.id;
       cv.comment = comment.id;
       cv.rshares = rshares;
       cv.vote_percent = o.weight;
       cv.last_update = db.head_block_time();
       cv.weight = calculated_curation_weight;
   });
   ```

5. **큐레이션 가중치 업데이트**
   ```cpp
   db.modify(comment, [&](comment_object& c) {
       c.total_vote_weight += max_vote_weight;
   });
   ```

### 하위 호환성 주의사항

```cpp
// 나눗셈을 마지막에 수행하면 반올림 오류가 줄어들지만,
// #1285에서 대체된 이전 구현과의 하위 호환성을 유지해야 합니다
int64_t used_power = ((current_power * abs_weight) / STEEM_100_PERCENT) * (60*60*24);
```

**`* (60*60*24)` 곱셈은 역사적 계산과 일치하도록 최종 나눗셈 전에 수행됩니다.**

### 보상 곡선 평가

보상 곡선 (HF19 이후 Steem에서 현재 선형)은 다음을 결정하기 위해 평가됩니다:
- `net_rshares`를 기반으로 한 댓글 지급
- `vote_rshares`를 기반으로 한 큐레이션 가중치

```cpp
new_rshares = util::evaluate_reward_curve(new_rshares);
old_rshares = util::evaluate_reward_curve(old_rshares);
```

### CLEAR_VOTES 컴파일 옵션

`CLEAR_VOTES`가 정의되면 만료된 게시물에 대한 투표 객체가 메모리 절약을 위해 생성/저장되지 않습니다:

```cpp
#ifndef CLEAR_VOTES
    // 투표 메타데이터 저장
#endif
```

**이것은 일반적으로 witness/seed 노드에서 메모리 사용량을 줄이기 위해 활성화됩니다.**

## 관련 코드

### 주요 파일

- **[libraries/chain/steem_evaluator.cpp](libraries/chain/steem_evaluator.cpp)** - Vote evaluator 구현
- **[libraries/protocol/include/steem/protocol/steem_operations.hpp](libraries/protocol/include/steem/protocol/steem_operations.hpp)** - Vote operation 구조
- **[libraries/protocol/include/steem/protocol/config.hpp](libraries/protocol/include/steem/protocol/config.hpp)** - 핵심 상수
- **[libraries/chain/include/steem/chain/comment_object.hpp](libraries/chain/include/steem/chain/comment_object.hpp)** - Comment object 정의
- **[libraries/chain/util/reward.cpp](libraries/chain/util/reward.cpp)** - 보상 곡선 평가

### 데이터베이스 객체

- `account_object` - voting_power와 last_vote_time을 가진 투표자 계정
- `comment_object` - rshares, 투표 카운트 및 지급 정보를 가진 댓글/게시물
- `comment_vote_object` - rshares와 큐레이션 가중치를 가진 개별 투표 기록
- `dynamic_global_property_object` - vote_power_reserve_rate 같은 전역 매개변수

### 관련 Operation

- `comment_operation` - 투표할 수 있는 게시물과 댓글 생성
- `vote_operation` - 이 문서에서 다룸
- `comment_options_operation` - 댓글에 대한 투표를 비활성화할 수 있음
- `decline_voting_rights_operation` - 계정의 투표 능력을 비활성화할 수 있음

## 참고 자료

- [Reputation System](reputation-system.md) - 투표가 평판에 미치는 영향
- [Chain Parameters](chain-parameters.md) - 전역 블록체인 매개변수
- [System Architecture](system-architecture.md) - 전체 시스템 설계
