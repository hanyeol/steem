# 이더리움 트랜잭션 배치 처리 (Transaction Batching)

## 개요

이더리움의 트랜잭션 배치 처리는 여러 개의 트랜잭션을 하나로 묶어서 처리하는 기술로, 가스 비용 절감과 처리 효율성 향상을 목표로 합니다. 최신 이더리움 생태계에서는 여러 가지 배치 처리 방식이 존재합니다.

## 주요 배치 처리 기술

### 1. EIP-4844: Proto-Danksharding (Blob Transactions)

**도입 시기:** 2024년 3월 (Cancun-Deneb 업그레이드)

**핵심 개념:**
- L2 롤업(Rollup)을 위한 새로운 트랜잭션 타입 추가
- "Blob"이라는 대용량 데이터 저장 공간 제공 (최대 ~125KB per blob)
- 블록당 최대 6개의 blob 첨부 가능 (목표 3개)
- Blob 데이터는 약 18일 후 자동 삭제 (영구 저장 불필요)

**장점:**
- L2 롤업의 데이터 가용성(DA) 비용을 90% 이상 절감
- 기존 calldata 방식 대비 훨씬 저렴한 비용
- 이더리움 메인넷 확장성 크게 향상

**사용 사례:**
```
Type 3 Transaction (Blob Transaction)
- chainId: 1 (Ethereum Mainnet)
- maxFeePerGas: 50 gwei
- maxPriorityFeePerGas: 2 gwei
- maxFeePerBlobGas: 10 gwei
- blobVersionedHashes: [hash1, hash2, ...]
- blobs: [blob_data_1, blob_data_2, ...]
```

### 2. ERC-4337: Account Abstraction (UserOperation Bundling)

**도입 시기:** 2023년 3월 (프로토콜 변경 없이 스마트 컨트랙트로 구현)

**핵심 개념:**
- 사용자의 "의도(UserOperation)"를 Bundler가 수집하여 하나의 트랜잭션으로 실행
- EOA(Externally Owned Account) 없이도 스마트 컨트랙트 지갑 사용 가능
- 가스비 대납, 소셜 복구, 일괄 실행 등 고급 기능 지원

**구성 요소:**
- **UserOperation:** 사용자가 실행하고자 하는 작업
- **Bundler:** 여러 UserOperation을 모아서 하나의 트랜잭션으로 제출
- **EntryPoint Contract:** UserOperation을 검증하고 실행하는 싱글턴 컨트랙트
- **Smart Contract Wallet:** 사용자의 스마트 컨트랙트 기반 지갑

**UserOperation 구조:**
```solidity
struct UserOperation {
    address sender;              // 스마트 컨트랙트 지갑 주소
    uint256 nonce;
    bytes initCode;              // 지갑 생성 코드 (선택)
    bytes callData;              // 실행할 함수 호출 데이터
    uint256 callGasLimit;
    uint256 verificationGasLimit;
    uint256 preVerificationGas;
    uint256 maxFeePerGas;
    uint256 maxPriorityFeePerGas;
    bytes paymasterAndData;      // 가스비 대납자 정보 (선택)
    bytes signature;
}
```

**Bundler 처리 흐름:**
1. 멤풀에서 여러 UserOperation 수집
2. 시뮬레이션을 통해 유효성 검증
3. EntryPoint.handleOps()로 일괄 실행
4. 각 UserOperation의 성공/실패 독립적으로 처리

**장점:**
- 여러 사용자의 작업을 하나의 트랜잭션으로 처리 → 가스비 절감
- 프로토콜 변경 없이 구현 가능
- 가스비 대납(Paymaster), 배치 실행, 세션 키 등 고급 기능

### 3. Multicall Pattern

**핵심 개념:**
- 하나의 트랜잭션으로 여러 컨트랙트 호출을 순차 실행
- 스마트 컨트랙트 레벨에서 구현

