# Asset 형식 및 표현

이 문서는 NAI(Numeric Asset Identifier) 시스템과 API 및 operation에서 사용되는 다양한 asset 형식을 포함하여 Steem 블록체인에서 asset이 표현되는 방식을 설명합니다.

## 목차

- [개요](#개요)
- [레거시 문자열 형식](#레거시-문자열-형식)
- [NAI 형식](#nai-형식)
- [핵심 Asset 유형](#핵심-asset-유형)
- [Asset 변환 예시](#asset-변환-예시)
- [코드에서 Asset 작업하기](#코드에서-asset-작업하기)
- [API 응답 형식](#api-응답-형식)
- [모범 사례](#모범-사례)

## 개요

Steem은 asset을 표현하기 위해 두 가지 주요 형식을 사용합니다:

1. **레거시 문자열 형식**: `"1.000 STEEM"`과 같은 사람이 읽을 수 있는 형식
2. **NAI 형식**: 숫자 asset 식별자를 사용하는 구조화된 JSON 형식

NAI(Numeric Asset Identifier) 시스템은 Smart Media Token(SMT)을 지원하기 위해 도입되었으며 블록체인의 다양한 asset 유형을 표현하는 더 확장 가능한 방법을 제공합니다.

## 레거시 문자열 형식

초기 Steem 구현에서 사용되었으며 하위 호환성을 위해 여전히 지원되는 전통적인 형식입니다.

### 형식

```
<amount> <symbol>
```

### 예시

```
"1.000 STEEM"
"10.000 TBD"
"1000.000000 VESTS"
```

### 규칙

- Amount는 소수점을 포함해야 함
- 정밀도는 asset 유형과 일치해야 함:
  - STEEM/TESTS: 소수점 3자리
  - SBD/TBD: 소수점 3자리
  - VESTS: 소수점 6자리
- Amount와 symbol 사이에 공백 필요
- Symbol은 대문자여야 함

### CLI Wallet에서 사용

```bash
# 레거시 형식을 사용한 전송
transfer alice bob "10.000 TESTS" "memo" true

# STEEM을 SBD로 변환
convert alice "100.000 TESTS"
```

## NAI 형식

API 응답 및 내부 표현에 사용되는 최신 구조화 형식입니다.

### 구조

```json
{
  "amount": "1000",
  "precision": 3,
  "nai": "@@000000021"
}
```

### 필드

| 필드 | 타입 | 설명 |
|-------|------|-------------|
| `amount` | string | 소수점 없이 최소 단위(satoshis)의 금액 |
| `precision` | integer | 소수점 자릿수 |
| `nai` | string | `@@`로 시작하는 Numeric Asset Identifier |

### 예시 표현

**1.000 STEEM:**
```json
{
  "amount": "1000",
  "precision": 3,
  "nai": "@@000000021"
}
```

**10.000 TBD:**
```json
{
  "amount": "10000",
  "precision": 3,
  "nai": "@@000000013"
}
```

**1000.000000 VESTS:**
```json
{
  "amount": "1000000000",
  "precision": 6,
  "nai": "@@000000037"
}
```

## 핵심 Asset 유형

### STEEM (테스트넷에서 TESTS)

Steem 블록체인의 주요 유동 토큰입니다.

**NAI:** `@@000000021`
**정밀도:** 3
**심볼:** STEEM (메인넷) / TESTS (테스트넷)

**예시:**
```json
{
  "amount": "250000000000",
  "precision": 3,
  "nai": "@@000000021"
}
```
표현: `250000000.000 STEEM`

### SBD (테스트넷에서 TBD)

Steem Backed Dollars - USD에 페그된 부채 수단입니다.

**NAI:** `@@000000013`
**정밀도:** 3
**심볼:** SBD (메인넷) / TBD (테스트넷)

**예시:**
```json
{
  "amount": "1000",
  "precision": 3,
  "nai": "@@000000013"
}
```
표현: `1.000 SBD`

### VESTS

Steem Power(잠긴/스테이킹된 STEEM)를 나타내는 vesting shares입니다.

**NAI:** `@@000000037`
**정밀도:** 6
**심볼:** VESTS

**예시:**
```json
{
  "amount": "1000000000",
  "precision": 6,
  "nai": "@@000000037"
}
```
표현: `1000.000000 VESTS`

## Asset 변환 예시

### 문자열을 NAI 형식으로 변환

**입력:** `"123.456 STEEM"`

**프로세스:**
1. amount 파싱: `123.456`
2. 정밀도 결정: `3`
3. satoshis로 변환: `123.456 * 1000 = 123456`
4. STEEM에 대한 NAI 조회: `@@000000021`

**출력:**
```json
{
  "amount": "123456",
  "precision": 3,
  "nai": "@@000000021"
}
```

### NAI를 문자열 형식으로 변환

**입력:**
```json
{
  "amount": "5000000",
  "precision": 6,
  "nai": "@@000000037"
}
```

**프로세스:**
1. amount 파싱: `5000000`
2. 정밀도 적용: `5000000 / 10^6 = 5.000000`
3. NAI `@@000000037`에 대한 symbol 조회: `VESTS`

**출력:** `"5.000000 VESTS"`

### 정밀도 계산

`amount` 필드는 최소 단위(satoshis)로 값을 저장합니다:

```
display_amount = amount / (10 ^ precision)
```

**예시:**

| Amount (satoshis) | 정밀도 | 표시 값 |
|-------------------|-----------|---------------|
| 1000 | 3 | 1.000 |
| 123456 | 3 | 123.456 |
| 1000000 | 6 | 1.000000 |
| 5000000000 | 6 | 5000.000000 |

## 코드에서 Asset 작업하기

### C++ Asset 클래스

**파일:** `src/core/protocol/include/steem/protocol/asset.hpp`

```cpp
// asset 생성
asset steem_amount = asset(1000, STEEM_SYMBOL);  // 1.000 STEEM

// 구성 요소 접근
int64_t amount = steem_amount.amount;  // 1000
asset_symbol_type symbol = steem_amount.symbol;

// 문자열 표현
std::string str = steem_amount.to_string();  // "1.000 STEEM"
```

### Asset Symbol 타입

```cpp
struct asset_symbol_type
{
    uint8_t  decimals;  // 정밀도
    uint32_t nai;       // Numeric asset identifier

    // 미리 정의된 symbol
    static const asset_symbol_type STEEM_SYMBOL;  // 3 decimals, NAI 21
    static const asset_symbol_type SBD_SYMBOL;    // 3 decimals, NAI 13
    static const asset_symbol_type VESTS_SYMBOL;  // 6 decimals, NAI 37
};
```

### 테스트에서 Asset 생성

```cpp
// ASSET 매크로 사용
asset amount = ASSET("10.000 TESTS");
asset sbd = ASSET("5.000 TBD");
asset vests = ASSET("1000.000000 VESTS");

// 수동 생성
asset steem = asset(10000, STEEM_SYMBOL);  // 10.000 STEEM
```

### JSON 직렬화

Asset은 컨텍스트에 따라 다르게 직렬화됩니다:

**문자열 형식:**
```json
"balance": "1.000 STEEM"
```

**객체 형식 (database_api):**
```json
"balance": {
  "amount": "1000",
  "precision": 3,
  "nai": "@@000000021"
}
```

## API 응답 형식

### database_api.find_accounts

NAI 형식으로 asset을 반환합니다:

```json
{
  "accounts": [{
    "name": "alice",
    "balance": {
      "amount": "1000",
      "precision": 3,
      "nai": "@@000000021"
    },
    "sbd_balance": {
      "amount": "500",
      "precision": 3,
      "nai": "@@000000013"
    },
    "vesting_shares": {
      "amount": "1000000000",
      "precision": 6,
      "nai": "@@000000037"
    }
  }]
}
```

### Operation 직렬화

Operation은 사람이 읽을 수 있도록 레거시 문자열 형식을 사용합니다:

```json
{
  "type": "transfer_operation",
  "value": {
    "from": "alice",
    "to": "bob",
    "amount": "10.000 STEEM",
    "memo": "payment"
  }
}
```

### Block API

Block 내용은 특정 API 호출에 따라 두 형식을 모두 포함할 수 있습니다.

## 모범 사례

### 각 형식을 사용해야 하는 경우

**레거시 문자열 형식 사용:**
- CLI wallet 명령
- 사용자 대면 디스플레이
- Operation 생성
- 사람이 읽을 수 있는 로그

**NAI 형식 사용:**
- API 응답 (최신 API)
- 내부 계산
- 데이터베이스 저장
- Asset 간 비교

### 검증

Asset을 처리하기 전에 항상 asset 형식을 검증하세요:

```cpp
// 정밀도가 asset 유형과 일치하는지 검증
if (amount.symbol.decimals() != 3) {
    throw invalid_asset_exception();
}

// Amount가 음수가 아닌지 검증 (명시적으로 허용되지 않는 한)
if (amount.amount < 0) {
    throw negative_amount_exception();
}

// Symbol이 예상 유형과 일치하는지 검증
if (amount.symbol != STEEM_SYMBOL) {
    throw wrong_asset_type_exception();
}
```

### 정밀도 처리

**변환 시 항상 정밀도 유지:**

```cpp
// 잘못됨 - 정밀도 손실
int64_t steem = asset.amount / 1000;

// 올바름 - 정밀도 유지
asset steem = asset(amount_satoshis, STEEM_SYMBOL);
```

**부동소수점 연산 피하기:**

```cpp
// 잘못됨 - 부동소수점 오류
double value = 10.000;
asset a = asset(value * 1000, STEEM_SYMBOL);

// 올바름 - 정수 연산
asset a = asset(10000, STEEM_SYMBOL);
```

### 문자열 파싱

Asset 문자열을 파싱할 때:

1. 공백으로 분할하여 amount와 symbol 분리
2. Symbol이 인식되는지 검증
3. 정확한 정밀도로 amount 파싱
4. 최소 단위(satoshis)로 변환
5. 결과가 유효한 범위 내에 있는지 검증

```cpp
// 파싱 예시
std::string input = "10.000 STEEM";
auto space_pos = input.find(' ');
std::string amount_str = input.substr(0, space_pos);
std::string symbol_str = input.substr(space_pos + 1);

// 검증 및 변환
asset result = asset::from_string(input);
```

## SMT 및 커스텀 Asset

SMT(Smart Media Tokens)가 활성화되면 커스텀 NAI로 추가 asset 유형을 생성할 수 있습니다:

### 커스텀 NAI 범위

- 핵심 asset: `@@000000001` - `@@000000099`
- SMT asset: `@@000000100` - `@@999999999`

### SMT Asset 예시

```json
{
  "amount": "1000000",
  "precision": 4,
  "nai": "@@000000123"
}
```

### SMT Asset 생성

SMT asset은 특별한 operation을 통해 생성되고 온체인에 등록됩니다. 각 SMT는:
- 고유한 NAI를 가짐
- 자체 정밀도 정의 (0-12 소수점)
- 구성 가능한 공급 및 발행 규칙을 가짐

## 관련 파일

- **Asset 정의:** `src/core/protocol/include/steem/protocol/asset.hpp`
- **Asset 구현:** `src/core/protocol/asset.cpp`
- **Asset Symbol:** `src/core/protocol/include/steem/protocol/asset_symbol.hpp`
- **직렬화:** `src/core/protocol/include/steem/protocol/types.hpp`

## 추가 리소스

- [Chain Parameters](chain-parameters.md) - 블록체인 매개변수 참조
- [API Notes](api-notes.md) - API 사용 가이드라인
- [System Architecture](system-architecture.md) - 전체 시스템 설계

## 요약

- **NAI 형식**: amount(satoshis), 정밀도 및 숫자 식별자가 포함된 구조화된 JSON
- **레거시 형식**: "1.000 STEEM"과 같은 사람이 읽을 수 있는 문자열
- **핵심 Asset**: STEEM (NAI 21), SBD (NAI 13), VESTS (NAI 37)
- **정밀도**: 최소 단위에서 항상 정수 연산 사용
- **API 호환성**: 최신 API는 NAI 형식 사용, 레거시 API는 문자열 형식 사용
- **모범 사례**: Asset을 처리하기 전에 정밀도와 symbol 검증
