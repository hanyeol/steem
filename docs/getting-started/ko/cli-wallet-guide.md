# CLI 월렛 사용자 가이드

Steem 명령줄 월렛(`cli_wallet`) 사용 완벽 가이드.

## 개요

`cli_wallet`은 Steem 블록체인에서 계정, 키 및 트랜잭션을 관리하기 위한 대화형 명령줄 인터페이스입니다. WebSocket을 통해 실행 중인 `steemd` 노드에 연결되며 블록체인 작업을 위한 안전한 환경을 제공합니다.

**기능:**
- 계정 관리 (생성, 업데이트)
- 키 관리 및 월렛 암호화
- 토큰 전송 및 변환
- 콘텐츠 작업 (게시, 투표, 댓글)
- 증인 작업
- 다중 서명 트랜잭션 지원
- 내장 도움말 시스템이 있는 자체 문서화

## 사전 요구사항

### 노드 요구사항

연결하는 `steemd` 노드에는 다음 플러그인이 활성화되어 있어야 합니다. 월렛은 **직접 모듈식 API 호출**을 사용합니다.

**필수 플러그인:**
- `webserver` - WebSocket/HTTP 서버
- `database_api` - 핵심 블록체인 상태 쿼리
- `account_by_key_api` - 공개 키에서 계정으로의 매핑
- `network_broadcast_api` - 트랜잭션 브로드캐스팅

**권장 플러그인** (전체 기능을 위해):
- `block_api` - 블록 및 헤더 쿼리
- `account_history_api` - 계정 트랜잭션 내역
- `tags_api` - 콘텐츠 및 토론 쿼리
- `follow_api` - 소셜 그래프 및 피드
- `reputation_api` - 평판 점수
- `market_history_api` - 시장 데이터 및 거래 내역
- `witness_api` - 증인 및 대역폭 데이터

**최소 구성** (기본 월렛 작업):
```ini
# 핵심 플러그인
plugin = webserver
plugin = database_api
plugin = account_by_key_api
plugin = network_broadcast_api

# 선택 사항이지만 권장
plugin = block_api
plugin = account_history_api

# WebSocket 엔드포인트
webserver-ws-endpoint = 0.0.0.0:8090
```

**풀 노드 구성** (모든 월렛 기능):
```ini
# 핵심 플러그인
plugin = webserver
plugin = database_api
plugin = account_by_key_api
plugin = network_broadcast_api

# 선택 플러그인
plugin = block_api
plugin = account_history_api
plugin = tags_api
plugin = follow_api
plugin = reputation_api
plugin = market_history_api
plugin = witness_api

# WebSocket 엔드포인트
webserver-ws-endpoint = 0.0.0.0:8090
```

### cli_wallet 빌드하기

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) cli_wallet
```

바이너리 위치: `./programs/cli_wallet/cli_wallet`

## 시작하기

### 1. 노드에 연결

```bash
# 로컬 노드
./cli_wallet -s ws://127.0.0.1:8090

# 원격 노드
./cli_wallet -s ws://node.example.com:8090

# 월렛 파일 지정
./cli_wallet -s ws://127.0.0.1:8090 -w my_wallet.json
```

### 2. 첫 실행

첫 실행 시 다음과 같이 표시됩니다:

```
new >>>
```

`new` 프롬프트는 월렛이 아직 생성되지 않았음을 나타냅니다.

### 3. 월렛 비밀번호 설정

월렛을 암호화할 비밀번호를 만드세요:

```
new >>> set_password my_secure_password
set_password my_secure_password
null
locked >>>
```

프롬프트가 `locked`로 변경됩니다 - 월렛이 암호화되었지만 잠겨 있습니다.

### 4. 월렛 잠금 해제

```
locked >>> unlock my_secure_password
unlock my_secure_password
null
unlocked >>>
```

이제 월렛이 잠금 해제되어 사용할 준비가 되었습니다!

## 기본 명령

### 도움말 얻기

```bash
# 모든 명령 나열
>>> help

# 특정 명령에 대한 도움말
>>> gethelp transfer
>>> gethelp create_account
```

### 월렛 정보

```bash
# 월렛이 잠겨 있는지 확인
>>> is_locked

# 월렛이 새로운지 확인
>>> is_new

# 월렛 정보 표시
>>> info

# 가져온 키 나열 (공개 키만)
>>> list_keys

