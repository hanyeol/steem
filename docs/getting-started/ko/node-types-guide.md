# Steem 노드 타입 가이드

다양한 타입의 Steem 노드와 그 구성에 대한 완전한 참조 가이드입니다.

## 개요

Steem 노드는 특정 플러그인을 활성화하고 리소스 할당을 조정하여 다양한 목적으로 구성할 수 있습니다. 이 가이드는 주요 노드 타입, 사용 사례 및 구성 방법을 설명합니다.

**핵심 원칙**: 노드의 타입은 바이너리 자체가 아닌 **플러그인 구성**에 의해 결정됩니다. 동일한 `steemd` 바이너리를 어떤 노드 타입으로도 구성할 수 있습니다.

## 노드 타입 비교

| 노드 타입 | 메모리 | 사용 사례 | API | 블록 생성 |
|-----------|--------|----------|-----|----------|
| **증인 노드** | 54GB | 블록 생성 | 최소 | 예 |
| **시드 노드** | 4GB | P2P 중계 | 없음 | 아니오 |
| **전체 노드** | 260GB+ | 공개 API | 전체 | 아니오 |
| **계정 기록 노드** | 70GB+ | 계정 쿼리 | 계정 기록 | 아니오 |
| **브로드캐스트 노드** | 24GB | 거래 제출 | 네트워크 브로드캐스트 | 아니오 |
| **테스트넷 노드** | 30GB | 개발/테스팅 | 전체 | 예 (제네시스) |

## 증인 노드

**목적**: Steem 블록체인의 블록을 생성하고 거래를 검증합니다.

### 특징

- **주요 기능**: 블록 생성 및 검증
- **메모리**: 54GB 공유 메모리
- **플러그인**: 최소 (witness, 기본 API)
- **네트워크**: 블록 전파를 위한 P2P
- **신뢰성**: 미션 크리티컬, 24/7 가동 필요

### 구성

**설정 파일**: [contrib/docker.config.ini](../contrib/docker.config.ini)

```ini
# 로깅
log-console-appender = {"appender":"stderr","stream":"std_error"}
log-logger = {"name":"default","level":"info","appender":"stderr"}

# 플러그인 - 합의를 위한 최소 세트
plugin = chain p2p webserver witness
plugin = database_api witness_api

# 저장소
shared-file-size = 54G
shared-file-dir = blockchain
flush-state-interval = 0

# 네트워크
p2p-endpoint = 0.0.0.0:2001
p2p-seed-node = seed.steem.com:2001

# 증인 설정
enable-stale-production = false
required-participation = 33
witness = "your-witness-account"
private-key = 5K...  # 증인 서명 키
```

### 요구사항

- **하드웨어**:
  - CPU: 4코어 이상
  - RAM: 64GB 이상 (54GB 공유 메모리 + 시스템 오버헤드)
  - 저장소: 100GB 이상 SSD
  - 네트워크: 안정적이고 저지연 연결

- **소프트웨어**:
  - 빌드: 표준 빌드 (`BUILD_STEEM_TESTNET=OFF`)
  - OS: Linux (Ubuntu 20.04/22.04 권장)

### 사용 사례

- **블록 생성**: 예정된 시간에 블록 서명 및 생성
- **거래 검증**: 거래 검증 및 브로드캐스트
- **합의 참여**: 블록체인 파라미터에 투표

### 모범 사례

1. **이중화**: 별도의 인프라에 백업 증인 운영
2. **모니터링**: 누락된 블록, 높은 지연 시간에 대한 알림
3. **보안**: P2P(2001)를 제외한 모든 포트 방화벽 설정
4. **키 관리**: 서명 키를 안전하게 저장 (하드웨어 지갑/HSM)
5. **가격 피드**: SBD 가격 피드 정기 업데이트 (상위 20 증인의 경우)

### Docker 배포

```bash
docker run -d \
  --name steem-witness \
  -v witness-data:/var/lib/steemd \
  -v $(pwd)/witness.config.ini:/var/lib/steemd/config.ini:ro \
  -p 2001:2001 \
  --restart=unless-stopped \
  hanyeol/steem:latest
```

