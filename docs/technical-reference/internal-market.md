# 내부 시장(Internal Market) 가이드

## 개요

Steem 블록체인은 탈중앙화된 내부 거래소(Internal Market)를 제공하여 사용자가 STEEM과 SBD(Steem Backed Dollar) 토큰을 직접 교환할 수 있습니다. 이 문서는 내부 시장의 작동 원리, 사용 방법, API, 그리고 구현 세부사항을 다룹니다.

## 목차

1. [지원 거래쌍](#지원-거래쌍)
2. [주문 생성 및 취소](#주문-생성-및-취소)
3. [주문 매칭 메커니즘](#주문-매칭-메커니즘)
4. [유동성 보상](#유동성-보상)
5. [Market History API](#market-history-api)
6. [제약사항 및 제한](#제약사항-및-제한)
7. [구현 세부사항](#구현-세부사항)

## 지원 거래쌍

내부 시장은 다음 거래쌍을 지원합니다:

### 기본 거래쌍
- **STEEM/SBD**: 메인 거래쌍
  - STEEM: 기본 암호화폐 토큰
  - SBD: 달러 페그 토큰 (Steem Backed Dollar)

### SMT 거래쌍 (SMT 활성화 시)
- **SMT/STEEM**: 스마트 미디어 토큰과 STEEM
- **SMT/SBD**: 스마트 미디어 토큰과 SBD

> **참고**: 모든 거래는 STEEM 또는 SBD와 페어를 이루어야 합니다. 직접적인 SMT 간 거래는 지원되지 않습니다.

## 주문 생성 및 취소

### 제한가 주문 생성

내부 시장은 **제한가 주문(Limit Order)** 방식을 사용합니다. 사용자는 원하는 가격을 지정하여 주문을 생성하고, 매칭 엔진이 조건에 맞는 주문을 자동으로 체결합니다.

#### limit_order_create_operation

```cpp
{
  "owner": "alice",              // 주문 소유자 계정
  "orderid": 123,                // 계정별 고유 주문 ID (uint32)
  "amount_to_sell": "100.000 STEEM",    // 판매할 자산
  "min_to_receive": "150.000 SBD",      // 최소 수령 자산
  "fill_or_kill": false,         // 즉시 체결 또는 취소 여부
  "expiration": "2025-12-31T23:59:59"   // 주문 만료 시간
}
```

**매개변수 설명**:

- **owner**: 주문을 생성하는 계정 (active 권한 필요)
- **orderid**: 사용자가 지정하는 고유 주문 ID. 같은 계정 내에서 중복되지 않아야 합니다.
- **amount_to_sell**: 판매하려는 자산의 양과 심볼
- **min_to_receive**: 받고자 하는 최소 자산의 양과 심볼
- **fill_or_kill**:
  - `true`: 주문 생성 시 즉시 완전히 체결되지 않으면 취소
  - `false`: 부분 체결 허용, 남은 주문은 주문장에 보관
- **expiration**: 주문 만료 시간
  - 현재 시각 이후여야 함
  - 최대 28일 (`STEEM_MAX_LIMIT_ORDER_EXPIRATION`)
  - 기본값: 최대 시간 (사실상 만료 없음)

**가격 계산**:

주문의 **가격**은 다음과 같이 계산됩니다:

```
가격 = amount_to_sell / min_to_receive
```

예시:
- `amount_to_sell`: 100 STEEM
- `min_to_receive`: 150 SBD
- 가격: 100/150 = 0.6667 STEEM/SBD (또는 1.5 SBD/STEEM)

#### limit_order_create2_operation

`limit_order_create2_operation`은 가격을 더 명시적으로 지정할 수 있는 변형입니다:

```cpp
{
  "owner": "alice",
  "orderid": 124,
  "amount_to_sell": "100.000 STEEM",
  "fill_or_kill": false,
  "exchange_rate": {             // 명시적 가격 객체
    "base": "3.000 SBD",
    "quote": "2.000 STEEM"
  },
  "expiration": "2025-12-31T23:59:59"
}
```

**price 객체**:
- `base / quote`로 가격 계산
- 위 예시: 3 SBD / 2 STEEM = 1.5 SBD/STEEM

### 주문 취소

주문 소유자는 언제든지 미체결 주문을 취소할 수 있습니다.

#### limit_order_cancel_operation

```cpp
{
  "owner": "alice",
  "orderid": 123
}
```

- 주문이 취소되면 판매하려던 자산이 계정으로 즉시 반환됩니다.
- 이미 부분 체결된 경우, 체결되지 않은 잔여 수량만 반환됩니다.

### 주문 체결 알림 (Virtual Operation)

주문이 매칭되어 체결되면 블록체인에 `fill_order_operation` 가상 operation이 기록됩니다.

#### fill_order_operation

```cpp
{
  "current_owner": "alice",       // 새 주문 소유자
  "current_orderid": 123,
  "current_pays": "50.000 STEEM", // 새 주문이 지불한 금액
  "open_owner": "bob",            // 기존 주문 소유자
  "open_orderid": 456,
  "open_pays": "75.000 SBD"       // 기존 주문이 지불한 금액
}
```

- 이는 사용자가 직접 제출하지 않는 **가상 operation**입니다.
- `market_history` 플러그인이 이를 감지하여 시장 데이터를 업데이트합니다.
- Alice는 50 STEEM을 지불하고 75 SBD를 받습니다.
- Bob은 75 SBD를 지불하고 50 STEEM을 받습니다.

## 주문 매칭 메커니즘

Steem의 내부 시장은 **Price-Time Priority** 알고리즘을 사용합니다:

1. **가격 우선**: 더 좋은 가격의 주문이 먼저 체결
2. **시간 우선**: 같은 가격이면 먼저 생성된 주문이 먼저 체결

### 매칭 프로세스

새 주문이 생성되면 다음 순서로 처리됩니다:

1. **주문 검증**
   - 거래쌍 유효성 확인 (STEEM/SBD 또는 SMT 페어)
   - 계정 잔고 확인
   - 만료 시간 검증

2. **즉시 매칭 시도**
   - 반대편 주문장에서 매칭 가능한 주문 검색
   - 가격 조건을 만족하는 주문부터 순차적으로 체결
   - `fill_order_operation` 생성

3. **잔여 수량 처리**
   - 완전히 체결된 경우: 주문 삭제
   - 부분 체결된 경우:
     - `fill_or_kill = true`: 주문 취소, 잔여 수량 반환
     - `fill_or_kill = false`: 주문장에 보관

4. **유동성 보상 추적** (해당하는 경우)

### 매칭 예시

**시나리오**: Alice가 STEEM을 SBD로 교환하고 싶음

**주문장 상태**:

| 소유자 | 유형 | 판매 자산 | 수령 자산 | 가격 (SBD/STEEM) |
|--------|------|-----------|-----------|------------------|
| Bob    | 매수 | 150 SBD   | 100 STEEM | 1.5              |
| Carol  | 매수 | 140 SBD   | 100 STEEM | 1.4              |

**Alice의 주문**:
- 판매: 80 STEEM
- 최소 수령: 112 SBD
- 가격: 1.4 SBD/STEEM

**매칭 결과**:

1. Bob의 주문과 매칭 (더 좋은 가격)
   - Alice 지불: 80 STEEM
   - Alice 수령: 120 SBD (80 × 1.5)
   - Bob 지불: 120 SBD
   - Bob 수령: 80 STEEM
   - Bob의 잔여 주문: 30 SBD로 20 STEEM 매수 (1.5 SBD/STEEM)

2. Alice의 주문 완전 체결
   - 주문장에서 제거됨

### 가격 결정

매칭 시 사용되는 가격은 **기존 주문(maker)의 가격**입니다:

- Maker (주문장에 먼저 있던 주문): 가격 설정자
- Taker (새로 들어온 주문): 가격 수용자

이는 먼저 주문을 넣은 사람에게 유리한 가격을 보장합니다.

### 부분 체결

주문이 여러 개의 기존 주문과 매칭될 수 있습니다:

```
Alice 주문: 200 STEEM 판매, 280 SBD 이상 수령 (1.4 SBD/STEEM)

주문장:
- Bob:   50 SBD로 30 STEEM 매수 (1.67 SBD/STEEM)
- Carol: 100 SBD로 60 STEEM 매수 (1.67 SBD/STEEM)
- Dave:  150 SBD로 100 STEEM 매수 (1.5 SBD/STEEM)

체결 과정:
1. Bob과 매칭:   Alice 30 STEEM → 50 SBD
2. Carol과 매칭: Alice 60 STEEM → 100 SBD
3. Dave와 매칭:  Alice 100 STEEM → 150 SBD
4. Alice 총 체결: 190 STEEM → 300 SBD
5. 잔여 주문: 10 STEEM을 14 SBD 이상에 판매 (주문장에 보관)
```

## 유동성 보상

Steem은 내부 시장에 유동성을 제공하는 사용자에게 보상을 지급합니다.

### 보상 메커니즘

1. **거래량 추적**
   - 각 계정의 STEEM 거래량 및 SBD 거래량 추적
   - 최근 7일간의 거래량만 계산 (`STEEM_LIQUIDITY_TIMEOUT_SEC`)

2. **가중치 계산**
   ```
   가중치 = steem_volume × sbd_volume
   ```
   - 양쪽 시장에서 모두 거래해야 보상 자격 획득
   - 균형 잡힌 거래를 장려

3. **보상 지급**
   - 1시간마다 지급 (`STEEM_LIQUIDITY_REWARD_PERIOD_SEC`)
   - 전체 가중치 대비 개인 가중치 비율로 분배
   - 연간 7.5% APR (`STEEM_LIQUIDITY_APR_PERCENT`)

### 유동성 보상 상수

| 상수 | 값 | 설명 |
|------|-----|------|
| `STEEM_LIQUIDITY_TIMEOUT_SEC` | 604,800 (7일) | 거래량 리셋 주기 |
| `STEEM_MIN_LIQUIDITY_REWARD_PERIOD_SEC` | 60 (1분) | 주문 최소 유지 시간 |
| `STEEM_LIQUIDITY_REWARD_PERIOD_SEC` | 3,600 (1시간) | 보상 지급 주기 |
| `STEEM_LIQUIDITY_APR_PERCENT` | 750 (7.5%) | 연간 수익률 |

### 보상 자격 조건

- 주문을 최소 1분 이상 유지
- STEEM 및 SBD 양쪽 시장에서 모두 거래
- 주문이 체결되어 실제 거래량 발생

## Market History API

`market_history_api` 플러그인은 내부 시장 데이터를 조회하는 다양한 API를 제공합니다.

### 플러그인 활성화

**config.ini**:
```ini
# 상태 추적 플러그인 (필수)
plugin = market_history

# API 플러그인 (API 노출용)
plugin = market_history_api

# 버킷 설정
market-history-bucket-size = [15,60,300,3600,86400]
market-history-buckets-per-size = 5760
```

### API 메서드

#### 1. get_ticker

현재 시세 정보를 조회합니다.

**요청**:
```json
{}
```

**응답**:
```json
{
  "latest": "1.523",              // 최근 거래 가격
  "lowest_ask": "1.530",          // 최저 매도 호가
  "highest_bid": "1.520",         // 최고 매수 호가
  "percent_change": "2.15",       // 24시간 변동률
  "steem_volume": "12345.678",    // 24시간 STEEM 거래량
  "sbd_volume": "18765.432"       // 24시간 SBD 거래량
}
```

#### 2. get_volume

24시간 거래량을 조회합니다.

**요청**:
```json
{}
```

**응답**:
```json
{
  "steem_volume": "12345.678",
  "sbd_volume": "18765.432"
}
```

#### 3. get_order_book

현재 주문장(Order Book)을 조회합니다.

**요청**:
```json
{
  "limit": 100  // 최대 500
}
```

**응답**:
```json
{
  "bids": [  // 매수 호가 (높은 가격부터)
    {
      "order_price": {
        "base": "1.520 SBD",
        "quote": "1.000 STEEM"
      },
      "real_price": 1.520,
      "steem": 100,
      "sbd": 152,
      "created": "2025-11-12T10:30:00"
    },
    ...
  ],
  "asks": [  // 매도 호가 (낮은 가격부터)
    {
      "order_price": {
        "base": "1.000 STEEM",
        "quote": "0.653 SBD"
      },
      "real_price": 1.531,
      "steem": 100,
      "sbd": 65.3,
      "created": "2025-11-12T10:25:00"
    },
    ...
  ]
}
```

**필드 설명**:
- **bids**: SBD로 STEEM을 구매하는 주문들
- **asks**: STEEM으로 SBD를 구매하는 주문들
- **order_price**: 주문 가격 (price 객체)
- **real_price**: 실제 가격 (double)
- **steem/sbd**: 거래 수량
- **created**: 주문 생성 시간

#### 4. get_trade_history

특정 시간 범위의 거래 내역을 조회합니다.

**요청**:
```json
{
  "start": "2025-11-12T00:00:00",
  "end": "2025-11-12T23:59:59",
  "limit": 1000  // 최대 1000
}
```

**응답**:
```json
[
  {
    "date": "2025-11-12T10:30:15",
    "current_pays": "50.000 STEEM",
    "open_pays": "76.150 SBD"
  },
  ...
]
```

#### 5. get_recent_trades

최근 거래 내역을 조회합니다.

**요청**:
```json
{
  "limit": 100  // 최대 1000
}
```

**응답**: `get_trade_history`와 동일

#### 6. get_market_history

OHLCV(Open-High-Low-Close-Volume) 버킷 데이터를 조회합니다.

**요청**:
```json
{
  "bucket_seconds": 300,  // 버킷 크기 (초)
  "start": "2025-11-12T00:00:00",
  "end": "2025-11-12T23:59:59"
}
```

**응답**:
```json
[
  {
    "id": 0,
    "open": "2025-11-12T10:00:00",
    "seconds": 300,
    "high_steem": 100,
    "high_sbd": 153,
    "low_steem": 100,
    "low_sbd": 150,
    "open_steem": 100,
    "open_sbd": 152,
    "close_steem": 100,
    "close_sbd": 151,
    "steem_volume": 5000,
    "sbd_volume": 7600
  },
  ...
]
```

**버킷 크기**:
- 15: 15초 (틱 데이터)
- 60: 1분
- 300: 5분
- 3600: 1시간
- 86400: 1일

#### 7. get_market_history_buckets

사용 가능한 버킷 크기 목록을 조회합니다.

**요청**:
```json
{}
```

**응답**:
```json
[15, 60, 300, 3600, 86400]
```

### API 호출 예시 (curl)

```bash
# Ticker 조회
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"market_history_api.get_ticker",
  "params":{},
  "id":1
}' https://api.steemit.com

# 주문장 조회
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"market_history_api.get_order_book",
  "params":{"limit":10},
  "id":1
}' https://api.steemit.com

# 최근 거래 조회
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"market_history_api.get_recent_trades",
  "params":{"limit":20},
  "id":1
}' https://api.steemit.com
```

## 제약사항 및 제한

### 주문 제약

| 제약사항 | 값 | 설명 |
|---------|-----|------|
| 최대 만료 기간 | 28일 | `STEEM_MAX_LIMIT_ORDER_EXPIRATION` |
| 필요 권한 | active | 주문 생성/취소에 필요 |
| 거래쌍 | STEEM/SBD, SMT/STEEM | 허용되는 거래쌍만 가능 |
| orderid | uint32_t (0-4,294,967,295) | 계정별 고유 ID |
| 최소 거래량 | 0.001 | 자산의 정밀도에 따름 |

### API 제한

| API | 최대 limit | 설명 |
|-----|-----------|------|
| `get_order_book` | 500 | 주문장 조회 개수 |
| `get_trade_history` | 1000 | 거래 내역 조회 개수 |
| `get_recent_trades` | 1000 | 최근 거래 조회 개수 |
| `get_market_history` | - | 시간 범위로 제한 |

### 성능 고려사항

- **읽기 락 만료**: API 호출은 1초 이내에 완료되어야 합니다.
- **대량 조회**: 큰 limit 값은 응답 시간을 증가시킵니다.
- **버킷 보관**: 기본적으로 각 버킷 크기당 5760개까지 보관됩니다.

## 구현 세부사항

### 데이터베이스 객체

#### limit_order_object

**파일**: [libraries/chain/include/steem/chain/steem_objects.hpp:185](libraries/chain/include/steem/chain/steem_objects.hpp#L185)

```cpp
class limit_order_object : public object<limit_order_object_type, limit_order_object>
{
public:
   id_type           id;
   time_point_sec    created;
   time_point_sec    expiration;
   account_name_type seller;
   uint32_t          orderid = 0;
   share_type        for_sale;     // 판매 수량
   price             sell_price;   // 판매 가격

   // 헬퍼 메서드
   asset amount_for_sale() const
   {
      return asset(for_sale, sell_price.base.symbol);
   }

   asset amount_to_receive() const
   {
      return amount_for_sale() * sell_price;
   }
};
```

**인덱스**:
- `by_id`: 주문 ID로 검색
- `by_expiration`: 만료 시간 순 정렬
- `by_price`: 가격 순 정렬 (매칭용)
- `by_account`: 계정과 orderid로 검색

#### bucket_object

**파일**: [libraries/plugins/market_history/include/steem/plugins/market_history/market_history_plugin.hpp:85](libraries/plugins/market_history/include/steem/plugins/market_history/market_history_plugin.hpp#L85)

```cpp
struct bucket_object
{
   id_type            id;
   fc::time_point_sec open;      // 버킷 시작 시간
   uint32_t           seconds;   // 버킷 크기

   // STEEM 측 OHLCV
   share_type         high_steem;
   share_type         low_steem;
   share_type         open_steem;
   share_type         close_steem;
   share_type         steem_volume;

   // SBD 측 OHLCV
   share_type         high_sbd;
   share_type         low_sbd;
   share_type         open_sbd;
   share_type         close_sbd;
   share_type         sbd_volume;
};
```

### 핵심 알고리즘

#### 주문 매칭 (apply_order)

**파일**: [libraries/chain/database.cpp:3401](libraries/chain/database.cpp#L3401)

```cpp
bool database::apply_order(const limit_order_object& new_order_object)
{
   auto order_id = new_order_object.id;
   const auto& limit_price_idx = get_index<limit_order_index>()
      .indices().get<by_price>();

   // 매칭 가능한 가격 범위 찾기
   auto max_price = ~new_order_object.sell_price;
   auto limit_itr = limit_price_idx.lower_bound(max_price.max());
   auto limit_end = limit_price_idx.upper_bound(max_price);

   // 순차적으로 매칭 시도
   bool finished = false;
   while (!finished && limit_itr != limit_end)
   {
      auto old_limit_itr = limit_itr;
      ++limit_itr;

      // 주문 매칭 (0x1 반환 시 새 주문 완전 체결)
      finished = (match(new_order_object, *old_limit_itr,
                       old_limit_itr->sell_price) & 0x1);
   }

   // 완전 체결 여부 반환
   return find<limit_order_object>(order_id) == nullptr;
}
```

#### 주문 체결 (match)

**파일**: [libraries/chain/database.cpp:3423](libraries/chain/database.cpp#L3423)

```cpp
int database::match(const limit_order_object& new_order,
                    const limit_order_object& old_order,
                    const price& match_price)
{
   // 거래쌍 검증
   STEEM_ASSERT(
      new_order.sell_price.quote.symbol == old_order.sell_price.base.symbol,
      ...
   );

   // 체결 수량 계산
   asset new_order_pays, new_order_receives;
   asset old_order_pays, old_order_receives;

   if (new_order_for_sale <= old_order_for_sale * match_price)
   {
      // 새 주문이 완전 체결
      old_order_receives = new_order_for_sale;
      new_order_receives = new_order_for_sale * match_price;
   }
   else
   {
      // 기존 주문이 완전 체결
      new_order_receives = old_order_for_sale;
      old_order_receives = old_order_for_sale * match_price;
   }

   // 가상 operation 생성
   push_virtual_operation(fill_order_operation(
      new_order.seller, new_order.orderid, new_order_pays,
      old_order.seller, old_order.orderid, old_order_pays
   ));

   // 유동성 보상 추적
   adjust_liquidity_reward(...);

   // 각 주문 체결 처리
   int result = 0;
   result |= fill_order(new_order, new_order_pays, new_order_receives);
   result |= fill_order(old_order, old_order_pays, old_order_receives) << 1;

   return result;
}
```

### 파일 경로 참조

#### Operations & Protocol
- Operations 정의: [libraries/protocol/include/steem/protocol/steem_operations.hpp:632](libraries/protocol/include/steem/protocol/steem_operations.hpp#L632)
- Virtual Operations: [libraries/protocol/include/steem/protocol/steem_virtual_operations.hpp:103](libraries/protocol/include/steem/protocol/steem_virtual_operations.hpp#L103)
- Operations 구현: [libraries/protocol/steem_operations.cpp:337](libraries/protocol/steem_operations.cpp#L337)

#### Chain & Database
- Evaluators: [libraries/chain/steem_evaluator.cpp:1514](libraries/chain/steem_evaluator.cpp#L1514)
- Database 객체: [libraries/chain/include/steem/chain/steem_objects.hpp:185](libraries/chain/include/steem/chain/steem_objects.hpp#L185)
- Database 로직: [libraries/chain/database.cpp:3401](libraries/chain/database.cpp#L3401)

#### Plugins
- market_history 플러그인: [libraries/plugins/market_history/market_history_plugin.cpp](libraries/plugins/market_history/market_history_plugin.cpp)
- market_history_api: [libraries/plugins/apis/market_history_api/market_history_api.cpp](libraries/plugins/apis/market_history_api/market_history_api.cpp)

#### Tests
- Operation 테스트: [tests/tests/operation_tests.cpp](tests/tests/operation_tests.cpp)
- Market History 테스트: [tests/plugin_tests/market_history.cpp](tests/plugin_tests/market_history.cpp)

## 사용 예시

### CLI Wallet으로 주문 생성

```bash
# CLI Wallet 연결
./cli_wallet -s wss://api.steemit.com

# 주문 생성: 100 STEEM을 150 SBD 이상에 판매
sell alice 100.000 STEEM 150.000 SBD false 123

# 주문 취소
cancel_order alice 123

# 주문장 조회
get_order_book 10
```

### Python으로 주문 생성

```python
from beem import Steem
from beem.account import Account
from beem.market import Market

# Steem 연결
steem = Steem()

# 계정 로드 (active key 필요)
account = Account("alice", steem_instance=steem)

# 시장 생성
market = Market(base="STEEM", quote="SBD", steem_instance=steem)

# 주문 생성: 100 STEEM을 1.5 SBD/STEEM에 판매
market.sell(
    price=1.5,          # SBD/STEEM
    amount=100,         # STEEM
    account=account,
    orderid=123
)

# 주문 취소
market.cancel(orderid=123, account=account)

# 주문장 조회
order_book = market.orderbook(limit=10)
print(order_book)
```

### JavaScript로 API 조회

```javascript
const dhive = require('@hiveio/dhive');

// API 클라이언트 생성
const client = new dhive.Client([
  'https://api.steemit.com',
]);

// Ticker 조회
async function getTicker() {
  const ticker = await client.call('market_history_api', 'get_ticker', {});
  console.log('Current price:', ticker.latest);
  console.log('24h volume:', ticker.steem_volume, 'STEEM');
}

// 주문장 조회
async function getOrderBook() {
  const orderBook = await client.call('market_history_api', 'get_order_book', {
    limit: 10
  });

  console.log('Top 10 Bids:');
  orderBook.bids.forEach(bid => {
    console.log(`  ${bid.real_price} SBD/STEEM - ${bid.steem} STEEM`);
  });

  console.log('Top 10 Asks:');
  orderBook.asks.forEach(ask => {
    console.log(`  ${ask.real_price} SBD/STEEM - ${ask.steem} STEEM`);
  });
}

// 최근 거래 조회
async function getRecentTrades() {
  const trades = await client.call('market_history_api', 'get_recent_trades', {
    limit: 20
  });

  trades.forEach(trade => {
    console.log(`${trade.date}: ${trade.current_pays} ↔ ${trade.open_pays}`);
  });
}

getTicker();
getOrderBook();
getRecentTrades();
```

## 모범 사례

### 주문 생성 시

1. **가격 검증**: 현재 시세와 크게 벗어난 가격은 피하세요.
2. **만료 시간 설정**: 장기간 유지할 주문이 아니라면 적절한 만료 시간을 설정하세요.
3. **orderid 관리**: 고유한 orderid를 사용하여 주문 추적을 용이하게 하세요.
4. **잔고 확인**: 주문 생성 전 충분한 잔고를 확인하세요.

### 거래 전략

1. **주문장 분석**: `get_order_book`으로 현재 시장 상황을 파악하세요.
2. **가격 모니터링**: `get_ticker`로 실시간 가격을 추적하세요.
3. **거래 내역 분석**: `get_trade_history`로 최근 거래 패턴을 분석하세요.
4. **단계별 주문**: 큰 주문은 여러 단계로 나누어 슬리피지를 줄이세요.

### 유동성 제공

1. **양쪽 시장 참여**: STEEM과 SBD 양쪽에서 거래하여 보상을 받으세요.
2. **주문 유지**: 최소 1분 이상 주문을 유지하세요.
3. **균형 잡힌 거래**: 한쪽으로 치우치지 않은 거래량을 유지하세요.

## 문제 해결

### 주문이 체결되지 않는 경우

- **가격 확인**: 너무 높거나 낮은 가격이 아닌지 확인
- **주문장 확인**: `get_order_book`으로 매칭 가능한 주문이 있는지 확인
- **만료 확인**: 주문이 만료되지 않았는지 확인

### 주문 생성 실패

- **잔고 부족**: 판매하려는 자산이 계정에 충분한지 확인
- **중복 orderid**: 같은 orderid로 기존 주문이 있는지 확인
- **권한 부족**: active key로 서명했는지 확인

### API 호출 실패

- **플러그인 확인**: 노드에 `market_history_api` 플러그인이 활성화되어 있는지 확인
- **limit 초과**: API별 최대 limit을 초과하지 않았는지 확인
- **노드 상태**: 연결된 노드가 정상 동작하는지 확인

## 참고 자료

### 관련 문서
- [CLI Wallet 가이드](../getting-started/cli-wallet-guide.md)
- [Market History 플러그인](../plugins/market_history.md)
- [Market History API](../plugins/market_history_api.md)
- [Operation 생성 가이드](../development/create-operation.md)

### 외부 링크
- [Steem 백서](https://steem.com/steem-whitepaper.pdf)
- [Steem API 문서](https://developers.steem.io/)
- [SteemDB 시장 페이지](https://steemdb.io/market)

### 소스 코드
- [limit_order operations](../../libraries/protocol/include/steem/protocol/steem_operations.hpp)
- [market_history plugin](../../libraries/plugins/market_history/)
- [market_history_api plugin](../../libraries/plugins/apis/market_history_api/)

## 요약

Steem의 내부 시장은 탈중앙화된 거래소로서 다음과 같은 특징을 제공합니다:

- ✅ **제한가 주문**: 원하는 가격에 주문 생성
- ✅ **자동 매칭**: Price-Time Priority 알고리즘
- ✅ **유동성 보상**: 시장 제조자에게 인센티브 제공
- ✅ **풍부한 API**: 시장 데이터 조회 및 분석
- ✅ **온체인 거래**: 투명하고 검증 가능한 거래
- ✅ **수수료 없음**: 거래 수수료 0%

내부 시장을 활용하여 STEEM과 SBD를 자유롭게 교환하고, 유동성 제공을 통해 추가 보상을 받을 수 있습니다.