# 월렛의 계정 나열
>>> list_my_accounts
```

## 키 관리

### 개인 키 가져오기

```bash
# 개인 키 가져오기 (WIF 형식)
>>> import_key 5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n
true

# 키가 가져와졌는지 확인
>>> list_keys
true

# 키가 가져와졌는지 확인
>>> list_keys
```

### 새 키 생성

```bash
# 새 키 쌍 생성 (브레인 키 방식)
>>> suggest_brain_key

# 출력:
{
  "brain_priv_key": "WORD1 WORD2 WORD3 ... WORD16",
  "wif_priv_key": "5J...",
  "pub_key": "STM..."
}
```

**중요**: `brain_priv_key`와 `wif_priv_key`를 안전하게 저장하세요!

### 브레인 키에서 키 파생

```bash
# 브레인 키에서 개인 키 얻기
>>> get_private_key_from_password account_name owner "BRAIN KEY WORDS HERE"
```

## 계정 작업

### 계정 잔액 확인

```bash
# 계정 정보 얻기
>>> get_account alice

# 출력 내용:
# - balance (유동 STEEM)
# - sbd_balance (유동 SBD)
# - vesting_shares (Steem Power)
```

### 계정 생성

제네시스 체인이나 genesis를 제어하는 테스트넷에서:

```bash
# 먼저 genesis 키 가져오기
>>> import_key 5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n

# 새 계정 생성 (테스트넷에서 무료)
>>> create_account genesis alice "" true

# 인수:
# - creator: genesis
# - new_account_name: alice
# - json_meta: "" (비어 있음)
# - broadcast: true (블록체인에 전송)
```

### 키로 계정 생성

```bash
# 먼저 키 생성
>>> suggest_brain_key
# 출력을 저장하세요!

# 특정 키로 계정 생성
>>> create_account_with_keys genesis alice "" "OWNER_KEY" "ACTIVE_KEY" "POSTING_KEY" "MEMO_KEY" true
```

### 계정 키 업데이트

```bash
# 소유자 키 업데이트
>>> update_account alice "" "NEW_OWNER_KEY" "" "" true

# 활성 키 업데이트
>>> update_account alice "" "" "NEW_ACTIVE_KEY" "" true

# 게시 키 업데이트
>>> update_account alice "" "" "" "NEW_POSTING_KEY" true
```

## 토큰 작업

### STEEM 전송

```bash
# 기본 전송
>>> transfer alice bob "10.000 STEEM" "Payment for services" true

# 인수:
# - from: alice
# - to: bob
# - amount: "10.000 STEEM" (소수점 3자리 포함)
# - memo: "Payment for services"
# - broadcast: true
```

### 저축으로 전송

```bash
# 저축으로 이동 (3일 인출)
>>> transfer_to_savings alice alice "100.000 STEEM" "Saving for later" true

# 저축에서 전송 (3일 대기 기간 시작)
>>> transfer_from_savings alice 1 "50.000 STEEM" "alice" "Withdrawal" true

# 대기 중인 저축 인출 취소
>>> cancel_transfer_from_savings alice 1 true
```

### 파워 업 (STEEM에서 Steem Power로)

```bash
# STEEM을 Steem Power로 변환 (베스팅)
>>> transfer_to_vesting alice alice "100.000 STEEM" true

# 다른 계정으로 파워 업
>>> transfer_to_vesting alice bob "50.000 STEEM" true
```

### 파워 다운 (베스팅 인출 시작)

```bash
# 13주 파워 다운 시작
>>> withdraw_vesting alice "1000.000000 VESTS" true

# 파워 다운 중지
>>> withdraw_vesting alice "0.000000 VESTS" true
```

### Steem Power 위임

```bash
# 다른 계정에 위임
>>> delegate_vesting_shares alice bob "10000.000000 VESTS" true

# 위임 제거 (0으로 설정)
>>> delegate_vesting_shares alice bob "0.000000 VESTS" true
```

### SBD를 STEEM으로 변환

```bash
# SBD를 STEEM으로 변환 (3.5일 변환)
>>> convert_sbd alice "10.000 SBD" true
```

## 콘텐츠 작업

### 콘텐츠 게시

```bash
# 게시물 생성
>>> post_comment alice "test-post" "" "parentpermlink" "My First Post" "This is the body of my post" "{}" true