## 시드 노드

**목적**: 전체 상태를 저장하지 않고 P2P 연결 및 블록 중계를 제공합니다.

### 특징

- **주요 기능**: P2P 네트워크 중계
- **메모리**: 4GB 공유 메모리 (LOW_MEMORY_NODE 빌드)
- **플러그인**: P2P만, API 없음
- **빌드**: `LOW_MEMORY_NODE=ON` 필요
- **최소 리소스 사용**

### 구성

```ini
# 로깅 (최소)
log-console-appender = {"appender":"stderr","stream":"std_error"}
log-logger = {"name":"default","level":"warn","appender":"stderr"}

# 플러그인 - P2P만
plugin = chain p2p

# 저장소 - 최소
shared-file-size = 4G

# 네트워크
p2p-endpoint = 0.0.0.0:2001
p2p-max-connections = 200
p2p-seed-node = seed1.example.com:2001
p2p-seed-node = seed2.example.com:2001
```

### 빌드 요구사항

`LOW_MEMORY_NODE=ON`으로 빌드해야 합니다:

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DLOW_MEMORY_NODE=ON \
      ..
make -j$(nproc) steemd
```

### 사용 사례

- **P2P 부트스트래핑**: 새로운 노드가 피어를 발견하도록 지원
- **블록 중계**: 네트워크 전체에 블록 전파
- **지리적 분산**: 다양한 지역에서 저지연 피어 제공

### 모범 사례

1. **높은 대역폭**: 충분한 업로드 용량 확보
2. **많은 연결**: `p2p-max-connections`를 높게 설정 (200+)
3. **글로벌 분산**: 여러 지역에 배포
4. **최소 로깅**: 디스크 I/O 감소

## 전체 노드 (전체 API 노드)

**목적**: 애플리케이션, 거래소 및 탐색기를 위한 완전한 API 액세스를 제공합니다.

### 특징

- **주요 기능**: API를 통해 모든 블록체인 데이터 제공
- **메모리**: 260GB+ 공유 메모리 (지속적으로 증가)
- **플러그인**: 모든 상태 추적 및 API 플러그인
- **네트워크**: API 엔드포인트 (WebSocket/HTTP)
- **높은 리소스 요구사항**

### 구성

**설정 파일**: [contrib/fullnode.config.ini](../contrib/fullnode.config.ini)

```ini
# 로깅
log-console-appender = {"appender":"stderr","stream":"std_error"}
log-logger = {"name":"default","level":"debug","appender":"stderr"}

# 모든 상태 추적 플러그인
plugin = webserver p2p json_rpc witness
plugin = account_by_key tags follow market_history

# 모든 API 플러그인
plugin = database_api account_by_key_api network_broadcast_api
plugin = tags_api follow_api market_history_api witness_api
plugin = condenser_api block_api

# 저장소 - 대용량
shared-file-size = 260G

# 네트워크
p2p-endpoint = 0.0.0.0:2001
webserver-http-endpoint = 0.0.0.0:8090
webserver-ws-endpoint = 0.0.0.0:8090
webserver-thread-pool-size = 256

