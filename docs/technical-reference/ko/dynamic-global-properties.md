# Dynamic Global Properties

이 문서는 Steem 블록체인의 실시간 전역 상태 정보를 유지하는 중요한 database object인 `dynamic_global_property_object`를 설명합니다.

## 목차

- [개요](#개요)
- [핵심 속성](#핵심-속성)
  - [블록 정보](#블록-정보)
  - [공급량 및 토큰 경제](#공급량-및-토큰-경제)
  - [Vesting 및 보상](#vesting-및-보상)
  - [Witness 스케줄링](#witness-스케줄링)
  - [블록체인 파라미터](#블록체인-파라미터)
- [주요 메서드](#주요-메서드)
- [코드에서의 사용](#코드에서의-사용)
- [API 접근](#api-접근)
- [구현 세부사항](#구현-세부사항)

## 개요

`dynamic_global_property_object`는 블록체인의 현재 상태를 저장하는 singleton database object입니다. 정적 구성 파라미터와 달리, 이러한 속성은 블록 처리 중에 지속적으로 업데이트되며 실시간 블록체인 상태를 반영합니다.

**위치**: `libraries/chain/include/steem/chain/global_property_object.hpp`

이 object는 다음에 필수적입니다:
- 블록 검증 및 생성
- 토큰 공급량 추적
- 보상 분배
- Witness 스케줄링
- 투표력 계산
- 경제 파라미터 조정

## 핵심 속성

### 블록 정보

블록체인의 현재 상태와 관련된 속성:

| 속성 | 타입 | 설명 |
|----------|------|-------------|
| `head_block_number` | `uint32_t` | 현재 블록 높이 (genesis는 0부터 시작) |
| `head_block_id` | `block_id_type` | 가장 최근 블록의 해시/ID |
| `time` | `time_point_sec` | 현재 헤드 블록의 타임스탬프 |
| `current_witness` | `account_name_type` | 현재 블록을 생성한 witness의 계정 이름 |
| `last_irreversible_block_num` | `uint32_t` | 되돌릴 수 없는 (포크할 수 없는) 블록 번호 |

**예제 접근**:
```cpp
const auto& dgpo = db.get_dynamic_global_properties();
uint32_t current_block = dgpo.head_block_number;
time_point_sec block_time = dgpo.time;
```

### 공급량 및 토큰 경제

STEEM 및 SBD 토큰의 총 공급량을 추적하는 속성:

| 속성 | 타입 | 설명 |
|----------|------|-------------|
| `virtual_supply` | `asset` | 속도 제한 및 price feed에 사용되는 가상 STEEM 공급량 |
| `current_supply` | `asset` | 총 유통 STEEM 토큰 |
| `confidential_supply` | `asset` | confidential/blinded 잔액에 보유된 STEEM |
| `current_sbd_supply` | `asset` | 총 유통 SBD 토큰 |
| `confidential_sbd_supply` | `asset` | confidential/blinded 잔액에 보유된 SBD |
| `sbd_interest_rate` | `uint16_t` | SBD 예금에 대한 연간 이자율 (베이시스 포인트) |
| `sbd_print_rate` | `uint16_t` | 새 SBD를 발행할 수 있는 비율 (기본값: `STEEM_100_PERCENT`) |

**참고**:
- 가상 공급량에는 유동 STEEM, 베스팅된 STEEM, SBD 부채가 포함됨
- 공급량 값은 보상, 전환 및 기타 operation을 기반으로 매 블록 업데이트됨
- 이자율은 witness가 설정하고 합의를 통해 조정됨

### Vesting 및 보상

STEEM Power (vested STEEM) 및 보상 풀과 관련된 속성:

| 속성 | 타입 | 설명 |
|----------|------|-------------|
| `total_vesting_fund_steem` | `asset` | 현재 vesting된 총 STEEM (STEEM Power 백업) |
| `total_vesting_shares` | `asset` | 존재하는 총 VESTS (Vesting Shares) |
| `total_reward_fund_steem` | `asset` | 보상 풀에서 사용 가능한 총 STEEM |
| `total_reward_shares2` | `fc::uint128` | 비례 보상 분배를 위한 곡선 조정 보상 shares의 합 (`evaluate_reward_curve(rshares)`의 누적 합계). 참고: "2"는 역사적 - 원래 2차 곡선용 |
| `pending_rewarded_vesting_shares` | `asset` | 보류 중인 보상이 청구될 때 생성될 VESTS |
| `pending_rewarded_vesting_steem` | `asset` | 보류 중인 VESTS 보상을 위한 STEEM 백업 |

**주요 계산**:

Object는 두 가지 중요한 가격 계산을 제공합니다:

```cpp
// 현재 VESTS에서 STEEM으로의 전환율
price get_vesting_share_price() const
{
    if (total_vesting_fund_steem.amount == 0 || total_vesting_shares.amount == 0)
        return price(asset(1000, STEEM_SYMBOL), asset(1000000, VESTS_SYMBOL));

    return price(total_vesting_shares, total_vesting_fund_steem);
}

// 보류 중인 보상을 포함한 VESTS에서 STEEM으로의 비율
price get_reward_vesting_share_price() const
{
    return price(total_vesting_shares + pending_rewarded_vesting_shares,
                 total_vesting_fund_steem + pending_rewarded_vesting_steem);
}
```

**사용 예**:
```cpp
const auto& dgpo = db.get_dynamic_global_properties();

// VESTS를 STEEM으로 전환
asset vests = asset(1000000, VESTS_SYMBOL);
asset steem = vests * dgpo.get_vesting_share_price();

// 보류 중인 보상을 포함한 총 vesting fund 계산
asset total_fund = dgpo.total_vesting_fund_steem + dgpo.pending_rewarded_vesting_steem;
```

### Witness 스케줄링

Witness 스케줄링 및 참여율 추적에 사용되는 속성:

| 속성 | 타입 | 설명 |
|----------|------|-------------|
| `current_aslot` | `uint64_t` | 절대 슬롯 번호 (genesis 이후 총 슬롯) |
| `recent_slots_filled` | `fc::uint128_t` | 최근 슬롯 채움의 비트맵 (1 = 채워짐, 0 = 놓침) |
| `participation_count` | `uint8_t` | 최근 히스토리에서 채워진 슬롯 수 (백분율을 위해 128로 나누기) |

**Witness 참여율 계산**:
```cpp
// Witness 참여율 계산
const auto& dgpo = db.get_dynamic_global_properties();
double participation_rate = dgpo.participation_count / 128.0;

// 특정 슬롯이 채워졌는지 확인
bool slot_filled = (dgpo.recent_slots_filled & (fc::uint128_t(1) << slot_offset)) != 0;
```

**참고**:
- 절대 슬롯 (`current_aslot`) = genesis 이후 총 슬롯 = 놓친 슬롯 + `head_block_number`
- `recent_slots_filled`는 비트맵으로 마지막 128 슬롯을 추적
- 참여는 네트워크 상태 모니터링에 중요

### 블록체인 파라미터

블록체인 동작을 관리하는 동적 파라미터:

| 속성 | 타입 | 설명 |
|----------|------|-------------|
| `maximum_block_size` | `uint32_t` | 허용되는 최대 블록 크기 (witness 제안의 중앙값) |
| `vote_power_reserve_rate` | `uint32_t` | 하루당 투표 재생성율 |
| `delegation_return_period` | `uint32_t` | 위임 해제 후 위임된 VESTS가 반환되기 전 시간 |
| `smt_creation_fee` | `asset` | Smart Media Token (SMT) 생성 수수료 (활성화된 경우) |

**참고**:
- `maximum_block_size`는 witness 합의로 설정됨 (모든 활성 witness 제안의 중앙값)
- 프로토콜은 네트워크 정지를 방지하기 위해 최소 블록 크기를 강제함
- `vote_power_reserve_rate`는 투표 재생성 속도를 결정 (기본값: `STEEM_INITIAL_VOTE_POWER_RATE`)
- `delegation_return_period`는 빠른 위임 사이클 남용을 방지

## 주요 메서드

### get_vesting_share_price()

VESTS (Vesting Shares)와 STEEM 간의 현재 환율을 계산합니다.

**시그니처**:
```cpp
price get_vesting_share_price() const
```

**반환값**: VESTS에서 STEEM으로의 전환율을 나타내는 `price` object

**사용**:
```cpp
const auto& dgpo = db.get_dynamic_global_properties();
price vests_to_steem = dgpo.get_vesting_share_price();

// 1,000,000 VESTS를 STEEM으로 전환
asset vests(1000000, VESTS_SYMBOL);
asset steem = vests * vests_to_steem;
```

**기본 비율**: Vesting fund가 비어 있으면 1000 STEEM = 1,000,000 VESTS 반환 (1:1000 비율)

### get_reward_vesting_share_price()

아직 청구되지 않은 보류 중인 보상을 포함한 VESTS에서 STEEM으로의 비율을 계산합니다.

**시그니처**:
```cpp
price get_reward_vesting_share_price() const
```

**반환값**: 보류 중인 보상 VESTS 및 STEEM을 포함하는 `price` object

**사용**:
```cpp
const auto& dgpo = db.get_dynamic_global_properties();
price reward_rate = dgpo.get_reward_vesting_share_price();

// 보상 계산에 사용
asset pending_vests = calculate_pending_vests_reward();
asset pending_steem_value = pending_vests * reward_rate;
```

## 코드에서의 사용

### Dynamic Global Properties 접근

Database는 singleton object에 접근하는 편의 메서드를 제공합니다:

```cpp
const dynamic_global_property_object& db.get_dynamic_global_properties() const
```

**일반적인 사용 패턴**:
```cpp
void some_evaluator::do_apply(const some_operation& op)
{
    const auto& dgpo = _db.get_dynamic_global_properties();

    // 현재 블록 시간 접근
    time_point_sec now = dgpo.time;

    // 블록 번호 확인
    uint32_t current_block = dgpo.head_block_number;

    // 현재 witness 가져오기
    account_name_type current_witness = dgpo.current_witness;

    // Vesting 전환 계산
    asset vests = op.vesting_shares;
    asset steem = vests * dgpo.get_vesting_share_price();
}
```

### 속성 수정

Dynamic global properties는 블록 처리 중에 database에 의해 수정됩니다. 직접 수정은 chain library 내에서만 수행해야 합니다:

```cpp
db.modify(dgpo, [&](dynamic_global_property_object& gpo) {
    gpo.total_vesting_fund_steem += new_vesting;
    gpo.total_vesting_shares += new_shares;
});
```

**중요**: Chain library만 이러한 속성을 수정해야 합니다. Plugin 및 evaluator는 이를 읽기 전용으로 처리해야 합니다.

## API 접근

### database_api

Dynamic global properties는 `database_api` plugin을 통해 접근할 수 있습니다:

**메서드**: `get_dynamic_global_properties`

**예제 JSON-RPC 요청**:
```json
{
  "jsonrpc": "2.0",
  "method": "database_api.get_dynamic_global_properties",
  "params": {},
  "id": 1
}
```

**예제 응답**:
```json
{
  "jsonrpc": "2.0",
  "result": {
    "id": 0,
    "head_block_number": 50000000,
    "head_block_id": "02faf080...",
    "time": "2023-01-15T12:00:00",
    "current_witness": "witness.account",
    "virtual_supply": "400000000.000 STEEM",
    "current_supply": "350000000.000 STEEM",
    "confidential_supply": "0.000 STEEM",
    "current_sbd_supply": "15000000.000 SBD",
    "confidential_sbd_supply": "0.000 SBD",
    "total_vesting_fund_steem": "200000000.000 STEEM",
    "total_vesting_shares": "400000000000.000000 VESTS",
    "total_reward_fund_steem": "5000000.000 STEEM",
    "total_reward_shares2": "10000000000000000000",
    "pending_rewarded_vesting_shares": "100000.000000 VESTS",
    "pending_rewarded_vesting_steem": "50.000 STEEM",
    "sbd_interest_rate": 0,
    "sbd_print_rate": 10000,
    "maximum_block_size": 65536,
    "current_aslot": 52000000,
    "recent_slots_filled": "340282366920938463463374607431768211455",
    "participation_count": 128,
    "last_irreversible_block_num": 49999900,
    "vote_power_reserve_rate": 10,
    "delegation_return_period": 432000
  },
  "id": 1
}
```

## 구현 세부사항

### Database Object 타입

```cpp
class dynamic_global_property_object :
    public object<dynamic_global_property_object_type, dynamic_global_property_object>
{
    // ... 속성 ...
};
```

**Object ID**: 항상 `id = 0` (singleton object)

**스토리지**: Chainbase memory-mapped database에 저장됨

### Multi-Index Container

```cpp
typedef multi_index_container<
    dynamic_global_property_object,
    indexed_by<
        ordered_unique<tag<by_id>,
            member<dynamic_global_property_object,
                   dynamic_global_property_object::id_type,
                   &dynamic_global_property_object::id>>
    >,
    allocator<dynamic_global_property_object>
> dynamic_global_property_index;
```

**인덱스**: ID별 단일 인덱스 (object가 하나뿐이므로)

### 직렬화

Object는 바이너리 및 JSON 형식으로 자동 직렬화하기 위해 `FC_REFLECT`를 사용합니다:

```cpp
FC_REFLECT(steem::chain::dynamic_global_property_object,
    (id)
    (head_block_number)
    (head_block_id)
    // ... 모든 필드 ...
)
```

이는 다음을 가능하게 합니다:
- 효율적인 저장을 위한 바이너리 패킹
- API 응답을 위한 JSON 변환
- 디버깅을 위한 타입 인트로스펙션

### 업데이트 빈도

속성은 다른 빈도로 업데이트됩니다:

| 속성 | 업데이트 빈도 |
|----------|------------------|
| 블록 정보 (`head_block_number`, `time` 등) | 매 블록 (3초) |
| 공급량 값 | 공급량에 영향을 주는 operation (전송, 보상 등) |
| Vesting 값 | Power up/down operation, 보상 청구 |
| Witness 스케줄링 | 매 블록 (슬롯 추적) |
| 파라미터 (`sbd_interest_rate` 등) | Witness가 변경에 투표할 때 |

### 스레드 안전성

- **읽기 접근**: Database read lock을 통한 스레드 안전
- **쓰기 접근**: 블록 적용 중에만 (단일 스레드)
- **중요**: Read lock은 1초 후 자동 만료 - API 호출이 빠르게 완료되도록 설계

### Hardfork 고려사항

일부 속성은 특정 hardfork에서 추가되거나 수정되었습니다:

- **Hardfork 0**: 초기 `delegation_return_period` 값
- **이후 Hardfork**: 추가 SMT 속성 (활성화된 경우)

새로 추가된 속성에 접근할 때는 항상 hardfork 버전을 확인하십시오:

```cpp
if (db.has_hardfork(STEEM_HARDFORK_0_XX)) {
    // HF 0_XX에서 추가된 속성에 안전하게 접근
}
```

## 일반적인 패턴

### 현재 STEEM Power 값 계산

```cpp
const auto& dgpo = db.get_dynamic_global_properties();
const auto& account = db.get_account(account_name);

asset vesting_shares = account.vesting_shares;
asset steem_power = vesting_shares * dgpo.get_vesting_share_price();
```

### 네트워크 참여 확인

```cpp
const auto& dgpo = db.get_dynamic_global_properties();
double participation = dgpo.participation_count / 128.0;

if (participation < 0.95) {
    wlog("Low network participation: ${p}%", ("p", participation * 100));
}
```

### 블록 타이밍 검증

```cpp
const auto& dgpo = db.get_dynamic_global_properties();
time_point_sec now = dgpo.time;

FC_ASSERT(op.timestamp >= now, "Operation timestamp is in the past");
FC_ASSERT(op.timestamp <= now + fc::seconds(3600), "Operation timestamp is too far in future");
```

### 인플레이션율 계산

```cpp
const auto& dgpo = db.get_dynamic_global_properties();

// 가상 공급량은 모든 형태의 STEEM 가치를 포함
asset virtual_supply = dgpo.virtual_supply;

// 현재 공급량은 유동 STEEM
asset current_supply = dgpo.current_supply;

// SBD 부채는 가상 공급량에 영향
asset sbd_debt = dgpo.current_sbd_supply;
```

## 모범 사례

1. **읽기 전용 접근**: Evaluator 및 plugin에서 `dynamic_global_property_object`를 항상 읽기 전용으로 처리
2. **로컬 캐시**: 여러 속성에 접근하는 경우 반복 조회를 피하기 위해 참조를 저장:
   ```cpp
   const auto& dgpo = db.get_dynamic_global_properties();
   // dgpo를 여러 번 사용
   ```
3. **오래 보유하지 않기**: 블록 경계를 넘어 참조를 저장하지 마십시오 - 속성이 자주 변경됨
4. **헬퍼 메서드 사용**: 수동 계산보다 `get_vesting_share_price()` 선호
5. **Hardfork 확인**: 이후 버전에서 추가된 속성에 접근하기 전에 hardfork 버전 확인
6. **API 성능**: API를 통해 노출할 때 속성이 블록당 한 번만 변경되므로 캐싱 고려

## 관련 문서

- [Chain Parameters](chain-parameters.md) - 정적 블록체인 구성
- [System Architecture](system-architecture.md) - 전체 시스템 설계
- [API Notes](api-notes.md) - API 사용 지침
- [Asset Format](asset-format.md) - Asset 표현 및 전환

## 참조

- **소스 코드**: `libraries/chain/include/steem/chain/global_property_object.hpp`
- **Database 구현**: `libraries/chain/database.cpp`
- **API 구현**: `libraries/plugins/apis/database_api/database_api.cpp`
- **Evaluator 사용**: `libraries/chain/steem_evaluator.cpp`