**표준 Multicall 컨트랙트:**
```solidity
contract Multicall {
    struct Call {
        address target;
        bytes callData;
    }

    function aggregate(Call[] memory calls)
        public
        returns (uint256 blockNumber, bytes[] memory returnData)
    {
        blockNumber = block.number;
        returnData = new bytes[](calls.length);

        for(uint256 i = 0; i < calls.length; i++) {
            (bool success, bytes memory ret) = calls[i].target.call(calls[i].callData);
            require(success, "Multicall: call failed");
            returnData[i] = ret;
        }
    }
}
```

**사용 예시:**
```javascript
// 여러 ERC20 토큰 잔액을 한 번에 조회
const calls = [
  { target: token1.address, callData: token1.interface.encodeFunctionData('balanceOf', [user]) },
  { target: token2.address, callData: token2.interface.encodeFunctionData('balanceOf', [user]) },
  { target: token3.address, callData: token3.interface.encodeFunctionData('balanceOf', [user]) }
];

const [blockNumber, returnData] = await multicall.aggregate(calls);
```

**장점:**
- 단일 트랜잭션으로 여러 작업 실행
- RPC 호출 횟수 감소
- 원자성(Atomicity) 보장 - 하나라도 실패하면 전체 롤백

**Multicall3 개선사항:**
- 개별 호출 실패 시에도 계속 진행 옵션
- 값(value) 전송 지원
- 반환 데이터 없이 실행만 하는 옵션

### 4. Layer 2 Rollup Batching

**핵심 개념:**
- L2에서 수천 개의 트랜잭션을 처리하고 결과를 L1에 일괄 제출
- Optimistic Rollup과 ZK-Rollup 방식 존재

**Optimistic Rollup (Optimism, Arbitrum):**
```
L2 Transactions (1000개) → Sequencer가 배치로 묶음
    ↓
State Root 계산 및 L1 제출
    ↓
7일 챌린지 기간 (fraud proof)
    ↓
최종 확정
```

**ZK-Rollup (zkSync, StarkNet, Polygon zkEVM):**
```
L2 Transactions (1000개) → Sequencer가 배치로 묶음
    ↓
Zero-Knowledge Proof 생성 (SNARK/STARK)
    ↓
Proof + State Root L1 제출
    ↓
즉시 최종 확정
```

**배치 제출 데이터 (EIP-4844 이전):**
```
calldata에 압축된 트랜잭션 데이터 포함:
- from (3 bytes - 인덱스)
- to (3 bytes - 인덱스)
- value (compressed)
- data (compressed)
- signature (compressed)
```

**배치 제출 데이터 (EIP-4844 이후):**
```
Blob에 트랜잭션 데이터 저장:
- 최대 125KB의 트랜잭션 데이터
- calldata 대비 10-100배 저렴
- L1 가스비 대폭 절감
```

**가스비 절감 효과:**
```
단일 트랜잭션: 21,000 gas + execution cost
1000개 배치: (21,000 + batch_overhead) / 1000 ≈ 수백 gas per tx

실제 사례 (Optimism):
- EIP-4844 이전: $0.50 - $2.00 per tx
- EIP-4844 이후: $0.001 - $0.01 per tx (99% 절감)
```

## 배치 처리 비교

| 기술 | 레벨 | 배치 크기 | 가스 절감 | 사용 사례 |
|------|------|----------|----------|----------|
| **EIP-4844** | L1 프로토콜 | ~125KB per blob | 90%+ (L2 DA 비용) | L2 롤업 데이터 게시 |
| **ERC-4337** | 스마트 컨트랙트 | 수십~수백 UserOps | 30-50% | 지갑 추상화, 가스비 대납 |
| **Multicall** | 스마트 컨트랙트 | 수십 calls | 20-40% | DApp 내 여러 호출 통합 |
| **L2 Rollup** | L2 프로토콜 | 수천~수만 txs | 95-99% | 대규모 확장성 솔루션 |

## 실제 구현 예시

### ERC-4337 Bundler 구현 (간소화)