# 최적화
follow-max-feed-size = 500
market-history-bucket-size = [15,60,300,3600,86400]
market-history-buckets-per-size = 5760
```

### 요구사항

- **하드웨어**:
  - CPU: 8코어 이상
  - RAM: 300GB 이상 (260GB 공유 메모리 + OS + 버퍼)
  - 저장소: 400GB 이상 SSD
  - 네트워크: API 트래픽을 위한 높은 대역폭

- **소프트웨어**:
  - 빌드: 표준 빌드
  - 리버스 프록시: 프로덕션에서는 NGINX/HAProxy 권장

### 활성화된 API

- `database_api` - 핵심 블록체인 데이터
- `account_by_key_api` - 공개 키로 계정 조회
- `network_broadcast_api` - 거래 브로드캐스팅
- `tags_api` - 콘텐츠 태그 및 트렌딩
- `follow_api` - 팔로우 관계 및 피드
- `market_history_api` - 내부 시장 OHLCV 데이터
- `witness_api` - 증인 정보
- `condenser_api` - 레거시 API (하위 호환성)
- `block_api` - 블록 데이터 쿼리
- `account_history_api` - 계정 거래 기록

### 사용 사례

- **애플리케이션 백엔드**: 소셜 미디어 dapp 구동
- **블록체인 탐색기**: 전체 블록체인 데이터 표시
- **분석 플랫폼**: 과거 데이터 쿼리
- **콘텐츠 플랫폼**: 게시물, 댓글, 투표 제공

### 모범 사례

1. **리버스 프록시 사용**: 속도 제한, 캐싱, TLS를 위한 NGINX
2. **리소스 사용 모니터링**: RAM, 디스크, 네트워크 감시
3. **정기 백업**: 공유 메모리 및 블록 로그 백업
4. **속도 제한**: API 남용 방지
5. **로드 밸런싱**: 높은 트래픽을 위해 로드 밸런서 뒤에 여러 노드 배치

### Docker 배포

```bash
docker run -d \
  --name steem-fullnode \
  -e USE_FULL_WEB_NODE=true \
  -v fullnode-data:/var/lib/steemd \
  -p 8090:8090 \
  -p 2001:2001 \
  --shm-size=280g \
  --restart=unless-stopped \
  hanyeol/steem:latest
```

## 계정 기록 노드

**목적**: 계정 거래 기록 쿼리를 위한 전문 노드입니다.

### 특징

- **주요 기능**: 계정 거래 기록 쿼리
- **메모리**: 70GB+ 공유 메모리
- **플러그인**: `account_history_rocksdb` (디스크 기반, 더 효율적)
- **저장소**: 전체 노드 대비 디스크 사용량 높음, 메모리 사용량 낮음

### 구성

**설정 파일**: [contrib/ahnode.config.ini](../contrib/ahnode.config.ini)

```ini
# 로깅
log-console-appender = {"appender":"stderr","stream":"std_error"}
log-logger = {"name":"default","level":"debug","appender":"stderr"}

# 플러그인 - 계정 기록 중심
plugin = webserver p2p json_rpc account_history_rocksdb
plugin = database_api account_history_api condenser_api

# 저장소
shared-file-size = 70G

# 네트워크
webserver-http-endpoint = 0.0.0.0:8090
webserver-ws-endpoint = 0.0.0.0:8090
webserver-thread-pool-size = 32

# 계정 기록 구성
# 모든 계정 추적 (기본값)
# account-history-track-account-range = ["a", "z"]

# 또는 특정 계정만 추적
# account-history-track-account-range = ["alice", "alice"]
# account-history-track-account-range = ["bob", "bob"]
```

### 플러그인 비교

**기존 account_history** (메모리 기반):
- 모든 데이터를 공유 메모리에 저장
- 더 빠른 쿼리
- 더 높은 메모리 사용량 (~200GB+)

**account_history_rocksdb** (디스크 기반):
- 데이터를 디스크의 RocksDB에 저장
- 약간 느린 쿼리
- 훨씬 낮은 메모리 사용량 (~70GB)
- 대부분의 사용 사례에 권장

### 선택적 추적

저장소를 줄이기 위해 특정 작업만 추적:

```ini
# 화이트리스트 - 이러한 작업만 추적
account-history-whitelist-ops = transfer_operation
account-history-whitelist-ops = transfer_to_vesting_operation
account-history-whitelist-ops = withdraw_vesting_operation
account-history-whitelist-ops = claim_reward_balance_operation

