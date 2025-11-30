# Bundler Plugin Implementation Plan

## 목차
1. [개요](#개요)
2. [시스템 아키텍처](#시스템-아키텍처)
3. [구현 단계](#구현-단계)
4. [데이터 구조](#데이터-구조)
5. [플러그인 구조](#플러그인-구조)
6. [API 설계](#api-설계)
7. [EVM 통합](#evm-통합)
8. [배치 처리 로직](#배치-처리-로직)
9. [보안 고려사항](#보안-고려사항)
10. [테스트 계획](#테스트-계획)
11. [배포 및 운영](#배포-및-운영)

---

## 개요

### 목적
Steem 블록체인에 ERC-4337 스타일의 Bundler 기능을 추가하여, 여러 사용자의 EVM 트랜잭션 의도(UserOperation)를 수집하고 배치로 묶어서 EVM 호환 체인에 제출하는 시스템을 구축합니다.

### 핵심 기능
- **UserOperation 수집**: 사용자로부터 EVM 트랜잭션 의도를 수신
- **검증 및 시뮬레이션**: UserOperation의 유효성 검사
- **배치 생성**: 여러 UserOperation을 하나의 번들로 묶기
- **EVM 제출**: 번들을 EVM 체인에 트랜잭션으로 제출
- **상태 추적**: 제출된 트랜잭션의 상태 모니터링

### 기대 효과
- **가스비 절감**: 배치 처리를 통한 개별 트랜잭션 비용 감소
- **처리량 향상**: 여러 트랜잭션을 동시에 처리
- **사용자 경험 개선**: 가스비 대납, 메타 트랜잭션 지원

---

## 시스템 아키텍처

```
┌─────────────────────────────────────────────────────────────┐
│                     Steem Blockchain                         │
│                                                              │
│  ┌────────────────────────────────────────────────────┐    │
│  │              Bundler Plugin (State)                 │    │
│  │                                                      │    │
│  │  ┌──────────────┐      ┌──────────────┐           │    │
│  │  │ UserOperation│      │    Bundle    │           │    │
│  │  │   Mempool    │─────▶│   Manager    │           │    │
│  │  └──────────────┘      └──────────────┘           │    │
│  │         │                      │                   │    │
│  │         │                      ▼                   │    │
│  │         │              ┌──────────────┐           │    │
│  │         │              │  EVM Client  │           │    │
│  │         │              │   (Web3cpp)  │           │    │
│  │         │              └──────────────┘           │    │
│  │         │                      │                   │    │
│  └─────────┼──────────────────────┼───────────────────┘    │
│            │                      │                        │
│  ┌─────────▼──────────────────────▼───────────────────┐    │
│  │           Bundler API Plugin (JSON-RPC)            │    │
│  │                                                     │    │
│  │  - submitUserOp()                                  │    │
│  │  - getUserOpStatus()                               │    │
│  │  - getBundleInfo()                                 │    │
│  │  - estimateUserOpGas()                             │    │
│  └─────────────────────────────────────────────────────┘    │
│                            │                                │
└────────────────────────────┼────────────────────────────────┘
                             │ JSON-RPC API
                             ▼
                    ┌─────────────────┐
                    │   External      │
                    │   Clients       │
                    │   (DApps, etc)  │
                    └─────────────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │   EVM Chain     │
                    │ (Ethereum, etc) │
                    └─────────────────┘
```

### 컴포넌트 설명

#### 1. Bundler Plugin (State Plugin)
- **역할**: 핵심 비즈니스 로직 및 상태 관리
- **책임**:
  - UserOperation 멤풀 관리
  - 배치 생성 및 스케줄링
  - EVM 트랜잭션 제출
  - 상태 추적 및 업데이트

#### 2. Bundler API Plugin (API Plugin)
- **역할**: 외부 클라이언트와의 인터페이스
- **책임**:
  - JSON-RPC API 엔드포인트 제공
  - 요청 검증 및 응답 포맷팅

#### 3. EVM Client
- **역할**: EVM 호환 체인과의 통신
- **책임**:
  - 트랜잭션 서명 및 제출
  - 가스 추정
  - 트랜잭션 상태 조회

---

## 구현 단계

### Phase 1: 기본 인프라 구축 (1-2주)

#### 1.1 플러그인 스켈레톤 생성
```bash
# 디렉토리 구조
src/plugins/bundler/
├── include/steem/plugins/bundler/
│   ├── bundler_plugin.hpp
│   ├── bundler_objects.hpp
│   └── bundler_operations.hpp
├── bundler_plugin.cpp
└── CMakeLists.txt

src/plugins/apis/bundler_api/
├── include/steem/plugins/bundler_api/
│   ├── bundler_api_plugin.hpp
│   └── bundler_api.hpp
├── bundler_api_plugin.cpp
├── bundler_api.cpp
└── CMakeLists.txt
```

**작업 항목:**
- [ ] 플러그인 디렉토리 구조 생성
- [ ] CMakeLists.txt 설정
- [ ] 기본 플러그인 클래스 구현 (plugin_initialize, plugin_startup, plugin_shutdown)
- [ ] appbase 플러그인 등록

#### 1.2 데이터 구조 정의
**작업 항목:**
- [ ] `user_operation_object` 정의 (완료)
- [ ] `bundle_object` 정의 (완료)
- [ ] Chainbase 인덱스 설정
- [ ] FC 리플렉션 매크로 추가

#### 1.3 설정 옵션 추가
```cpp
// config.ini 옵션
bundler-enabled = true
bundler-evm-rpc-endpoint = http://localhost:8545
bundler-chain-id = 1
bundler-private-key = 0x...
bundler-max-bundle-size = 10
bundler-bundle-interval = 5000  # milliseconds
bundler-gas-price-multiplier = 1.2
```

**작업 항목:**
- [ ] `set_program_options()` 구현
- [ ] 설정 파일 파싱 로직
- [ ] 프라이빗 키 보안 저장 방식 설계

---

### Phase 2: EVM 통합 (2-3주)

#### 2.1 EVM 클라이언트 라이브러리 선택 및 통합

**옵션 1: libcurl + JSON-RPC (추천)**
- 장점: 의존성 최소화, 간단한 구현
- 단점: 직접 JSON-RPC 처리 필요

**옵션 2: web3cpp**
- 장점: 완전한 Web3 기능
- 단점: 추가 의존성

**작업 항목:**
- [ ] 라이브러리 선택 및 의존성 추가
- [ ] EVM RPC 클라이언트 래퍼 클래스 구현
- [ ] 트랜잭션 서명 로직 구현 (secp256k1)
- [ ] Nonce 관리 시스템 구현

#### 2.2 EVM 트랜잭션 제출 로직

```cpp
class evm_transaction_submitter {
public:
   // EVM 트랜잭션 제출
   string submit_transaction(const signed_transaction& tx);

   // 트랜잭션 상태 확인
   transaction_receipt get_receipt(const string& tx_hash);

   // 가스 가격 조회
   uint64_t get_gas_price();

   // Nonce 조회
   uint64_t get_nonce(const string& address);
};
```

**작업 항목:**
- [ ] `evm_transaction_submitter` 클래스 구현
- [ ] RPC 메서드 래핑 (eth_sendRawTransaction, eth_getTransactionReceipt 등)
- [ ] 에러 처리 및 재시도 로직
- [ ] 트랜잭션 큐 및 pending 관리

#### 2.3 EntryPoint 컨트랙트 통합

```solidity
// ERC-4337 EntryPoint interface
interface IEntryPoint {
    function handleOps(
        UserOperation[] calldata ops,
        address payable beneficiary
    ) external;
}
```

**작업 항목:**
- [ ] EntryPoint 컨트랙트 ABI 정의
- [ ] `handleOps` 함수 호출 인코딩 구현
- [ ] 컨트랙트 주소 설정 옵션 추가

---

### Phase 3: 멤풀 및 배치 관리 (2주)

#### 3.1 UserOperation 멤풀 구현

```cpp
class user_operation_mempool {
public:
   // UserOperation 추가
   void add_user_op(const user_operation_object& user_op);

   // 유효한 UserOperation 선택
   vector<user_operation_object> select_user_ops(
      uint32_t max_count,
      uint64_t max_gas
   );

   // 만료된 UserOperation 정리
   void cleanup_expired();

   // 우선순위 정렬 (가스비 기준)
   void sort_by_priority();
};
```

**작업 항목:**
- [ ] 멤풀 데이터 구조 구현 (우선순위 큐)
- [ ] UserOperation 추가/제거 로직
- [ ] 가스비 기반 정렬 알고리즘
- [ ] 멤풀 크기 제한 및 정리 로직
- [ ] 중복 방지 (동일 sender + nonce)

#### 3.2 배치 생성 로직

```cpp
class bundle_manager {
public:
   // 번들 생성
   bundle_object create_bundle(
      const vector<user_operation_object>& user_ops
   );

   // 번들 시뮬레이션 (선택적)
   bool simulate_bundle(const bundle_object& bundle);

   // 번들 제출
   string submit_bundle(const bundle_object& bundle);
};
```

**작업 항목:**
- [ ] 번들 생성 알고리즘
- [ ] 가스 한도 계산 및 최적화
- [ ] 번들 크기 제한 처리
- [ ] 실패한 UserOperation 처리 전략

#### 3.3 스케줄러 구현

```cpp
class bundle_scheduler {
public:
   // 주기적으로 번들 생성 및 제출
   void schedule_bundle_creation();

   // 즉시 번들 생성 (충분한 UserOps 있을 때)
   void trigger_immediate_bundle();
};
```

**작업 항목:**
- [ ] Boost.Asio 타이머 기반 스케줄러
- [ ] 설정 가능한 번들 생성 주기
- [ ] 동적 번들 크기 조정
- [ ] 가스 가격 모니터링 및 조정

---

### Phase 4: API 구현 (1-2주)

#### 4.1 JSON-RPC API 엔드포인트

```cpp
class bundler_api {
public:
   // UserOperation 제출
   struct submit_user_op_result {
      string user_op_hash;
      bool accepted;
      string error_message;
   };
   submit_user_op_result submit_user_op(const user_operation_object& user_op);

   // UserOperation 상태 조회
   struct user_op_status {
      string user_op_hash;
      uint8_t status;  // pending, bundled, submitted, confirmed, failed
      string tx_hash;
      uint32_t block_number;
   };
   user_op_status get_user_op_status(const string& user_op_hash);

   // 번들 정보 조회
   struct bundle_info {
      bundle_object bundle;
      vector<user_operation_object> user_ops;
   };
   bundle_info get_bundle_info(const bundle_object::id_type& bundle_id);

   // 가스 추정
   struct gas_estimate {
      uint64_t call_gas_limit;
      uint64_t verification_gas_limit;
      uint64_t pre_verification_gas;
   };
   gas_estimate estimate_user_op_gas(const user_operation_object& user_op);

   // 지원되는 EntryPoint 조회
   vector<string> get_supported_entry_points();
};
```

**작업 항목:**
- [ ] API 클래스 구현
- [ ] JSON-RPC 플러그인 통합
- [ ] 요청 검증 로직
- [ ] 에러 응답 처리
- [ ] API 문서 작성

#### 4.2 WebSocket 지원 (선택적)

**작업 항목:**
- [ ] WebSocket 엔드포인트 추가
- [ ] 실시간 UserOperation 상태 업데이트 스트리밍
- [ ] 구독/취소 로직

---

### Phase 5: 검증 및 시뮬레이션 (2주)

#### 5.1 UserOperation 검증

```cpp
class user_op_validator {
public:
   struct validation_result {
      bool valid;
      string error_message;
      uint64_t estimated_gas;
   };

   // 기본 검증 (형식, 서명)
   validation_result validate_syntax(const user_operation_object& user_op);

   // 시뮬레이션 검증 (EVM 호출)
   validation_result simulate(const user_operation_object& user_op);

   // 중복 검사
   bool is_duplicate(const user_operation_object& user_op);
};
```

**작업 항목:**
- [ ] 서명 검증 (ECDSA)
- [ ] 필드 유효성 검사 (가스 한도, nonce 등)
- [ ] EVM 시뮬레이션 (`eth_call` 사용)
- [ ] 금지된 opcode 검사 (ERC-4337 규칙)
- [ ] 스토리지 액세스 규칙 검증

#### 5.2 Paymaster 지원 (선택적)

```cpp
class paymaster_manager {
public:
   // Paymaster 검증
   bool validate_paymaster(
      const string& paymaster_address,
      const user_operation_object& user_op
   );

   // Paymaster 잔액 확인
   uint256_t get_paymaster_balance(const string& paymaster_address);
};
```

**작업 항목:**
- [ ] Paymaster 검증 로직
- [ ] Paymaster 화이트리스트/블랙리스트
- [ ] Paymaster 잔액 추적

---

### Phase 6: 상태 추적 및 모니터링 (1주)

#### 6.1 트랜잭션 상태 업데이트

```cpp
class transaction_tracker {
public:
   // 트랜잭션 상태 폴링
   void poll_transaction_status(const string& tx_hash);

   // 상태 업데이트 콜백
   void on_transaction_confirmed(const string& tx_hash);
   void on_transaction_failed(const string& tx_hash);

   // 재제출 로직
   void resubmit_if_needed(const bundle_object& bundle);
};
```

**작업 항목:**
- [ ] 주기적인 트랜잭션 상태 폴링
- [ ] 확인(confirmation) 수 추적
- [ ] 실패한 트랜잭션 재제출 로직
- [ ] 타임아웃 처리

#### 6.2 메트릭 및 로깅

**작업 항목:**
- [ ] 주요 메트릭 수집 (제출된 번들 수, 성공률 등)
- [ ] 상세 로깅 (디버그, 경고, 에러)
- [ ] StatsD 플러그인 통합 (선택적)

---

### Phase 7: 테스트 및 최적화 (2-3주)

#### 7.1 단위 테스트

**작업 항목:**
- [ ] UserOperation 검증 테스트
- [ ] 멤풀 관리 테스트
- [ ] 배치 생성 테스트
- [ ] EVM 트랜잭션 제출 테스트 (모킹)

#### 7.2 통합 테스트

**작업 항목:**
- [ ] 로컬 EVM 체인 (Ganache, Hardhat) 연동 테스트
- [ ] EntryPoint 컨트랙트 배포 및 테스트
- [ ] End-to-end 플로우 테스트
- [ ] 부하 테스트 (다수의 UserOperation 처리)

#### 7.3 성능 최적화

**작업 항목:**
- [ ] 멤풀 성능 프로파일링
- [ ] 배치 생성 알고리즘 최적화
- [ ] 데이터베이스 인덱스 튜닝
- [ ] 메모리 사용량 최적화

---

## 데이터 구조

### UserOperation Object

```cpp
class user_operation_object : public object< user_operation_object_type, user_operation_object >
{
   public:
      id_type           id;

      // EVM transaction fields
      string            sender;              // EVM address (20 bytes hex)
      uint64_t          nonce;
      string            init_code;           // Hex string
      string            call_data;           // Hex string

      // Gas limits
      uint64_t          call_gas_limit;
      uint64_t          verification_gas_limit;
      uint64_t          pre_verification_gas;

      // Gas pricing (wei as string to handle uint256)
      string            max_fee_per_gas;
      string            max_priority_fee_per_gas;

      // Optional paymaster
      string            paymaster_and_data;  // Hex string

      // Signature
      string            signature;           // Hex string

      // Metadata
      string            user_op_hash;        // Keccak256 hash
      fc::time_point_sec created_time;
      fc::time_point_sec expiration_time;

      // Status
      enum status_type {
         pending = 0,
         bundled = 1,
         submitted = 2,
         confirmed = 3,
         failed = 4
      };
      uint8_t           status;

      string            tx_hash;             // EVM tx hash
      string            error_message;
};
```

### Bundle Object

```cpp
class bundle_object : public object< bundle_object_type, bundle_object >
{
   public:
      id_type           id;

      vector< user_operation_object::id_type > user_ops;

      fc::time_point_sec created_time;
      fc::time_point_sec submitted_time;

      string            evm_tx_hash;
      uint32_t          block_number;

      enum status_type {
         pending = 0,
         submitted = 1,
         confirmed = 2,
         failed = 3
      };
      uint8_t           status;

      string            error_message;
};
```

### 인덱스

```cpp
// UserOperation 인덱스
- by_id: 기본 ID 인덱스
- by_user_op_hash: UserOperation 해시로 빠른 조회
- by_sender: sender 주소로 그룹화
- by_status: 상태별 필터링
- by_expiration: 만료 시간 정렬
- by_created_time: 생성 시간 정렬 (FIFO)

// Bundle 인덱스
- by_id: 기본 ID 인덱스
- by_bundle_status: 번들 상태별 필터링
- by_bundle_created_time: 생성 시간 정렬
```

---

## 플러그인 구조

### Bundler Plugin (State Plugin)

```cpp
namespace steem { namespace plugins { namespace bundler {

class bundler_plugin : public appbase::plugin< bundler_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES(
      (steem::plugins::chain::chain_plugin)
      (steem::plugins::p2p::p2p_plugin)
   )

   bundler_plugin();
   virtual ~bundler_plugin();

   static const std::string& name() {
      static std::string name = "bundler";
      return name;
   }

   virtual void set_program_options(
      boost::program_options::options_description &cli,
      boost::program_options::options_description &cfg
   ) override;

   virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
   virtual void plugin_startup() override;
   virtual void plugin_shutdown() override;

   // Public API for bundler_api_plugin
   void submit_user_operation(const user_operation_object& user_op);
   user_operation_object get_user_operation(const string& user_op_hash) const;
   vector<user_operation_object> get_pending_user_operations(uint32_t limit) const;
   bundle_object get_bundle(const bundle_object::id_type& id) const;

private:
   std::unique_ptr< class bundler_plugin_impl > my;
};

} } } // steem::plugins::bundler
```

### Bundler API Plugin (API Plugin)

```cpp
namespace steem { namespace plugins { namespace bundler_api {

class bundler_api_plugin : public appbase::plugin< bundler_api_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES(
      (steem::plugins::bundler::bundler_plugin)
      (steem::plugins::json_rpc::json_rpc_plugin)
   )

   bundler_api_plugin();
   virtual ~bundler_api_plugin();

   static const std::string& name() {
      static std::string name = "bundler_api";
      return name;
   }

   virtual void set_program_options(
      boost::program_options::options_description& cli,
      boost::program_options::options_description& cfg
   ) override;

   virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
   virtual void plugin_startup() override;
   virtual void plugin_shutdown() override;

   std::shared_ptr< class bundler_api > api;
};

} } } // steem::plugins::bundler_api
```

---

## API 설계

### JSON-RPC Methods

#### 1. `bundler.submitUserOp`

**요청:**
```json
{
  "jsonrpc": "2.0",
  "method": "bundler.submitUserOp",
  "params": {
    "sender": "0x1234...",
    "nonce": "0",
    "initCode": "0x",
    "callData": "0xabcd...",
    "callGasLimit": "100000",
    "verificationGasLimit": "50000",
    "preVerificationGas": "21000",
    "maxFeePerGas": "50000000000",
    "maxPriorityFeePerGas": "2000000000",
    "paymasterAndData": "0x",
    "signature": "0x5678..."
  },
  "id": 1
}
```

**응답:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "userOpHash": "0xabcd...",
    "accepted": true,
    "estimatedConfirmationTime": 30
  },
  "id": 1
}
```

**에러:**
```json
{
  "jsonrpc": "2.0",
  "error": {
    "code": -32000,
    "message": "Invalid signature"
  },
  "id": 1
}
```

#### 2. `bundler.getUserOpStatus`

**요청:**
```json
{
  "jsonrpc": "2.0",
  "method": "bundler.getUserOpStatus",
  "params": {
    "userOpHash": "0xabcd..."
  },
  "id": 2
}
```

**응답:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "userOpHash": "0xabcd...",
    "status": "confirmed",
    "bundleId": "123",
    "evmTxHash": "0xdef0...",
    "blockNumber": 12345678,
    "confirmations": 12
  },
  "id": 2
}
```

#### 3. `bundler.getBundleInfo`

**요청:**
```json
{
  "jsonrpc": "2.0",
  "method": "bundler.getBundleInfo",
  "params": {
    "bundleId": "123"
  },
  "id": 3
}
```

**응답:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "bundleId": "123",
    "userOps": [
      {"userOpHash": "0xabc1...", "sender": "0x1234..."},
      {"userOpHash": "0xabc2...", "sender": "0x5678..."}
    ],
    "evmTxHash": "0xdef0...",
    "status": "confirmed",
    "blockNumber": 12345678,
    "createdTime": "2024-01-15T10:30:00Z",
    "submittedTime": "2024-01-15T10:30:05Z"
  },
  "id": 3
}
```

#### 4. `bundler.estimateUserOpGas`

**요청:**
```json
{
  "jsonrpc": "2.0",
  "method": "bundler.estimateUserOpGas",
  "params": {
    "sender": "0x1234...",
    "callData": "0xabcd...",
    "initCode": "0x"
  },
  "id": 4
}
```

**응답:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "callGasLimit": "100000",
    "verificationGasLimit": "50000",
    "preVerificationGas": "21000"
  },
  "id": 4
}
```

#### 5. `bundler.getSupportedEntryPoints`

**요청:**
```json
{
  "jsonrpc": "2.0",
  "method": "bundler.getSupportedEntryPoints",
  "params": {},
  "id": 5
}
```

**응답:**
```json
{
  "jsonrpc": "2.0",
  "result": [
    "0x5FF137D4b0FDCD49DcA30c7CF57E578a026d2789"
  ],
  "id": 5
}
```

---

## EVM 통합

### EVM RPC 클라이언트 구현

```cpp
class evm_rpc_client {
public:
   evm_rpc_client(const string& rpc_endpoint, uint64_t chain_id);

   // 트랜잭션 제출
   string send_raw_transaction(const string& signed_tx_hex);

   // 트랜잭션 영수증 조회
   optional<transaction_receipt> get_transaction_receipt(const string& tx_hash);

   // 가스 가격 조회
   uint64_t get_gas_price();

   // Nonce 조회
   uint64_t get_transaction_count(const string& address);

   // 블록 번호 조회
   uint64_t get_block_number();

   // eth_call (시뮬레이션)
   string eth_call(const call_params& params);

   // 가스 추정
   uint64_t estimate_gas(const call_params& params);

private:
   string rpc_endpoint_;
   uint64_t chain_id_;

   // JSON-RPC 요청 전송
   json send_request(const string& method, const json& params);
};
```

### 트랜잭션 서명

```cpp
class transaction_signer {
public:
   transaction_signer(const string& private_key_hex);

   // EIP-1559 트랜잭션 서명
   string sign_eip1559_transaction(const eip1559_tx_params& params);

   // 메시지 서명 (UserOp 검증용)
   string sign_message(const string& message_hash);

   // 퍼블릭 키 조회
   string get_public_key() const;

   // 주소 조회
   string get_address() const;

private:
   fc::ecc::private_key private_key_;
};
```

### EntryPoint 컨트랙트 인터페이스

```cpp
class entry_point_contract {
public:
   entry_point_contract(
      const string& contract_address,
      evm_rpc_client& rpc_client
   );

   // handleOps 함수 호출 데이터 생성
   string encode_handle_ops(
      const vector<user_operation_object>& user_ops,
      const string& beneficiary
   );

   // 가스 추정
   uint64_t estimate_handle_ops_gas(
      const vector<user_operation_object>& user_ops
   );

   // UserOperation 시뮬레이션
   struct simulation_result {
      bool success;
      uint64_t gas_used;
      string error_message;
   };
   simulation_result simulate_validation(
      const user_operation_object& user_op
   );

private:
   string contract_address_;
   evm_rpc_client& rpc_client_;

   // ABI 인코딩/디코딩 헬퍼
   string encode_user_operation(const user_operation_object& user_op);
};
```

---

## 배치 처리 로직

### 배치 생성 알고리즘

```cpp
class bundle_creator {
public:
   struct bundle_config {
      uint32_t max_bundle_size = 10;        // 최대 UserOp 개수
      uint64_t max_bundle_gas = 12000000;   // 최대 가스 한도
      uint32_t min_bundle_size = 1;         // 최소 UserOp 개수
      uint32_t bundle_interval_ms = 5000;   // 번들 생성 주기 (ms)
   };

   bundle_object create_bundle(
      const vector<user_operation_object>& candidates,
      const bundle_config& config
   );

private:
   // 우선순위 정렬 (가스비 높은 순)
   vector<user_operation_object> sort_by_priority(
      const vector<user_operation_object>& user_ops
   );

   // 가스 한도 내에서 최대한 많이 선택
   vector<user_operation_object> select_optimal_batch(
      const vector<user_operation_object>& sorted_ops,
      uint64_t max_gas
   );

   // 충돌 검사 (같은 sender + nonce)
   bool has_conflicts(const vector<user_operation_object>& user_ops);
};
```

### 스케줄링 전략

#### 1. **Time-based Scheduling** (기본)
- 설정된 주기마다 번들 생성 (예: 5초마다)
- 안정적이고 예측 가능

```cpp
void bundler_plugin_impl::schedule_bundle_creation() {
   _bundle_timer.expires_from_now(
      boost::posix_time::milliseconds(_bundle_interval_ms)
   );

   _bundle_timer.async_wait([this](const boost::system::error_code& ec) {
      if (!ec) {
         try_create_and_submit_bundle();
         schedule_bundle_creation();  // 재스케줄
      }
   });
}
```

#### 2. **Threshold-based Scheduling** (최적화)
- 멤풀에 충분한 UserOp가 쌓이면 즉시 번들 생성
- 가스비가 높은 경우 더 빠른 처리 가능

```cpp
void bundler_plugin_impl::on_user_op_added() {
   uint32_t pending_count = get_pending_user_op_count();

   if (pending_count >= _min_bundle_size) {
      // 충분한 UserOp가 모였으면 즉시 번들 생성
      try_create_and_submit_bundle();
   }
}
```

#### 3. **Gas Price Adaptive Scheduling** (고급)
- EVM 네트워크 상태에 따라 제출 타이밍 조정
- 가스비가 낮을 때 대량 제출

```cpp
void bundler_plugin_impl::adaptive_schedule() {
   uint64_t current_gas_price = _evm_client.get_gas_price();

   if (current_gas_price < _gas_price_threshold) {
      // 가스비가 낮으면 즉시 제출
      try_create_and_submit_bundle();
   } else {
      // 가스비가 높으면 대기
      ilog("Gas price too high (${gp}), waiting...", ("gp", current_gas_price));
   }
}
```

### 재시도 로직

```cpp
class transaction_resubmitter {
public:
   struct retry_config {
      uint32_t max_retries = 3;
      uint32_t retry_interval_ms = 10000;
      double gas_price_multiplier = 1.2;  // 재제출 시 가스비 증가율
   };

   // 트랜잭션 재제출
   void resubmit_bundle(
      const bundle_object& bundle,
      const retry_config& config
   );

private:
   // 가스비 상향 조정
   string increase_gas_price(
      const string& original_tx,
      double multiplier
   );

   // 재시도 횟수 추적
   map<bundle_object::id_type, uint32_t> retry_counts_;
};
```

---

## 보안 고려사항

### 1. Private Key 관리

**위험:**
- 프라이빗 키 노출 시 번들러 계정 탈취

**대책:**
- [ ] 프라이빗 키를 메모리에만 보관 (디스크 저장 금지)
- [ ] 설정 파일 권한 제한 (chmod 600)
- [ ] 환경 변수나 Key Management System 사용 권장
- [ ] 프라이빗 키 암호화 저장 (옵션)

```cpp
// 예시: 환경 변수에서 키 로드
const char* key_env = std::getenv("BUNDLER_PRIVATE_KEY");
if (key_env) {
   _private_key = fc::ecc::private_key::regenerate(
      fc::sha256::hash(string(key_env))
   );
}
```

### 2. UserOperation 검증

**위험:**
- 악의적인 UserOperation으로 DoS 공격
- 잘못된 서명으로 번들 전체 실패

**대책:**
- [ ] 서명 검증 필수
- [ ] 가스 한도 상한선 설정
- [ ] 금지된 opcode 검사 (ERC-4337 규칙)
- [ ] Rate limiting (동일 sender)
- [ ] 멤풀 크기 제한

```cpp
bool validate_user_operation(const user_operation_object& user_op) {
   // 1. 서명 검증
   if (!verify_signature(user_op)) {
      return false;
   }

   // 2. 가스 한도 검사
   if (user_op.call_gas_limit > MAX_CALL_GAS_LIMIT) {
      return false;
   }

   // 3. Nonce 검사
   if (is_duplicate_nonce(user_op.sender, user_op.nonce)) {
      return false;
   }

   // 4. 만료 시간 검사
   if (user_op.expiration_time < fc::time_point_sec::now()) {
      return false;
   }

   return true;
}
```

### 3. EVM RPC 엔드포인트 보안

**위험:**
- 신뢰할 수 없는 RPC 노드
- Man-in-the-middle 공격

**대책:**
- [ ] HTTPS 사용 강제
- [ ] RPC 노드 인증 (API 키)
- [ ] 응답 검증 (서명, 해시 확인)
- [ ] 여러 RPC 노드 사용 (fallback)

### 4. DoS 방어

**위험:**
- 대량의 무효한 UserOperation으로 멤풀 포화

**대책:**
- [ ] Rate limiting (IP, sender 기준)
- [ ] 최소 가스비 요구
- [ ] 멤풀 크기 제한 (LRU 정책)
- [ ] 빠른 유효성 검사 (시뮬레이션 전)

---

## 테스트 계획

### 1. 단위 테스트 (Boost.Test)

**파일:** `tests/tests/bundler_tests.cpp`

```cpp
BOOST_AUTO_TEST_SUITE(bundler_tests)

BOOST_AUTO_TEST_CASE(user_operation_validation_test) {
   // UserOperation 검증 로직 테스트
}

BOOST_AUTO_TEST_CASE(bundle_creation_test) {
   // 번들 생성 알고리즘 테스트
}

BOOST_AUTO_TEST_CASE(gas_estimation_test) {
   // 가스 추정 테스트
}

BOOST_AUTO_TEST_CASE(signature_verification_test) {
   // 서명 검증 테스트
}

BOOST_AUTO_TEST_CASE(mempool_management_test) {
   // 멤풀 추가/제거/정렬 테스트
}

BOOST_AUTO_TEST_SUITE_END()
```

### 2. 통합 테스트

**환경 설정:**
- Hardhat 또는 Ganache로 로컬 EVM 체인 실행
- EntryPoint 컨트랙트 배포
- 테스트 스마트 컨트랙트 지갑 배포

**테스트 시나리오:**
1. **기본 플로우 테스트**
   - UserOperation 제출
   - 번들 생성
   - EVM 트랜잭션 제출
   - 상태 업데이트 확인

2. **에러 처리 테스트**
   - 잘못된 서명
   - 가스 부족
   - Nonce 충돌
   - 네트워크 오류

3. **부하 테스트**
   - 100개 UserOperation 동시 제출
   - 멤풀 성능 측정
   - 번들 처리 시간 측정

```python
# Python 통합 테스트 예시
import requests

def test_submit_user_op():
    user_op = {
        "sender": "0x1234...",
        "nonce": "0",
        "callData": "0xabcd...",
        # ... 기타 필드
    }

    response = requests.post(
        "http://localhost:8080/rpc",
        json={
            "jsonrpc": "2.0",
            "method": "bundler.submitUserOp",
            "params": user_op,
            "id": 1
        }
    )

    result = response.json()
    assert result["result"]["accepted"] == True
```

### 3. 성능 테스트

**메트릭:**
- UserOperation 처리량 (ops/sec)
- 평균 번들 생성 시간
- 평균 확인 시간
- 멤풀 메모리 사용량

**도구:**
- Apache JMeter 또는 Locust
- 커스텀 벤치마크 스크립트

---

## 배포 및 운영

### 설정 예시 (config.ini)

```ini
# Bundler Plugin Configuration
plugin = bundler
plugin = bundler_api

# EVM RPC Endpoint
bundler-evm-rpc-endpoint = https://mainnet.infura.io/v3/YOUR_API_KEY
bundler-chain-id = 1

# Private Key (또는 환경 변수 사용)
# bundler-private-key = 0x...

# EntryPoint Contract
bundler-entry-point = 0x5FF137D4b0FDCD49DcA30c7CF57E578a026d2789

# Bundle Configuration
bundler-max-bundle-size = 10
bundler-min-bundle-size = 1
bundler-bundle-interval = 5000  # milliseconds
bundler-max-bundle-gas = 12000000

# Gas Configuration
bundler-gas-price-multiplier = 1.2
bundler-max-gas-price = 200000000000  # 200 gwei
bundler-priority-fee = 2000000000     # 2 gwei

# Mempool Configuration
bundler-mempool-size = 1000
bundler-user-op-expiration = 3600  # seconds

# API Configuration
bundler-api-enabled = true

# Monitoring
bundler-metrics-enabled = true
```

### 시스템 요구사항

**하드웨어:**
- CPU: 4+ cores
- RAM: 8GB+
- 디스크: SSD 100GB+
- 네트워크: 안정적인 인터넷 연결

**소프트웨어:**
- Ubuntu 20.04 LTS 이상
- Boost 1.58-1.60
- CMake 3.10+
- GCC 7+ 또는 Clang 5+
- libcurl (EVM RPC 통신용)
- OpenSSL (서명용)

### 모니터링

**주요 메트릭:**
- `bundler.user_ops.submitted`: 제출된 UserOperation 수
- `bundler.user_ops.confirmed`: 확인된 UserOperation 수
- `bundler.user_ops.failed`: 실패한 UserOperation 수
- `bundler.bundles.created`: 생성된 번들 수
- `bundler.bundles.submitted`: 제출된 번들 수
- `bundler.mempool.size`: 현재 멤풀 크기
- `bundler.avg_confirmation_time`: 평균 확인 시간

**로그 레벨:**
- INFO: 번들 제출, UserOperation 확인
- WARN: 재시도, 가스비 경고
- ERROR: 트랜잭션 실패, 네트워크 오류
- DEBUG: 상세한 처리 과정

### 운영 절차

**시작:**
```bash
./steemd --data-dir=/path/to/data \
         --config-file=/path/to/config.ini \
         --plugin=bundler \
         --plugin=bundler_api
```

**프라이빗 키 관리:**
```bash
# 환경 변수로 프라이빗 키 전달
export BUNDLER_PRIVATE_KEY="0x..."
./steemd --plugin=bundler
```

**번들러 계정 충전:**
- 번들러 계정에 충분한 ETH 잔액 유지
- 자동 잔액 모니터링 및 알림 설정

**업그레이드:**
1. 새 버전 빌드
2. 멤풀의 pending UserOperation 처리 대기
3. Graceful shutdown
4. 새 버전으로 재시작

---

## 향후 개선 사항

### Phase 8: 고급 기능 (추후)

1. **멀티 체인 지원**
   - 여러 EVM 체인 동시 지원
   - 체인별 설정 및 계정 관리

2. **고급 Paymaster 지원**
   - ERC-20 토큰으로 가스비 지불
   - 구독 기반 Paymaster

3. **MEV 보호**
   - Private mempool 연동
   - Flashbots 통합

4. **대시보드 및 관리 도구**
   - Web UI로 멤풀 모니터링
   - UserOperation 상태 실시간 추적
   - 통계 및 분석 도구

5. **고급 배치 최적화**
   - AI 기반 가스비 예측
   - 동적 배치 크기 조정
   - 교차 체인 배치

---

## 참고 자료

### ERC-4337 스펙
- [ERC-4337 Specification](https://eips.ethereum.org/EIPS/eip-4337)
- [Account Abstraction Docs](https://docs.alchemy.com/docs/account-abstraction-overview)
- [Bundler Specification](https://github.com/eth-infinitism/bundler)

### Steem 개발 문서
- [Plugin Development Guide](../development/plugin.md)
- [Testing Guide](../development/testing.md)
- [CLAUDE.md](../../CLAUDE.md)

### EVM 통합
- [JSON-RPC API Reference](https://ethereum.org/en/developers/docs/apis/json-rpc/)
- [Web3 C++ Libraries](https://github.com/ethereum/wiki/wiki/JSON-RPC)
- [secp256k1 Library](https://github.com/bitcoin-core/secp256k1)

---

## 마일스톤 및 일정

| Phase | 작업 내용 | 예상 기간 | 의존성 |
|-------|----------|----------|--------|
| Phase 1 | 기본 인프라 구축 | 1-2주 | - |
| Phase 2 | EVM 통합 | 2-3주 | Phase 1 |
| Phase 3 | 멤풀 및 배치 관리 | 2주 | Phase 1, 2 |
| Phase 4 | API 구현 | 1-2주 | Phase 1, 3 |
| Phase 5 | 검증 및 시뮬레이션 | 2주 | Phase 2, 3 |
| Phase 6 | 상태 추적 및 모니터링 | 1주 | Phase 2, 3 |
| Phase 7 | 테스트 및 최적화 | 2-3주 | All phases |
| **Total** | | **11-15주** | |

---

## 팀 역할 분담 (제안)

- **Backend Developer**: 플러그인 코어 로직, 데이터 구조
- **Blockchain Engineer**: EVM 통합, 트랜잭션 제출, 서명
- **DevOps**: 배포, 모니터링, 인프라
- **QA Engineer**: 테스트 작성, 부하 테스트, 보안 감사

---

## 결론

본 문서는 Steem 블록체인에 Bundler 플러그인을 구현하기 위한 전체적인 계획을 제시합니다. ERC-4337 표준을 기반으로 하여 검증된 아키텍처를 따르며, Steem의 플러그인 시스템과 자연스럽게 통합됩니다.

구현 시 주요 고려사항:
- **보안 우선**: 프라이빗 키 관리, UserOperation 검증
- **확장성**: 높은 처리량을 위한 효율적인 배치 처리
- **안정성**: 에러 처리, 재시도 로직, 모니터링
- **유연성**: 다양한 설정 옵션, 여러 체인 지원 가능

단계별로 구현하고 테스트하면서 점진적으로 기능을 추가하는 것을 권장합니다.