# 인수:
# - author: alice
# - permlink: test-post (URL 슬러그)
# - parent_author: "" (최상위 게시물의 경우 비어 있음)
# - parent_permlink: parentpermlink (카테고리/태그)
# - title: "My First Post"
# - body: "This is the body of my post"
# - json_metadata: "{}"
# - broadcast: true
```

### 게시물에 댓글 달기

```bash
# 게시물에 답글
>>> post_comment alice "my-reply" bob "test-post" "" "Great post!" "{}" true

# 인수:
# - author: alice
# - permlink: my-reply
# - parent_author: bob
# - parent_permlink: test-post
# - title: "" (댓글의 경우 비어 있음)
# - body: "Great post!"
```

### 콘텐츠에 투표

```bash
# 업보트 (100%)
>>> vote alice bob "test-post" 100 true

# 부분 업보트 (50%)
>>> vote alice bob "test-post" 50 true

# 다운보트 (플래그)
>>> vote alice bob "test-post" -100 true

# 투표 제거
>>> vote alice bob "test-post" 0 true

# 가중치: -100에서 100 (백분율 포인트)
```

### 댓글 삭제

```bash
# 자신의 댓글/게시물 삭제
>>> delete_comment alice "test-post" true
```

## 증인 작업

### 증인 업데이트

```bash
# 서명 키 생성
>>> suggest_brain_key
# 서명 키를 저장하세요!

# 증인 속성 업데이트
>>> update_witness alice "https://witness.example.com" "STM_SIGNING_PUBLIC_KEY" {"account_creation_fee":"0.100 STEEM","maximum_block_size":65536,"sbd_interest_rate":0} true
```

### 증인에게 투표

```bash
# 증인에게 투표
>>> vote_for_witness alice bob true true

# 증인 투표 취소
>>> vote_for_witness alice bob false true
```

### 증인 투표 프록시 설정

```bash
# 프록시 설정 (증인 투표 위임)
>>> set_voting_proxy alice bob true

# 프록시 제거
>>> set_voting_proxy alice "" true
```

## 고급 작업

### 에스크로 트랜잭션

```bash
# 에스크로 생성
>>> escrow_transfer alice bob 123 eve "10.000 STEEM" "0.000 SBD" "0.100 STEEM" "2025-12-31T00:00:00" "2026-01-31T00:00:00" "{}" true

# 에스크로 승인 (에이전트)
>>> escrow_approve alice bob eve 123 true true

# 에스크로 해제 (에이전트)
>>> escrow_release alice bob eve 123 alice "10.000 STEEM" "0.000 SBD" true

# 에스크로 분쟁
>>> escrow_dispute alice bob eve 123 true
```

### 다중 서명 트랜잭션

```bash
# 다중 서명으로 계정 생성
>>> update_account alice "" "STM_KEY1" "" "" true
>>> update_account_auth_account alice active bob 1 true
>>> update_account_auth_threshold alice active 2 true

# 이제 alice는 2개의 서명이 필요합니다 (소유자 키 + bob의 키)
```

## 트랜잭션 빌드

### 커스텀 트랜잭션 빌드

```bash
# 트랜잭션 빌드 시작
>>> begin_builder_transaction

# 작업 추가
>>> add_operation_to_builder_transaction 0 ["transfer",{"from":"alice","to":"bob","amount":"1.000 STEEM","memo":"test"}]

# 트랜잭션 미리보기
>>> preview_builder_transaction 0

# 트랜잭션 서명
>>> sign_builder_transaction 0 true

# 브로드캐스트 또는 나중에 저장
```

## 쿼리 작업

### 블록체인 정보

```bash
# 블록체인 정보 얻기
>>> info

# 동적 글로벌 속성 얻기
>>> get_dynamic_global_properties

# 특정 블록 얻기
>>> get_block 1000

# 계정 얻기
>>> get_account alice

# 증인 얻기
>>> get_witness bob
```

### 계정 내역

```bash
# 계정 내역 얻기 (마지막 100개 작업)
>>> get_account_history alice -1 100