# 블랙리스트 - 이러한 작업 무시
account-history-blacklist-ops = vote_operation
account-history-blacklist-ops = custom_json_operation
```

### 사용 사례

- **거래소 통합**: 입출금 기록 추적
- **지갑 백엔드**: 사용자 거래 기록 표시
- **계정 분석**: 계정 활동 분석
- **컴플라이언스/감사**: 금융 거래 추적

### 모범 사례

1. **RocksDB 플러그인 사용**: 메모리 낮음, 유사한 성능
2. **선택적 추적**: 거래소의 경우 필요한 작업만 화이트리스트에 등록
3. **SSD 저장소**: RocksDB는 빠른 I/O의 이점
4. **정기 압축**: RocksDB는 주기적인 압축 필요

## 브로드캐스트 노드

**목적**: 거래 브로드캐스팅 전용 경량 노드입니다.

### 특징

- **주요 기능**: 거래 수락 및 브로드캐스트
- **메모리**: 24GB 공유 메모리 (최소)
- **플러그인**: `network_broadcast_api`만
- **경량**: 가장 낮은 리소스 요구사항

### 구성

**설정 파일**: [contrib/broadcaster.config.ini](../contrib/broadcaster.config.ini)

```ini
# 로깅
log-console-appender = {"appender":"stderr","stream":"std_error"}
log-logger = {"name":"default","level":"info","appender":"stderr"}

# 최소 플러그인
plugin = webserver p2p json_rpc
plugin = database_api network_broadcast_api

# 저장소 - 최소
shared-file-size = 24G

# 네트워크
webserver-http-endpoint = 0.0.0.0:8090
webserver-ws-endpoint = 0.0.0.0:8090
```

### 사용 사례

- **대량 거래 제출**: 거래소, 봇
- **비용 효율적인 브로드캐스팅**: 최소 인프라
- **거래 중계**: 네트워크에 거래 전달

### 모범 사례

1. **로드 밸런싱**: 이중화를 위한 여러 브로드캐스트 노드
2. **모니터링**: 거래 성공률 추적
3. **속도 제한**: 스팸/남용 방지

## 테스트넷 노드

**목적**: 프라이빗 또는 퍼블릭 테스트넷에서의 개발 및 테스팅입니다.

### 특징

- **주요 기능**: 개발 환경
- **메모리**: 30GB 공유 메모리
- **플러그인**: 테스팅을 위한 전체 플러그인 세트
- **빌드**: `BUILD_STEEM_TESTNET=ON` 필요

### 구성

**설정 파일**: [contrib/testnet.config.ini](../contrib/testnet.config.ini)

```ini
# 로깅 - 디버깅을 위한 상세 로그
log-console-appender = {"appender":"stderr","stream":"std_error"}
log-logger = {"name":"default","level":"debug","appender":"stderr"}

# 전체 플러그인 세트
plugin = webserver p2p json_rpc witness account_by_key tags follow market_history account_history
plugin = database_api account_by_key_api network_broadcast_api tags_api follow_api market_history_api witness_api condenser_api account_history_api

# 저장소
shared-file-size = 30G

# 네트워크
webserver-http-endpoint = 0.0.0.0:8090
webserver-ws-endpoint = 0.0.0.0:8090

# 테스트넷 전용 설정
enable-stale-production = true
required-participation = 0

# 제네시스 증인
witness = "genesis"
private-key = 5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n
```

### 빌드 요구사항

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_STEEM_TESTNET=ON \
      ..
make -j$(nproc) steemd
```

### 사용 사례

- **애플리케이션 개발**: 실제 STEEM 없이 앱 테스트
- **프로토콜 테스팅**: 하드포크 및 업그레이드 테스트
- **플러그인 개발**: 커스텀 플러그인 테스트
- **통합 테스팅**: CI/CD 파이프라인

### 모범 사례

1. **별도 인프라**: 테스트넷과 메인넷 혼합 금지
2. **제네시스 구성**: 특정 테스트를 위한 제네시스 커스터마이징
3. **정기적인 리셋**: 깨끗한 테스트를 위한 새로운 테스트넷
4. **모의 데이터**: 현실적인 테스트 데이터로 채우기

## 특수 노드 구성

### 거래소 노드

암호화폐 거래소 운영을 위해 최적화되었습니다.