```typescript
class Bundler {
  private mempool: UserOperation[] = [];

  // UserOperation 수신
  async submitUserOp(userOp: UserOperation): Promise<string> {
    // 기본 검증
    await this.validateUserOp(userOp);

    // 멤풀에 추가
    this.mempool.push(userOp);

    return this.getUserOpHash(userOp);
  }

  // 배치 생성 및 제출
  async createBundle(): Promise<string> {
    const bundle = this.selectUserOps(this.mempool, 10); // 최대 10개 선택

    // 시뮬레이션
    for (const userOp of bundle) {
      const valid = await this.simulateUserOp(userOp);
      if (!valid) {
        this.mempool = this.mempool.filter(op => op !== userOp);
      }
    }

    // EntryPoint를 통해 일괄 실행
    const tx = await this.entryPoint.handleOps(
      bundle,
      this.bundlerAddress  // 가스비 수령자
    );

    return tx.hash;
  }

  private selectUserOps(mempool: UserOperation[], maxCount: number): UserOperation[] {
    // 가스비가 높은 순서로 선택
    return mempool
      .sort((a, b) => b.maxPriorityFeePerGas - a.maxPriorityFeePerGas)
      .slice(0, maxCount);
  }
}
```

### Multicall3 사용 예시

```solidity
// SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;

interface IMulticall3 {
    struct Call3 {
        address target;
        bool allowFailure;
        bytes callData;
    }

    struct Result {
        bool success;
        bytes returnData;
    }

    function aggregate3(Call3[] calldata calls)
        external
        payable
        returns (Result[] memory returnData);
}

contract BatchProcessor {
    IMulticall3 constant multicall = IMulticall3(0xcA11bde05977b3631167028862bE2a173976CA11);

    function batchTransfer(
        address[] calldata tokens,
        address[] calldata recipients,
        uint256[] calldata amounts
    ) external {
        require(tokens.length == recipients.length && recipients.length == amounts.length);

        IMulticall3.Call3[] memory calls = new IMulticall3.Call3[](tokens.length);

        for (uint256 i = 0; i < tokens.length; i++) {
            calls[i] = IMulticall3.Call3({
                target: tokens[i],
                allowFailure: false,
                callData: abi.encodeWithSignature(
                    "transferFrom(address,address,uint256)",
                    msg.sender,
                    recipients[i],
                    amounts[i]
                )
            });
        }

        multicall.aggregate3(calls);
    }
}
```

### EIP-4844 Blob 트랜잭션 제출 예시

```python
from web3 import Web3
from eth_account import Account

# Blob 트랜잭션 생성
def create_blob_transaction(w3, account, blob_data):
    # Blob 데이터 준비 (4096 필드 요소, 각 32바이트)
    blob = prepare_blob(blob_data)  # 패딩 및 포맷팅

    # KZG 커밋먼트 생성
    commitment = compute_kzg_commitment(blob)
    versioned_hash = kzg_to_versioned_hash(commitment)

    # Type 3 트랜잭션 생성
    tx = {
        'type': 3,  # Blob transaction
        'chainId': 1,
        'nonce': w3.eth.get_transaction_count(account.address),
        'maxFeePerGas': w3.eth.gas_price * 2,
        'maxPriorityFeePerGas': w3.eth.max_priority_fee,
        'maxFeePerBlobGas': w3.eth.get_blob_base_fee() * 2,
        'gas': 21000,
        'to': '0x...',  # L2 Batch Inbox 주소
        'value': 0,
        'data': b'',
        'blobVersionedHashes': [versioned_hash],
        'blobs': [blob],
        'commitments': [commitment],
        'proofs': [compute_blob_kzg_proof(blob, commitment)]
    }

    # 서명 및 전송
    signed_tx = account.sign_transaction(tx)
    tx_hash = w3.eth.send_raw_transaction(signed_tx.rawTransaction)

    return tx_hash
```

## 가스비 절감 예시

### 시나리오: 100명에게 토큰 전송

**방법 1: 개별 트랜잭션**
```
가스비 = 100 txs × (21,000 + 65,000) gas = 8,600,000 gas
비용 (50 gwei) = 8,600,000 × 50 × 10^-9 × $3,000 = $1,290
```

