# 평판 시스템

이 문서는 Steem 블록체인의 평판 시스템이 작동하는 방법을 설명합니다. 계산, 업데이트, 규칙 및 표시 형식을 포함합니다.

## 개요

Steem은 커뮤니티 내에서 사용자의 신뢰성과 영향력을 측정하기 위해 **평판 시스템**을 사용합니다. 평판은:
- 핵심 블록체인 상태에 **온체인으로 저장되지 않음**
- 파생 통계로 **reputation plugin에서 추적**됨
- 투표 패턴(업보트 및 다운보트)에서 **계산**됨
- 점수로 **표시**됨 (일반적으로 0-99, 이를 넘을 수도 있음)

## 평판 저장소

### Reputation Object

Reputation plugin은 핵심 체인 상태와 별도로 평판 데이터를 저장합니다 ([reputation_objects.hpp:23](../src/plugins/reputation/include/steem/plugins/reputation/reputation_objects.hpp#L23)):

```cpp
class reputation_object {
    id_type           id;
    account_name_type account;
    share_type        reputation;  // 원시 평판 값
};
```

**주요 속성:**
- `account` - 계정 이름
- `reputation` - 원시 평판 점수 (양수 또는 음수 가능)
- 빠른 조회를 위해 계정 이름으로 인덱싱됨
- Memory-mapped database (chainbase)에 저장됨

### 왜 Plugin 기반인가?

평판은 핵심 합의가 아닌 **plugin**으로 구현됩니다:
- **유연성**: Hardfork 없이 규칙 변경 가능
- **성능**: 블록 검증 속도에 영향을 주지 않음
- **실험**: 비활성화하거나 교체 가능
- **비합의**: 다른 노드가 다른 평판 알고리즘을 사용할 수 있음

## 평판 계산

### 기본 공식

평판은 투표에서 받은 **rshares** (보상 shares)에서 파생됩니다 ([reputation_plugin.cpp:68](../src/plugins/reputation/reputation_plugin.cpp#L68)):

```cpp
// 투표로부터의 원시 평판 델타
rep_delta = cv->rshares >> 6;  // 6만큼 오른쪽 시프트 (64로 나누기)
```

**분석:**
1. **rshares** = 보상 shares로 변환된 투표력
2. **64로 나누기** = 정밀도 감소 (vesting shares의 노이즈 제거)
3. **누적** = 모든 rep_delta 값의 합

### Rshares 계산

사용자가 투표할 때 rshares는 다음을 기반으로 계산됩니다 ([steem_evaluator.cpp:1345](../src/core/chain/steem_evaluator.cpp#L1345)):

**공식:**
```cpp
// 유효 vesting shares (Steem Power)
vesting_shares = voter.vesting_shares + received_vesting_shares;

// 사용된 투표력 (백분율)
used_power = (current_power * vote_weight) / STEEM_100_PERCENT * (60*60*24);
used_power = used_power / (vote_power_reserve_rate * VOTE_REGENERATION_SECONDS);

// 절대 rshares
abs_rshares = (vesting_shares * used_power) / STEEM_100_PERCENT;

// 부호 있는 rshares (업보트는 양수, 다운보트는 음수)
rshares = (vote_weight < 0) ? -abs_rshares : abs_rshares;
```

**Rshares에 영향을 주는 요인:**
- **Vesting Shares (SP)**: SP가 많을수록 rshares 증가
- **Voting Power**: 현재 투표력 (시간이 지나면서 재생성됨)
- **Vote Weight**: 투표 강도 (-10000 ~ +10000, 즉 -100% ~ +100%)
- **Dust Threshold**: 최소 투표 크기 (HF20 이후: `STEEM_VOTE_DUST_THRESHOLD`)

### 평판 업데이트 프로세스

평판 업데이트는 두 단계로 발생합니다:

**1단계: Pre-Operation (이전 투표 취소)** ([reputation_plugin.cpp:54](../src/plugins/reputation/reputation_plugin.cpp#L54))
```cpp
void pre_operation(const vote_operation& op) {
    // 재투표하는 경우 이전 투표의 평판 효과 제거
    if (old_vote_exists) {
        old_rep_delta = old_vote.rshares >> 6;
        author.reputation -= old_rep_delta;
    }
}
```

**2단계: Post-Operation (새 투표 적용)** ([reputation_plugin.cpp:112](../src/plugins/reputation/reputation_plugin.cpp#L112))
```cpp
void post_operation(const vote_operation& op) {
    // 새 투표의 평판 효과 적용
    new_rep_delta = new_vote.rshares >> 6;
    author.reputation += new_rep_delta;
}
```

## 평판 규칙

평판 시스템은 남용을 방지하기 위해 두 가지 주요 규칙을 시행합니다 ([reputation_plugin.cpp:76](../src/plugins/reputation/reputation_plugin.cpp#L76)):

### 규칙 #1: 음수 평판은 다른 사람에게 영향을 줄 수 없음

```cpp
// 규칙 #1: 다른 사용자의 평판에 영향을 주려면 음수가 아닌 평판이 있어야 함
if (voter_rep < 0) return; // 효과 없음
```

**근거:**
- 스팸 계정이 합법적인 사용자를 손상시키는 것을 방지
- 음수 평판을 가진 사용자는 "격리"됨
- 다른 사람에게 영향을 주기 전에 양수 평판을 재구축해야 함

### 규칙 #2: 다운보트는 더 높은 평판 필요

```cpp
// 규칙 #2: 다운보트하는 경우 투표자는 저자보다 더 많은 평판이 있어야 함
if (rshares < 0 && voter_rep <= author_rep) return; // 효과 없음
```

**근거:**
- 낮은 평판 사용자가 높은 평판 사용자를 공격하는 것을 방지
- 다운보트는 "평판 권한"이 필요
- 다운보트하기 전에 평판을 구축하도록 장려

### 업보트 규칙

**업보트는 더 관대합니다:**
- 음수가 아닌 평판은 누구에게나 업보트 가능
- 투표자의 SP에 비례하여 저자의 평판 증가
- 평판 비교 불필요

## 표시 평판 점수

### 원시에서 표시로 전환

원시 평판 값은 사용자 친화적인 표시 점수로 변환됩니다 (일반적으로 0-99):

**일반적인 공식 (프론트엔드에서 사용):**
```python
def reputation_to_display(raw_reputation):
    if raw_reputation == 0:
        return 25

    neg = raw_reputation < 0
    raw_reputation = abs(raw_reputation)

    # 로그 척도
    leading_digits = len(str(raw_reputation)) - 1
    log_value = math.log10(raw_reputation)

    # 표시 점수 계산
    score = (log_value - leading_digits) * 9 + leading_digits

    if neg:
        score = 50 - score
    else:
        score = 50 + score

    return max(score, -9)  # 최소 표시 값
```

**특성:**
- **로그 척도**: 각 자릿수 증가마다 약 9점 추가
- **기본 점수**: 새 사용자는 25 (원시 평판 = 0)
- **일반적인 범위**: 0-99 (이를 넘을 수도 있음)
- **음수 값**: 음수로 표시 (예: -5, -10)

### 표시 점수 예제

| 원시 평판 | 표시 점수 | 설명 |
|----------------|---------------|-------------|
| 0 | 25 | 새 계정, 받은 투표 없음 |
| 1,000 | ~30 | 약간 긍정적인 평판 |
| 10,000 | ~39 | 보통 긍정적인 평판 |
| 100,000 | ~48 | 좋은 평판 |
| 1,000,000 | ~57 | 강한 평판 |
| 10,000,000 | ~66 | 매우 강한 평판 |
| 100,000,000 | ~75 | 우수한 평판 |
| 1,000,000,000 | ~84 | 뛰어난 평판 |
| -1,000 | ~20 | 약간 부정적인 평판 |
| -10,000 | ~11 | 보통 부정적인 평판 |
| -100,000 | ~2 | 강한 부정적인 평판 |

## Reputation API

### 평판 조회

**reputation_api를 통해** ([reputation_api.hpp:27](../src/plugins/apis/reputation_api/include/steem/plugins/reputation_api/reputation_api.hpp#L27)):

```json
// 요청
{
  "jsonrpc": "2.0",
  "method": "reputation_api.get_account_reputations",
  "params": {
    "account_lower_bound": "alice",
    "limit": 100
  },
  "id": 1
}

// 응답
{
  "result": {
    "reputations": [
      {
        "account": "alice",
        "reputation": "123456789"
      },
      {
        "account": "bob",
        "reputation": "987654321"
      }
    ]
  }
}
```

### API 메서드

**`get_account_reputations(account_lower_bound, limit)`**
- 계정의 평판 데이터 반환
- `account_lower_bound` - 이 계정부터 시작 (알파벳순)
- `limit` - 최대 결과 수 (기본값: 1000)
- `{account, reputation}` 객체 배열 반환

## 평판 역학

### 평판 구축

**긍정적인 행동:**
- 높은 SP 사용자로부터 업보트 받기
- 좋은 평판을 가진 사용자로부터 업보트 받기
- 업보트를 받는 양질의 콘텐츠 생성
- 커뮤니티와 긍정적으로 참여

**성장률:**
- 로그 척도는 수익 체감을 의미
- 높은 레벨에서는 증가하기 더 어려움
- 일관된 품질 기여 필요

### 평판 잃기

**부정적인 행동:**
- 높은 SP 사용자로부터 다운보트 받기
- 높은 평판 사용자로부터 다운보트 받기
- 낮은 품질 또는 스팸 콘텐츠 생성
- 커뮤니티 표준 위반

**피해 제한:**
- 낮은 평판 사용자는 귀하의 평판을 손상시킬 수 없음 (규칙 #1)
- 귀하보다 낮은 평판을 가진 사용자는 효과적으로 다운보트할 수 없음 (규칙 #2)
- 확립된 사용자를 스팸 공격으로부터 보호

### 평판 회복

**음수 평판에서 회복:**
1. 다운보트를 유발하는 행동 중단
2. 양질의 콘텐츠 생성
3. 확립된 사용자로부터 업보트 획득
4. 점진적으로 긍정적인 평판 재구축
5. 일단 긍정적이면 다시 다른 사람에게 영향을 줄 수 있음 (규칙 #1)

## 평판 사용 사례

### 콘텐츠 플랫폼

**게시물 가시성:**
- 높은 평판 게시물이 더 두드러지게 표시됨
- 낮은/음수 평판은 필터링되거나 숨겨짐
- 평판 점수 기반 스팸 탐지

**사용자 신뢰:**
- 사회적 증명으로서의 평판
- 팔로워/구독자 결정에 영향
- 큐레이션 보상 분배에 영향

### 커뮤니티 조정

**다운보트 권한:**
- 높은 평판 사용자는 더 많은 조정 권한을 가짐
- 낮은 평판 사용자는 다운보트에 제한됨 (규칙 #2)
- 신뢰의 계층 생성

**스팸 방지:**
- 음수 평판 사용자 격리 (규칙 #1)
- 스팸 공격의 효과 감소
- 자체 규제 커뮤니티 조정

### 경제적 영향

**간접적 효과:**
- 평판은 보상에 직접 영향을 주지 않음
- 높은 평판은 더 많은 투표를 유치할 수 있음
- 낮은 평판은 투표자를 억제할 수 있음
- 심리적 및 사회적 영향

## 구현 세부사항

### Plugin 아키텍처

**초기화** ([reputation_plugin.cpp:196](../src/plugins/reputation/reputation_plugin.cpp#L196)):
```cpp
void plugin_initialize(const variables_map& options) {
    // Operation 핸들러 등록
    _pre_apply_operation_conn = db.add_pre_apply_operation_handler(
        [&](const operation_notification& note) {
            my->pre_operation(note);
        }
    );

    _post_apply_operation_conn = db.add_post_apply_operation_handler(
        [&](const operation_notification& note) {
            my->post_operation(note);
        }
    );

    // Database에 reputation 인덱스 추가
    add_plugin_index<reputation_index>(db);
}
```

### Operation Hook

**Vote operation 처리:**
1. `pre_operation` - 이전 투표의 평판 효과 제거
2. 투표가 핵심 블록체인에서 처리됨
3. `post_operation` - 새 투표의 평판 효과 적용

**Visitor 패턴:**
```cpp
struct post_operation_visitor {
    void operator()(const vote_operation& op) {
        // Vote operation 처리
    }

    template<typename T>
    void operator()(const T&) {
        // 다른 operation 무시
    }
};
```

### Database 인덱스

**Reputation 인덱스** ([reputation_objects.hpp:46](../src/plugins/reputation/include/steem/plugins/reputation/reputation_objects.hpp#L46)):
```cpp
typedef multi_index_container<
    reputation_object,
    indexed_by<
        ordered_unique<tag<by_id>, member<..., id>>,
        ordered_unique<tag<by_account>, member<..., account>>
    >
> reputation_index;
```

**조회 메서드:**
- ID별: `O(log n)` 조회
- 계정별: `O(log n)` 조회
- Chainbase에 영구 저장

## 구성

### Reputation Plugin 활성화

```ini
# config.ini
plugin = reputation
plugin = reputation_api
```

**의존성:**
- `chain` plugin 필요
- 선택사항: RPC 쿼리를 위한 `reputation_api`

### 빌드 옵션

특별한 빌드 플래그가 필요하지 않습니다. Reputation plugin은 표준 빌드의 일부입니다.

## 엣지 케이스 및 특수 시나리오

### 초기 계정 생성

**새 계정:**
- 원시 평판 = 0으로 시작
- 표시 점수 = 25
- 누구에게서나 투표를 받을 수 있음
- 다른 사람을 업보트할 수 있음 (하지만 낮은 SP로 인해 영향 적음)

### 자체 투표

**자체 업보트:**
- 기술적으로 허용됨
- 평판에 영향 (자신의 SP로 제한됨)
- 일반적으로 커뮤니티에서 권장하지 않음
- 일부 프론트엔드는 평판 계산에서 자체 투표를 숨김

### 투표 변경

**투표 변경:**
- Pre-operation이 이전 투표의 평판 효과 제거
- Post-operation이 새 투표의 평판 효과 적용
- 순 변경 = new_rep - old_rep

**투표 제거:**
- weight = 0으로 처리됨
- 평판 효과 제거
- 투표 전 상태로 평판 반환

### 지급 윈도우

**지급 후:**
- 지급 후 투표는 평판에 영향을 주지 않음 (이전 동작)
- HF12 이후: 투표가 기록되지만 평판은 이미 계산됨
- 일부 구성에서는 투표 변경이 여전히 평판을 업데이트할 수 있음

## 보안 고려사항

### Sybil 공격

**보호 메커니즘:**
- 규칙 #1: 음수 평판은 다른 사람에게 영향을 줄 수 없음
- 규칙 #2: 낮은 평판은 높은 평판을 다운보트할 수 없음
- 평판에 영향을 주려면 상당한 SP 필요
- SP 획득에는 경제적 비용 발생

**공격 난이도:**
- 여러 계정 생성: 계정 생성 수수료로 제한
- SP 획득: 자본 투자 또는 시간 필요
- 평판 구축: 커뮤니티 수용 필요

### 평판 조작

**과제:**
- 순환 투표는 커뮤니티에서 탐지됨
- 투표 구매는 탐지 가능하고 권장되지 않음
- Bid-bot 투표는 평판에 비례적으로 영향을 주지 않을 수 있음
- 커뮤니티 다운보트가 조작에 대응할 수 있음

### 프라이버시 고려사항

**평판 가시성:**
- 모든 투표는 블록체인에서 공개됨
- 평판 계산은 결정론적임
- 블록체인 히스토리에서 재계산 가능
- 투표 패턴에 대한 프라이버시 없음

## 평판 vs 합의

### 비합의 특성

**중요한 구분:**
- 평판은 블록체인 합의의 **일부가 아님**
- 다른 노드가 다른 평판 값을 가질 수 있음
- 평판은 블록 유효성에 영향을 주지 않음
- Plugin은 합의를 깨지 않고 비활성화할 수 있음

**의미:**
- Hardfork 없이 변경 가능
- 다른 UI가 다른 평판 점수를 표시할 수 있음
- 합의 중요 로직에 적합하지 않음
- 사용자 경험 및 사회적 신호에만 사용됨

### 대안 알고리즘

**가능한 변형:**
- 다른 투표 가중치 계산
- 대안 표시 공식
- 시간 감쇠 요인
- 카테고리별 평판
- 사용자 정의 평판 규칙

프론트엔드는 여전히 블록체인의 투표 데이터를 사용하면서 자체 평판 계산을 구현할 수 있습니다.

## 모니터링 및 디버깅

### 로그

```bash
# Reputation plugin 로깅 활성화
log-logger = {"name":"reputation","level":"debug","appender":"stderr"}
```

### 쿼리

```bash
# 특정 계정의 평판 가져오기
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"reputation_api.get_account_reputations",
  "params":{"account_lower_bound":"alice","limit":1},
  "id":1
}' http://localhost:8080

# Reputation plugin이 로드되었는지 확인
curl -s http://localhost:8080 | jq '.plugins[]' | grep reputation
```

### 일반적인 문제

**문제: 평판이 업데이트되지 않음**
- 확인: Reputation plugin이 활성화되어 있는가?
- 확인: Vote operation이 처리되고 있는가?
- 확인: Database 인덱스가 제대로 초기화되었는가?

**문제: 예상치 못한 평판 값**
- 확인: Vote rshares 계산
- 확인: 평판 규칙 (음수 투표자, 불충분한 평판)
- 확인: Vote weight 및 voting power

**문제: 평판 표시 불일치**
- 확인: 프론트엔드 표시 공식
- 확인: 원시 vs 표시 변환
- 확인: 정밀도 및 반올림

## 관련 문서

- [Plugin Development Guide](plugin.md) - 사용자 정의 plugin 생성
- [Block Confirmation Process](block-confirmation.md) - 투표가 확인되는 방법
- [P2P Implementation](p2p-implementation.md) - 투표의 네트워크 전파

## 요약

**핵심 요점:**
- ✅ 평판은 투표 rshares (SP 가중)에서 파생됨
- ✅ Plugin에서 저장, 핵심 합의에는 없음
- ✅ 두 가지 주요 규칙: 음수 평판 격리, 다운보트는 더 높은 평판 필요
- ✅ 로그 표시 척도 (일반적으로 25-99)
- ✅ 비합의: 노드/프론트엔드 간에 다를 수 있음
- ✅ 사회적 신호, 경제적 가치 아님
- ✅ 자체 규제 커뮤니티 조정 시스템

**평판 공식 (단순화):**
```
원시 평판 = Σ(vote_rshares >> 6)
표시 점수 = f(log₁₀(|원시 평판|))  // 50 주변의 로그 척도
```