```ini
# 플러그인 - 전송 및 계정 기록 중심
plugin = webserver p2p json_rpc account_history_rocksdb
plugin = database_api account_history_api network_broadcast_api condenser_api

# 거래소 계정만 추적
account-history-track-account-range = ["exchange-deposit", "exchange-deposit"]
account-history-track-account-range = ["exchange-hot", "exchange-hot"]
account-history-track-account-range = ["exchange-cold", "exchange-cold"]

# 금융 작업만 화이트리스트
account-history-whitelist-ops = transfer_operation
account-history-whitelist-ops = transfer_to_vesting_operation
account-history-whitelist-ops = withdraw_vesting_operation
account-history-whitelist-ops = transfer_to_savings_operation
account-history-whitelist-ops = transfer_from_savings_operation
account-history-whitelist-ops = fill_vesting_withdraw_operation

# 저장소
shared-file-size = 70G
```

자세한 거래소 통합은 [docs/getting-started/exchange-quick-start.md](../exchange-quick-start.md)를 참조하세요.

### 콘텐츠 플랫폼 노드

소셜 미디어 애플리케이션을 위해 최적화되었습니다.

```ini
# 플러그인 - 콘텐츠 및 소셜 기능 중심
plugin = webserver p2p json_rpc
plugin = tags follow account_by_key
plugin = database_api tags_api follow_api account_by_key_api network_broadcast_api condenser_api

# 팔로우 플러그인 설정
follow-max-feed-size = 500
follow-start-feeds = 0  # 제네시스부터 피드 추적

# 저장소
shared-file-size = 200G
```

### 분석 노드

블록체인 데이터 분석을 위해 최적화되었습니다.

```ini
# 완전한 데이터를 위한 모든 플러그인
plugin = webserver p2p json_rpc
plugin = account_history_rocksdb tags follow market_history
plugin = database_api account_history_api tags_api follow_api market_history_api block_api

# 시장 기록 - 전체 해상도
market-history-bucket-size = [15,60,300,3600,86400]
market-history-buckets-per-size = 10000  # 더 많은 기록

# 계정 기록 - 모든 작업
# (화이트리스트/블랙리스트 없음)

# 저장소
shared-file-size = 150G
```

## 노드 선택 가이드

### 노드 타입 선택

**질문**: 주요 사용 사례는 무엇입니까?

1. **증인으로 블록을 생성하고 싶습니다**
   → **증인 노드** (54GB RAM, 높은 신뢰성)

2. **공개 API 액세스를 제공하고 싶습니다**
   → **전체 노드** (300GB+ RAM, 리버스 프록시 권장)

3. **거래소를 운영하며 입출금 추적이 필요합니다**
   → 선택적 추적이 있는 **계정 기록 노드** (70GB RAM)

4. **P2P 네트워크에서 블록을 중계해야 합니다**
   → **시드 노드** (4GB RAM, 특수 빌드 필요)

5. **많은 거래를 브로드캐스트해야 합니다**
   → **브로드캐스트 노드** (24GB RAM, 경량)

6. **애플리케이션을 개발/테스팅하고 있습니다**
   → **테스트넷 노드** (30GB RAM, 특수 빌드)


### 리소스 계획

| 노드 타입 | CPU | RAM | 저장소 | 네트워크 |
|-----------|-----|-----|---------|---------|
| 증인 | 4코어 이상 | 64GB | 100GB SSD | 안정적, 저지연 |
| 시드 | 2코어 이상 | 8GB | 50GB | 높은 대역폭 |
| 전체 노드 | 8코어 이상 | 300GB | 400GB SSD | 높은 대역폭 |
| 계정 기록 | 4코어 이상 | 80GB | 300GB SSD | 중간 대역폭 |
| 브로드캐스트 | 2코어 이상 | 32GB | 100GB | 중간 대역폭 |
| 테스트넷 | 4코어 이상 | 40GB | 100GB | 낮은 대역폭 |

### 확장 전략

**수평 확장** (권장):
- 여러 전문 노드 실행
- 로드 밸런서를 사용하여 트래픽 분산
- 예: 2x 전체 노드 + 1x 계정 기록 노드 + 1x 브로드캐스트 노드

**수직 확장**:
- 단일 전기능 노드
- 고성능 하드웨어 필요 (32코어 이상, 512GB+ RAM)
- 더 비싸고 단일 장애 지점

## 노드 타입 간 마이그레이션

### 노드 구성 변경

플러그인 변경은 플러그인 타입에 따라 다른 접근 방식이 필요합니다:

**API 전용 플러그인** (리플레이 불필요):
- `database_api`, `witness_api`, `network_broadcast_api` 등
- 구성에서 추가/제거하고 재시작만 하면 됨

**상태 추적 플러그인** (리플레이 필요):
- `account_history`, `tags`, `follow`, `market_history`
- 전체 블록체인 리플레이 필요

### 리플레이 프로세스

```bash
# 노드 중지
sudo systemctl stop steemd

# 새 플러그인으로 config.ini 업데이트
vim witness_node_data_dir/config.ini

# 블록체인 리플레이 (몇 시간/며칠 소요될 수 있음)
./programs/steemd/steemd --replay-blockchain --data-dir=witness_node_data_dir

# 또는 상태를 삭제하고 P2P에서 재동기화
rm -rf witness_node_data_dir/blockchain
./programs/steemd/steemd --data-dir=witness_node_data_dir
```

### 다운그레이드 예: 전체 노드 → 증인 노드

```bash
# 1. 현재 상태 백업 (선택 사항)
cp -r witness_node_data_dir/blockchain /backup/

# 2. config.ini 업데이트 - 불필요한 플러그인 제거
# 이전:
# plugin = tags follow market_history account_history
# 이후:
# (위 줄 제거)

# 3. 재시작 (플러그인 제거 시 리플레이 불필요)
sudo systemctl restart steemd
```

### 업그레이드 예: 증인 노드 → 전체 노드

```bash
# 1. 노드 중지
sudo systemctl stop steemd

# 2. config.ini 업데이트 - 플러그인 추가
# plugin = tags follow market_history account_history
# plugin = tags_api follow_api market_history_api account_history_api

# 3. shared-file-size 증가
# shared-file-size = 260G

# 4. 블록체인 리플레이 (상태 추적 플러그인에 필수)
./programs/steemd/steemd --replay-blockchain --data-dir=witness_node_data_dir
```

## 모니터링 및 유지보수

### 노드 타입별 주요 메트릭

**모든 노드**:
- 블록 높이 (네트워크와 일치해야 함)
- P2P 연결 (5-30개 활성)
- 메모리 사용량
- 디스크 I/O

**증인 노드**:
- 누락된 블록 (0이어야 함)
- 블록 생성 시간 (슬롯 내)
- 피어에 대한 네트워크 지연

**전체 노드 / API 노드**:
- API 요청 속도
- API 응답 시간
- WebSocket 연결
- 쿼리 큐 깊이

### 헬스 체크

```bash
# 동기화 상태 확인
curl -s -X POST http://localhost:8090 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_dynamic_global_properties"}' \
  | jq -r '.result | "\(.head_block_number) \(.time)"'

# 플러그인 상태 확인
curl -s http://localhost:8090 | jq

# 로그 모니터링
tail -f witness_node_data_dir/logs/steemd.log

# 리소스 사용량 확인
docker stats steem-node
# 또는
htop
```

### 백업 전략

**증인 노드**:
- 서명 키 백업 (중요)
- config.ini 백업
- 선택 사항: 빠른 복구를 위한 공유 메모리 파일 백업

**전체 노드 / API 노드**:
- 공유 메모리 파일의 정기 스냅샷
- RocksDB 데이터 백업 (account_history_rocksdb 사용 시)
- config.ini 백업

**모든 노드**:
- 블록 로그는 재생 가능 (네트워크에서 재동기화 가능)
- 공유 메모리 파일은 리플레이를 통해 재생성 가능

## 추가 리소스

- [Steem 빌드하기](building.md) - 빌드 지침
- [체인 구성](chain-configuration.md) - 상세한 config.ini 참조
- [제네시스 런치 가이드](genesis-launch.md) - 프라이빗 네트워크 시작
- [리버스 프록시 가이드](reverse-proxy-guide.md) - NGINX/HAProxy 설정
- [거래소 빠른 시작](exchange-quick-start.md) - 거래소 통합
- [예제 설정](../../contrib/) - 바로 사용 가능한 구성 파일

## 라이선스

[LICENSE.md](../../LICENSE.md)를 참조하세요.