**방법 2: Multicall 배치**
```
가스비 = 21,000 + (100 × 50,000) = 5,021,000 gas
비용 (50 gwei) = 5,021,000 × 50 × 10^-9 × $3,000 = $753
절감: 41.6%
```

**방법 3: ERC-4337 Bundler**
```
가스비 = 21,000 + 검증(100 × 20,000) + 실행(100 × 50,000) = 7,021,000 gas
비용 분담: $1,053 / 100명 = $10.53/인
개인 부담: 개별 대비 18% 절감
```

**방법 4: L2 Rollup (Optimism with EIP-4844)**
```
L2 실행: 100 txs × 500 gas (L2 가스) = 50,000 gas
L1 DA 비용: 1 blob × 0.01 ETH = 0.01 ETH
총 비용: ~$30 (99% 절감)
개인 부담: $0.30/인
```

## 베스트 프랙티스

### 1. Bundler 운영 시 고려사항

**멤풀 관리:**
- UserOperation의 우선순위 큐 유지
- 가스비가 너무 낮은 작업은 거부
- 멤풀 크기 제한 설정

**시뮬레이션:**
- 번들 생성 전 모든 UserOperation 시뮬레이션
- 실패할 작업은 제거하여 전체 번들 실패 방지
- 스토리지 액세스 규칙 검증

**가스 관리:**
- 동적 가스비 조정 (EIP-1559)
- 번들 크기 최적화로 블록 가스 한도 초과 방지
- Paymaster 가스 대납 처리

### 2. Blob 트랜잭션 사용 시

**비용 최적화:**
- Blob 공간 최대 활용 (125KB 가까이)
- 데이터 압축 (zlib, brotli)
- 배치 주기 최적화 (너무 자주 제출하지 않기)

**안정성:**
- `maxFeePerBlobGas` 적절히 설정
- Blob 가스 시장 모니터링
- 제출 실패 시 재시도 로직

### 3. Multicall 패턴

**안전성:**
- allowFailure 옵션 적절히 사용
- 각 호출의 반환값 검증
- 가스 한도 충분히 설정

**효율성:**
- 관련 있는 호출만 배치로 묶기
- 순서에 의존성이 있는 호출 주의
- View 함수 호출은 eth_call로 직접

## 미래 발전 방향

### Full Danksharding (EIP-4844 이후)

- 블록당 Blob 수를 16MB까지 확장
- 목표: 100,000 TPS (L2 합산)
- 데이터 샤딩을 통한 추가 확장성

### Native Account Abstraction (EIP-7702)

- EOA를 스마트 컨트랙트처럼 동작하게 하는 기능
- ERC-4337보다 더 효율적인 배치 처리
- 프로토콜 레벨 지원

### Cross-L2 Interoperability

- 여러 L2 간 원자적 배치 실행
- Shared Sequencer를 통한 통합 배치 처리
- L2 간 브리징 비용 대폭 절감

## 참고 자료

- [EIP-4844: Shard Blob Transactions](https://eips.ethereum.org/EIPS/eip-4844)
- [ERC-4337: Account Abstraction](https://eips.ethereum.org/EIPS/eip-4337)
- [Multicall3 Contract](https://github.com/mds1/multicall)
- [Optimism Bedrock Architecture](https://community.optimism.io/docs/developers/bedrock/)
- [zkSync Era Documentation](https://era.zksync.io/docs/)
- [Bundler Reference Implementation](https://github.com/eth-infinitism/bundler)

## 결론

이더리움의 트랜잭션 배치 처리는 블록체인 확장성 문제를 해결하는 핵심 기술입니다.

- **EIP-4844**는 L2 롤업의 데이터 비용을 획기적으로 줄였습니다.
- **ERC-4337**은 사용자 경험을 개선하고 배치 처리를 통해 가스비를 절감합니다.
- **Multicall**은 DApp에서 즉시 적용 가능한 간단하고 효과적인 방법입니다.
- **L2 Rollup**은 대규모 배치 처리를 통해 가장 큰 비용 절감을 제공합니다.

이러한 기술들을 적절히 조합하여 사용하면, 사용자에게 저렴하고 빠른 블록체인 경험을 제공할 수 있습니다.
