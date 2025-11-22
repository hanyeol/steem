# 테스트 가이드

이 문서는 Steem 블록체인 코드베이스의 테스트에 대한 포괄적인 가이드를 제공하며, 단위 테스트, fixture, 테스트 유틸리티 및 코드 커버리지 분석을 포함합니다.

## 개요

Steem 테스트 스위트는 [Boost.Test](https://www.boost.org/doc/libs/1_74_0/libs/test/doc/html/index.html) 프레임워크를 사용하며 두 개의 주요 테스트 실행 파일로 구성됩니다:

- **`chain_test`** - 핵심 블록체인 단위 테스트
- **`plugin_test`** - 플러그인 기능 테스트

## 테스트 빌드

### 사전 요구사항

테스트에는 테스트넷 빌드 구성이 필요합니다:

```bash
cmake -DBUILD_STEEM_TESTNET=ON -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc) chain_test
make -j$(nproc) plugin_test
```

**중요**: `BUILD_STEEM_TESTNET=ON` 플래그는 테스트 빌드에 **필수**입니다. 이것이 없으면 테스트 실행 파일 빌드가 실패합니다.

### 빌드 대상

**파일**: [tests/CMakeLists.txt](../../tests/CMakeLists.txt)

- `chain_test` - `tests/tests/`의 모든 `.cpp` 파일에서 빌드
- `plugin_test` - `tests/plugin_tests/`의 모든 `.cpp` 파일에서 빌드
- `db_fixture` - 공유 테스트 fixture 라이브러리

## 테스트 실행

### 모든 테스트 실행

```bash
# 모든 블록체인 테스트 실행
./tests/chain_test

# 모든 플러그인 테스트 실행
./tests/plugin_test
```

### 특정 테스트 스위트 실행

```bash
# 특정 테스트 스위트 실행
./tests/chain_test --run_test=basic_tests
./tests/chain_test --run_test=block_tests
./tests/chain_test --run_test=operation_tests
./tests/chain_test --run_test=operation_time_tests
./tests/chain_test --run_test=serialization_tests
```

### 단일 테스트 케이스 실행

```bash
# 스위트 내의 특정 테스트 케이스 실행
./tests/chain_test --run_test=basic_tests/parse_size_test
./tests/chain_test --run_test=basic_tests/valid_name_test
./tests/chain_test --run_test=operation_tests/account_create_apply
```

### 테스트 출력 옵션

```bash
# 상세한 진행 상황 표시
./tests/chain_test --show_progress

# 로그 레벨 설정 (all, test_suite, message, warning, error, cpp_exception, system_error, fatal_error, nothing)
./tests/chain_test --log_level=all
./tests/chain_test --log_level=message

# 실행하지 않고 모든 테스트 케이스 나열
./tests/chain_test --list_content

# 커스텀 인수로 테스트 실행
./tests/chain_test --record-assert-trip  # 디버깅을 위한 assertion trip 기록
./tests/chain_test --show-test-names     # 실행 중인 테스트 이름 출력
```

## 테스트 스위트

### chain_test 스위트

**파일 위치**: [tests/tests/](../../tests/tests/)

| 테스트 스위트 | 파일 | 설명 |
|------------|------|-------------|
| `basic_tests` | [basic_tests.cpp](../../tests/tests/basic_tests.cpp) | 기본 기능 테스트 (이름 검증, 파싱, merkle 트리) |
| `block_tests` | [block_tests.cpp](../../tests/tests/block_tests.cpp) | 블록체인 및 블록 검증 테스트 |
| `live_tests` | [live_tests.cpp](../../tests/tests/live_tests.cpp) | 라이브 체인 데이터에 대한 테스트 (하드포크 테스트) |
| `operation_tests` | [operation_tests.cpp](../../tests/tests/operation_tests.cpp) | Steem operation에 대한 단위 테스트 |
| `operation_time_tests` | [operation_time_tests.cpp](../../tests/tests/operation_time_tests.cpp) | 시간 기반 operation 테스트 (예: 베스팅 출금) |
| `serialization_tests` | [serialization_tests.cpp](../../tests/tests/serialization_tests.cpp) | 직렬화 및 역직렬화 테스트 |
| `undo_tests` | [undo_tests.cpp](../../tests/tests/undo_tests.cpp) | 데이터베이스 undo/redo 기능 테스트 |
| `bmic_tests` | [bmic_tests.cpp](../../tests/tests/bmic_tests.cpp) | BMIC (Block Metadata Index Cache) 테스트 |

### plugin_test 스위트

**파일 위치**: [tests/plugin_tests/](../../tests/plugin_tests/)

- Market History Plugin Tests
- JSON-RPC Plugin Tests
- Account History Plugin Tests
- SMT Market History Tests (SMT 활성화 시)

## 테스트 Fixture

테스트 fixture는 테스트를 위한 제어된 데이터베이스 환경을 제공합니다. 모든 fixture는 [tests/db_fixture/database_fixture.hpp](../../tests/db_fixture/database_fixture.hpp)에 정의되어 있습니다.

### clean_database_fixture

가장 일반적으로 사용되는 fixture입니다. 각 테스트에 대해 새로운 블록체인 데이터베이스를 생성합니다.

**파일**: [tests/db_fixture/database_fixture.cpp](../../tests/db_fixture/database_fixture.cpp#L37)

```cpp
BOOST_FIXTURE_TEST_SUITE( operation_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( account_create_apply )
{
    // 여기에 테스트 코드 - 이 테스트를 위한 새로운 데이터베이스
}

BOOST_AUTO_TEST_SUITE_END()
```

**기능**:
- 테스트당 새로운 데이터베이스 인스턴스
- `genesis` 계정이 있는 제네시스 블록
- 최신 버전으로 설정된 하드포크
- 초기 증인 생성 및 자금 지원
- 설정 후 데이터베이스 검증
- 테스트 후 자동 정리

**설정 프로세스** (lines 37-91):
1. 필수 플러그인 등록 (account_history, debug_node, witness)
2. appbase 애플리케이션 초기화
3. 클린 데이터베이스 열기
4. 제네시스 블록 생성
5. 하드포크를 현재 버전으로 설정
6. genesis에 10,000 TESTS 베스팅
7. 추가 증인 생성 및 자금 지원

### live_database_fixture

기존 블록체인 스냅샷에 대한 테스트에 사용됩니다.

**파일**: [tests/db_fixture/database_fixture.cpp](../../tests/db_fixture/database_fixture.cpp#L153)

```cpp
BOOST_FIXTURE_TEST_SUITE( live_tests, live_database_fixture )

BOOST_AUTO_TEST_CASE( hardfork_test )
{
    // 실제 블록체인 데이터에 대한 테스트
}

BOOST_AUTO_TEST_SUITE_END()
```

**요구사항**:
- `./test_blockchain` 디렉토리에 블록체인 데이터 예상
- 주로 히스토리컬 데이터에 대한 하드포크 테스트에 사용

### json_rpc_database_fixture

JSON-RPC API 테스트에 사용됩니다.

**파일**: [tests/db_fixture/database_fixture.hpp](../../tests/db_fixture/database_fixture.hpp#L335-351)

**기능**:
- `make_request()` - JSON-RPC 요청 만들기 및 응답 검증
- `make_array_request()` - 배치 JSON-RPC 요청 만들기
- `make_positive_request()` - 성공을 기대하는 요청 만들기

## 테스트 유틸리티 및 매크로

### Actor 매크로

테스트 계정을 빠르게 생성합니다:

```cpp
// 단일 actor 생성
ACTOR(alice)  // 자동 생성된 키로 계정 "alice" 생성
ACTOR(bob)

// 한 번에 여러 actor 생성
ACTORS((alice)(bob)(carol))  // 세 계정 모두 생성

// 기존 actor 가져오기
GET_ACTOR(alice)  // 기존 "alice" 계정에 대한 참조 가져오기
```

**정의 위치**: [database_fixture.hpp](../../tests/db_fixture/database_fixture.hpp#L117-135)

**`ACTOR(name)`이 수행하는 작업**:
1. 이름에서 개인 키 생성
2. 포스팅 키 생성
3. 공개 키 도출
4. 해당 키로 계정 생성
5. `name_id` 변수에 계정 ID 저장

### Asset 매크로

```cpp
// 문자열에서 자산 생성
asset amount = ASSET("1.000 TESTS");
asset sbd_amount = ASSET("10.000 TBD");
```

**정의 위치**: [database_fixture.hpp](../../tests/db_fixture/database_fixture.hpp#L140)

### Operation 검증 매크로

데이터베이스 상태에 영향을 주지 않고 operation 검증 테스트:

```cpp
account_create_operation op;
op.creator = "alice";
op.new_account_name = "bob";

// 필드 값이 검증을 통과하는지 테스트
REQUIRE_OP_VALIDATION_SUCCESS( op, fee, ASSET("0.100 TESTS") );

// 필드 값이 검증에 실패하는지 테스트
REQUIRE_OP_VALIDATION_FAILURE( op, new_account_name, "ab" );  // 너무 짧음
REQUIRE_OP_VALIDATION_FAILURE( op, new_account_name, "" );    // 비어 있음
```

**정의 위치**: [database_fixture.hpp](../../tests/db_fixture/database_fixture.hpp#L33-96)

### 예외 테스트 매크로

```cpp
// 표현식이 특정 예외를 throw하도록 요구
STEEM_REQUIRE_THROW( db->push_transaction(tx, 0), fc::exception );

// 커스텀 값으로 throw 요구
REQUIRE_THROW_WITH_VALUE( op, fee, ASSET("99999999.999 TESTS") );
```

### 트랜잭션 매크로

```cpp
transfer_operation op;
op.from = "alice";
op.to = "bob";
op.amount = ASSET("1.000 TESTS");

// 블록체인에 operation 푸시
PUSH_OP(op, alice_private_key);

// operation을 두 번 푸시 (idempotency 테스트)
PUSH_OP_TWICE(op, alice_private_key);

// operation이 실패할 것으로 예상
FAIL_WITH_OP(op, alice_private_key, fc::assert_exception);
```

**정의 위치**: [database_fixture.hpp](../../tests/db_fixture/database_fixture.hpp#L154-179)

## 데이터베이스 Fixture 헬퍼 함수

`database_fixture` 클래스는 테스트 설정을 위한 많은 헬퍼 함수를 제공합니다.

**파일**: [tests/db_fixture/database_fixture.hpp](../../tests/db_fixture/database_fixture.hpp#L185-274)

### 계정 관리

```cpp
// 특정 매개변수로 계정 생성
const account_object& alice = account_create(
    "alice",                          // 이름
    "genesis",                      // 생성자
    init_account_priv_key,           // 생성자 키
    ASSET("0.100 TESTS").amount,     // 수수료
    alice_public_key,                // owner/active 키
    alice_post_key.get_public_key(), // 포스팅 키
    "{}"                             // json_metadata
);

// 기본 매개변수로 계정 생성
const account_object& bob = account_create("bob", bob_public_key);
```

### 자금 지원

```cpp
// 기본 금액으로 계정에 자금 지원 (500000 satoshis)
fund("alice");

// 특정 금액으로 자금 지원
fund("alice", 1000000);

// 자산으로 자금 지원
fund("alice", ASSET("100.000 TESTS"));
```

### 베스팅

```cpp
// STEEM을 베스팅하여 VESTS 획득
vest("alice", 10000);
vest("alice", ASSET("100.000 TESTS"));
```

### 전송 및 변환

```cpp
// 계정 간 전송
transfer("alice", "bob", ASSET("10.000 TESTS"));

// STEEM을 SBD로 변환
convert("alice", ASSET("100.000 TESTS"));
```

### 증인 작업

```cpp
// 증인 생성
const witness_object& wit = witness_create(
    "alice",                    // owner
    alice_private_key,          // owner 키
    "https://alice.com",        // url
    alice_public_key,           // signing 키
    ASSET("0.000 TESTS").amount // 수수료
);

// 가격 피드 설정
set_price_feed( price( ASSET("1.000 TBD"), ASSET("1.000 TESTS") ) );

// 증인 속성 설정
flat_map<string, vector<char>> props;
set_witness_props(props);
```

### 블록 생성

```cpp
// 단일 블록 생성
generate_block();

// 특정 수의 블록 생성
generate_blocks(100);  // 100개의 블록 생성

// 타임스탬프까지 블록 생성
generate_blocks(db->head_block_time() + fc::seconds(60));
```

### 데이터베이스 검증

```cpp
// 데이터베이스 불변성 검증
validate_database();  // ACTORS() 매크로에 의해 자동으로 호출
```

## 테스트 작성

### 기본 테스트 구조

```cpp
BOOST_FIXTURE_TEST_SUITE( operation_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( transfer_operation_test )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: transfer_operation" );

        // 설정
        ACTORS((alice)(bob))
        fund("alice", ASSET("100.000 TESTS"));

        // 실행
        transfer_operation op;
        op.from = "alice";
        op.to = "bob";
        op.amount = ASSET("10.000 TESTS");

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.set_expiration(db->head_block_time() + STEEM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db->get_chain_id());
        db->push_transaction(tx, 0);

        // 검증
        BOOST_REQUIRE(get_balance("alice") == ASSET("90.000 TESTS"));
        BOOST_REQUIRE(get_balance("bob") == ASSET("10.000 TESTS"));

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
```

### 테스트 모범 사례

1. **항상 테스트를 try-catch로 래핑**: `FC_LOG_AND_RETHROW()`를 사용하여 상세한 오류 정보 얻기
2. **설명적인 테스트 이름 사용**: 이름이 테스트되는 내용을 명확히 나타내야 함
3. **테스트 메시지 추가**: `BOOST_TEST_MESSAGE()`를 사용하여 테스트 단계 문서화
4. **변경 후 데이터베이스 검증**: `validate_database()`를 호출하여 일관성 보장
5. **성공 및 실패 케이스 모두 테스트**: operation이 실패해야 할 때 실패하는지 확인
6. **일반 패턴에 대한 매크로 사용**: `ACTORS()`, `PUSH_OP()` 등 활용
7. **테스트를 집중적으로 유지**: 각 테스트는 하나의 특정 동작을 검증해야 함
8. **적절히 정리**: fixture가 정리를 처리하지만 상태를 인식해야 함

### 예제: Operation 검증 테스트

```cpp
BOOST_AUTO_TEST_CASE( account_create_validate )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: account_create validation" );

        account_create_operation op;
        op.creator = "alice";
        op.fee = ASSET("0.100 TESTS");

        // 유효한 이름 테스트
        REQUIRE_OP_VALIDATION_SUCCESS(op, new_account_name, "validname");
        REQUIRE_OP_VALIDATION_SUCCESS(op, new_account_name, "valid-name");
        REQUIRE_OP_VALIDATION_SUCCESS(op, new_account_name, "valid.name");

        // 유효하지 않은 이름 테스트
        REQUIRE_OP_VALIDATION_FAILURE(op, new_account_name, "a");      // 너무 짧음
        REQUIRE_OP_VALIDATION_FAILURE(op, new_account_name, "ab");     // 너무 짧음
        REQUIRE_OP_VALIDATION_FAILURE(op, new_account_name, "INVALID"); // 대문자
        REQUIRE_OP_VALIDATION_FAILURE(op, new_account_name, "-invalid"); // 대시로 시작
    }
    FC_LOG_AND_RETHROW()
}
```

### 예제: Operation 적용 테스트

```cpp
BOOST_AUTO_TEST_CASE( account_create_apply )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: account_create application" );

        set_price_feed(price(ASSET("1.000 TBD"), ASSET("1.000 TESTS")));

        ACTOR(alice)
        fund("alice", ASSET("10.000 TESTS"));

        private_key_type bob_key = generate_private_key("bob");

        account_create_operation op;
        op.creator = "alice";
        op.new_account_name = "bob";
        op.fee = ASSET("0.100 TESTS");
        op.owner = authority(1, bob_key.get_public_key(), 1);
        op.active = authority(1, bob_key.get_public_key(), 1);
        op.posting = authority(1, bob_key.get_public_key(), 1);
        op.memo_key = bob_key.get_public_key();
        op.json_metadata = "{}";

        PUSH_OP(op, alice_private_key);

        const account_object& bob = db->get_account("bob");
        BOOST_REQUIRE(bob.name == "bob");
        BOOST_REQUIRE(bob.memo_key == bob_key.get_public_key());
        BOOST_REQUIRE(bob.created == db->head_block_time());

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}
```

### 예제: 권한 테스트

```cpp
BOOST_AUTO_TEST_CASE( transfer_authorities )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: transfer authorities" );

        transfer_operation op;
        op.from = "alice";
        op.to = "bob";
        op.amount = ASSET("1.000 TESTS");

        flat_set<account_name_type> auths;
        flat_set<account_name_type> expected;

        // Owner 권한 불필요
        op.get_required_owner_authorities(auths);
        BOOST_REQUIRE(auths == expected);

        // Active 권한 필요
        expected.insert("alice");
        auths.clear();
        op.get_required_active_authorities(auths);
        BOOST_REQUIRE(auths == expected);

        // Posting 권한 불필요
        expected.clear();
        auths.clear();
        op.get_required_posting_authorities(auths);
        BOOST_REQUIRE(auths == expected);
    }
    FC_LOG_AND_RETHROW()
}
```

## 코드 커버리지 테스트

코드 커버리지 분석은 테스트 중 실행되는 코드 부분을 보여줍니다.

### lcov 설치

```bash
# macOS
brew install lcov

# Ubuntu/Debian
sudo apt-get install lcov
```

### 커버리지 리포트 생성

```bash
# 1. 커버리지가 활성화된 빌드 구성
cmake -DBUILD_STEEM_TESTNET=ON \
      -DENABLE_COVERAGE_TESTING=ON \
      -DCMAKE_BUILD_TYPE=Debug \
      ..

# 2. 테스트 빌드
make -j$(nproc)

# 3. 기준 커버리지 캡처
lcov --capture --initial --directory . --output-file base.info --no-external

# 4. 테스트 실행
./tests/chain_test

# 5. 테스트 커버리지 캡처
lcov --capture --directory . --output-file test.info --no-external

# 6. 기준 및 테스트 커버리지 결합
lcov --add-tracefile base.info --add-tracefile test.info --output-file total.info

# 7. 커버리지 리포트에서 테스트 코드 제거
lcov -o interesting.info -r total.info 'tests/*'

# 8. HTML 리포트 생성
mkdir -p lcov
genhtml interesting.info --output-directory lcov --prefix $(pwd)

# 9. 브라우저에서 리포트 열기
open lcov/index.html  # macOS
xdg-open lcov/index.html  # Linux
```

### 커버리지 리포트 세부 사항

HTML 리포트는 다음을 보여줍니다:
- **라인 커버리지**: 실행된 라인의 백분율
- **함수 커버리지**: 호출된 함수의 백분율
- **분기 커버리지**: 취한 조건부 분기의 백분율
- 색상 코드화된 소스 파일 (녹색 = 커버됨, 빨간색 = 커버되지 않음)

### 커버리지 해석

- **높은 커버리지 (>80%)**: 좋은 테스트 커버리지
- **중간 커버리지 (50-80%)**: 테스트에 약간의 간격
- **낮은 커버리지 (<50%)**: 상당한 테스트 필요

**참고**: 100% 커버리지가 버그 없는 코드를 보장하지는 않지만, 낮은 커버리지는 테스트되지 않은 코드 경로를 나타냅니다.

## 특정 테스트 시나리오 실행

### 다양한 Skip 플래그로 테스트

```cpp
// 모든 검증 확인 건너뛰기
db->push_transaction(tx, ~0);

// 특정 확인 건너뛰기
db->push_transaction(tx, database::skip_transaction_signatures);
db->push_transaction(tx, database::skip_authority_check);
db->push_transaction(tx, database::skip_transaction_dupe_check);
```

### 시간 기반 Operation 테스트

```cpp
// 베스팅 출금 설정
withdraw_vesting_operation op;
op.account = "alice";
op.vesting_shares = ASSET("1000.000000 VESTS");
PUSH_OP(op, alice_private_key);

// 시간을 진행하기 위해 블록 생성
generate_blocks(db->head_block_time() + fc::days(7));

// 출금이 실행되었는지 확인
BOOST_REQUIRE(get_balance("alice").amount > 0);
```

### 하드포크 동작 테스트

```cpp
// 하드포크 이전 동작 테스트
db->set_hardfork(18);
// ... 테스트 코드 ...

// 하드포크 이후 동작 테스트
db->set_hardfork(19);
// ... 테스트 코드 ...
```

## 플러그인 테스트

플러그인 테스트는 플러그인 기능 및 API를 검증합니다.

### 예제: Market History Plugin 테스트

```cpp
BOOST_FIXTURE_TEST_SUITE( market_history_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( mh_test )
{
    try
    {
        // 시장 거래 설정
        ACTORS((alice)(bob))
        fund("alice", ASSET("1000.000 TESTS"));
        convert("alice", ASSET("100.000 TESTS"));

        generate_blocks(db->head_block_time() + fc::hours(1));

        // 시장 히스토리 API 쿼리
        // 히스토리컬 데이터가 올바르게 기록되었는지 확인
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
```

## 테스트 디버깅

### 디버그 출력 활성화

```bash
# 상세 로깅으로 실행
./tests/chain_test --log_level=all --run_test=operation_tests/account_create_apply

# assertion trip 기록
./tests/chain_test --record-assert-trip

# 실행 중인 테스트 이름 표시
./tests/chain_test --show-test-names
```

### GDB에서 테스트 디버그

```bash
# 디버거에서 테스트 실행
gdb --args ./tests/chain_test --run_test=operation_tests/account_create_apply

# GDB 내부
(gdb) break database_fixture.cpp:100  # 중단점 설정
(gdb) run                              # 테스트 실행
(gdb) bt                               # 중지 시 백트레이스 표시
```

### 일반적인 테스트 실패

1. **데이터베이스 검증 실패**: 일반적으로 상태 불일치를 나타냄
2. **Assertion 실패**: operation 검증 및 권한 확인
3. **자금 부족**: operation 전에 계정에 적절히 자금이 지원되었는지 확인
4. **권한 실패**: 서명에 올바른 키가 사용되었는지 확인
5. **하드포크 문제**: operation이 현재 하드포크에서 사용 가능한지 확인

## 지속적 통합

테스트는 다음과 같은 경우에 실행되어야 합니다:
- 변경 사항 커밋 전
- pull request 검증의 일부로
- 머지 후 main 브랜치에서

권장 CI 워크플로:
```bash
mkdir build && cd build
cmake -DBUILD_STEEM_TESTNET=ON -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc) chain_test plugin_test
./tests/chain_test
./tests/plugin_test
```

## 테스트 파일 구성

```
tests/
├── CMakeLists.txt                 # 테스트 빌드 구성
├── db_fixture/                    # 테스트 fixture 및 유틸리티
│   ├── database_fixture.hpp       # Fixture 클래스 정의
│   └── database_fixture.cpp       # Fixture 구현
├── tests/                         # 블록체인 단위 테스트
│   ├── main.cpp                   # 테스트 러너 진입점
│   ├── basic_tests.cpp           # 기본 기능 테스트
│   ├── block_tests.cpp           # 블록 검증 테스트
│   ├── operation_tests.cpp       # Operation 단위 테스트
│   ├── operation_time_tests.cpp  # 시간 기반 operation 테스트
│   ├── serialization_tests.cpp   # 직렬화 테스트
│   ├── undo_tests.cpp            # Undo/redo 테스트
│   ├── live_tests.cpp            # 라이브 블록체인 테스트
│   ├── smt_tests.cpp             # SMT 테스트 (활성화 시)
│   └── smt_operation_tests.cpp   # SMT operation 테스트 (활성화 시)
└── plugin_tests/                  # 플러그인별 테스트
    ├── main.cpp                   # 플러그인 테스트 러너
    ├── market_history.cpp         # Market history 플러그인 테스트
    └── json_rpc.cpp              # JSON-RPC 플러그인 테스트
```

## 관련 문서

- [Create Operation Guide](create-operation.md) - 새 operation 추가 방법
- [Plugin Development Guide](plugin.md) - 플러그인 개발 가이드
- [Boost.Test Documentation](https://www.boost.org/doc/libs/1_74_0/libs/test/doc/html/index.html) - Boost.Test 프레임워크 참조

## 요약

- 대부분의 테스트에 `clean_database_fixture` 사용
- 더 깔끔한 테스트 코드를 위해 헬퍼 매크로 (`ACTORS`, `PUSH_OP` 등) 활용
- operation의 검증 및 적용 모두 테스트
- 상태 변경 후 항상 데이터베이스 검증
- 테스트되지 않은 코드를 식별하기 위해 코드 커버리지 실행
- 설명적인 테스트 이름 및 메시지 사용
- `FC_LOG_AND_RETHROW()`를 사용하여 try-catch로 테스트 래핑