# 특정 작업 유형 얻기
>>> get_account_history alice -1 100
```

## 구성

### 월렛 파일 위치

기본값: 현재 디렉터리의 `wallet.json`

커스텀 위치 지정:
```bash
./cli_wallet -s ws://127.0.0.1:8090 -w /path/to/my_wallet.json
```

### 체인 ID

커스텀 체인의 경우, 체인 ID를 지정하세요:
```bash
./cli_wallet -s ws://127.0.0.1:8090 --chain-id=MY_CHAIN_ID
```

노드에서 체인 ID 얻기:
```bash
curl -s http://127.0.0.1:8090 -d '{"jsonrpc":"2.0","method":"database_api.get_config","params":{},"id":1}' | jq -r '.result.STEEM_CHAIN_ID'
```

## 보안 모범 사례

### 1. 월렛 비밀번호

- 강력하고 고유한 비밀번호 사용
- 월렛 비밀번호를 공유하지 마세요
- 비밀번호를 안전하게 저장 (비밀번호 관리자)

### 2. 개인 키

- 개인 키를 노출하지 마세요
- 안전한 장소에 키 백업 보관
- 다른 권한 수준(소유자/활성/게시)에 다른 키 사용

### 3. 월렛 파일

- `wallet.json`을 정기적으로 백업
- 백업 암호화
- 안전한 장소에 저장

### 4. 네트워크 보안

- 원격 연결에는 HTTPS/WSS 사용
- 노드 진위 확인
- 신뢰할 수 있는 노드만 사용

### 5. 키 계층 구조

**소유자 키**: 마스터 키, 다음에만 사용:
- 계정 복구
- 다른 키 변경
- 가능하면 오프라인 보관

**활성 키**: 금융 작업:
- 전송
- 파워 업/다운
- 변환

**게시 키**: 소셜 작업:
- 게시
- 투표
- 댓글
- 정기적으로 사용하기에 가장 안전

**메모 키**: 암호화된 메모용

## 일반적인 사용 사례

### 제네시스 노드 설정

```bash
# 제네시스 노드에 연결
./cli_wallet -s ws://127.0.0.1:8090

# 비밀번호 설정
>>> set_password mypassword
>>> unlock mypassword

# genesis 키 가져오기 (테스트넷)
>>> import_key 5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n

# 계정 생성
>>> create_account genesis alice "" true
>>> create_account genesis bob "" true

# 계정에 자금 지원
>>> transfer genesis alice "1000.000 STEEM" "Initial funding" true
>>> transfer genesis bob "1000.000 STEEM" "Initial funding" true
```

### 증인 설정

```bash
# 증인 서명 키 생성
>>> suggest_brain_key
# 키를 저장하세요!

# 활성 키 가져오기
>>> import_key YOUR_ACTIVE_PRIVATE_KEY

# 증인 업데이트
>>> update_witness myaccount "https://witness.example.com" "STM_SIGNING_PUBLIC_KEY" {"account_creation_fee":"0.100 STEEM","maximum_block_size":65536} true

# steemd config.ini에 추가:
# witness = "myaccount"
# private-key = WIF_SIGNING_PRIVATE_KEY
```

### 투표 봇

```bash
# 게시 키 가져오기
>>> import_key POSTING_PRIVATE_KEY

# 콘텐츠에 투표
>>> vote mybot author "permlink" 100 true
```

## 명령 참조

### 월렛 관리
- `set_password` - 월렛 비밀번호 설정
- `unlock` - 월렛 잠금 해제
- `lock` - 월렛 잠금
- `import_key` - 개인 키 가져오기
- `suggest_brain_key` - 새 키 쌍 생성
- `list_keys` - 가져온 공개 키 나열
- `list_my_accounts` - 월렛 계정 나열

### 계정 작업
- `create_account` - 새 계정 생성
- `create_account_with_keys` - 특정 키로 계정 생성
- `update_account` - 계정 키/메타데이터 업데이트
- `get_account` - 계정 정보 얻기

### 전송
- `transfer` - STEEM/SBD 전송
- `transfer_to_vesting` - 파워 업
- `withdraw_vesting` - 파워 다운
- `transfer_to_savings` - 저축으로 이동
- `transfer_from_savings` - 저축에서 인출
- `delegate_vesting_shares` - Steem Power 위임

### 콘텐츠
- `post_comment` - 게시 또는 댓글
- `vote` - 콘텐츠에 투표
- `delete_comment` - 댓글/게시물 삭제

### 증인
- `update_witness` - 증인 속성 업데이트
- `vote_for_witness` - 증인에게 투표
- `set_voting_proxy` - 증인 투표 프록시 설정

### 쿼리
- `info` - 월렛 정보
- `get_block` - 번호로 블록 얻기
- `get_account_history` - 계정 내역 얻기
- `get_dynamic_global_properties` - 블록체인 상태 얻기

### 고급
- `begin_builder_transaction` - 커스텀 트랜잭션 빌드 시작
- `add_operation_to_builder_transaction` - 작업 추가
- `sign_builder_transaction` - 서명 및 브로드캐스트

## 문제 해결

### 일반적인 오류

#### "Could not connect to steemd"
**문제**: 노드에 연결할 수 없음

**해결책**:
- 노드가 실행 중인지 확인: `curl http://127.0.0.1:8090`
- WebSocket 엔드포인트 확인: config.ini의 `webserver-ws-endpoint`
- 다른 포트나 주소 시도

#### "Missing Active Authority"
**문제**: 트랜잭션에 활성 키가 필요하지만 가져오지 않음

**해결책**:
```bash
>>> import_key YOUR_ACTIVE_PRIVATE_KEY
```

#### "Wallet is locked"
**문제**: 월렛을 잠금 해제해야 함

**해결책**:
```bash
>>> unlock YOUR_PASSWORD
```

#### "Assert Exception: account_by_key_api plugin not enabled"
**문제**: 노드에 필수 플러그인이 없음

**해결책**: 노드의 config.ini에 추가:
```ini
plugin = account_by_key_api
```

#### "Invalid transaction signature"
**문제**: 잘못된 개인 키 또는 체인 ID

**해결책**:
- 올바른 개인 키가 가져와졌는지 확인
- 체인 ID가 노드와 일치하는지 확인
- 월렛이 잠금 해제되었는지 확인

### 연결 문제

**문제:** `Server has disconnected us` 또는 연결 오류

**해결책:**
1. `steemd`가 실행 중이고 WebSocket 엔드포인트에 액세스할 수 있는지 확인:
   ```bash
   curl -s --data '{"jsonrpc":"2.0","method":"database_api.get_config","params":{},"id":1}' http://127.0.0.1:8090
   ```

2. 노드 로그에서 플러그인 초기화 확인
3. 방화벽이 WebSocket 연결을 허용하는지 확인

### 플러그인을 사용할 수 없음 오류

**문제:** 플러그인 또는 API 누락에 대한 RPC 오류

**해결책:**
1. 노드에 로드된 플러그인 확인:
   ```bash
   # steemd 로그에서 "Registered plugin:" 메시지 확인
   grep "Registered plugin" witness_node_data_dir/logs/*.log
   ```

2. `config.ini`에 누락된 플러그인 추가:
   ```ini
   plugin = database_api
   plugin = account_history_api
   plugin = block_api
   # ... 다른 필수 플러그인 추가
   ```

3. `steemd` 노드 재시작

### 명령이 작동하지 않음

**문제:** 특정 월렛 명령이 오류를 반환하거나 빈 결과를 반환

**가능한 원인:**
- **get_account_votes()** - 현재 빈 벡터를 반환 (알려진 제한 사항)
- **broadcast_transaction_synchronous()** - 더미 결과를 반환 (알려진 제한 사항)
- 연결된 노드에 선택적 플러그인이 없음

**해결책:**
- 계정 투표의 경우, 커스텀 쿼리를 통해 대체 API 사용
- 동기 브로드캐스트의 경우, 일반 `broadcast_transaction()`을 사용하고 확인을 폴링
- 전체 월렛 기능을 위해 권장 플러그인 활성화

### 성능 문제

**문제:** 느린 명령 응답

**해결책:**
1. 노드에 충분한 리소스(RAM, CPU, SSD)가 있는지 확인
2. 더 나은 지연 시간을 위해 원격 노드 대신 로컬 노드 사용
3. 노드 오버헤드를 줄이기 위해 필수 플러그인만 활성화
4. `get_dynamic_global_properties`로 노드 동기화 상태 확인

## 추가 리소스

- [제네시스 실행 가이드](../operations/genesis-launch.md) - 제네시스 노드 설정
- [Steem 빌드하기](building.md) - 빌드 지침
- [노드 타입 가이드](node-types-guide.md) - 다양한 노드 구성
- [월렛 API 리팩토링](../development/todo/wallet-api-refactoring.md) - 모듈식 API 마이그레이션의 기술적 세부 사항
- [프로토콜 작업](../../libraries/protocol/include/steem/protocol/) - 작업 정의

## 라이선스

[LICENSE.md](../../LICENSE.md) 참조
