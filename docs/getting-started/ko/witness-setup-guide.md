# 증인 노드 설정 가이드

새로운 증인 노드를 설정하고 기존 Steem 블록체인 네트워크에 연결하는 방법에 대한 완전한 가이드입니다.

## 개요

이 가이드는 다음 내용을 다룹니다:
- 증인 노드 빌드 및 설정
- 기존 블록체인 네트워크 연결
- 증인으로 등록
- 증인 운영 모범 사례

**사전 요구사항**:
- Steem 블록체인이 이미 실행 중 (메인넷 또는 프라이빗 네트워크)
- P2P 연결을 위한 시드 노드 주소
- 블록체인에 생성된 증인 계정
- 증인 서명 키 쌍 생성

## 아키텍처 개요

```
기존 네트워크             새로운 증인 노드
┌───────────────┐         ┌──────────────────┐
│  시드 노드    │◄────────┤   P2P 플러그인   │
│  (P2P 동기화) │         │                  │
└───────────────┘         │   체인 플러그인  │
                          │  (블록 저장소)   │
┌───────────────┐         │                  │
│ 다른 증인     │◄────────┤  증인 플러그인   │
│   노드들      │         │  (블록 서명)     │
└───────────────┘         └──────────────────┘
```

## 단계 1: 증인 노드 빌드

### 표준 빌드

증인 운영을 위한 `steemd` 바이너리를 빌드합니다 (`LOW_MEMORY_NODE`나 테스트넷 빌드를 사용하지 마세요):

```bash
cd /path/to/steem
git submodule update --init --recursive

# 빌드 디렉토리 생성
mkdir -p build && cd build

# 프로덕션 증인용 빌드 설정
cmake -DCMAKE_BUILD_TYPE=Release \
      -DLOW_MEMORY_NODE=OFF \
      -DBUILD_STEEM_TESTNET=OFF \
      ..

# steemd 빌드
make -j$(nproc) steemd

# 바이너리 확인
./programs/steemd/steemd --version
```

### 빌드 참고사항

- **사용 금지** `LOW_MEMORY_NODE=ON` - 증인은 전체 합의 검증이 필요합니다
- **사용 금지** `BUILD_STEEM_TESTNET=ON` (테스트넷 연결이 아닌 경우)
- 최적의 성능을 위해 `Release` 빌드 타입 사용
- `steemd` 바이너리 위치: `build/programs/steemd/steemd`

## 단계 2: 증인 노드 설정

### 데이터 디렉토리 생성

```bash
# 데이터 디렉토리 생성
mkdir -p ~/witness_node_data_dir

# 하위 디렉토리 생성
mkdir -p ~/witness_node_data_dir/blockchain
mkdir -p ~/witness_node_data_dir/logs
```

### 기본 설정 생성

```bash
# 기본 config.ini 생성
./programs/steemd/steemd --data-dir=~/witness_node_data_dir --dump-config > ~/witness_node_data_dir/config.ini
```

### 설정 파일 편집

`~/witness_node_data_dir/config.ini`를 다음 설정으로 편집합니다:

```ini
# ==================== 로깅 설정 ====================

log-console-appender = {"appender":"stderr","stream":"std_error"}
log-logger = {"name":"default","level":"info","appender":"stderr"}
log-logger = {"name":"p2p","level":"warn","appender":"stderr"}

# ==================== 플러그인 설정 ====================

# 증인 노드에 필수적인 플러그인
plugin = chain p2p webserver witness

# 최소 API 플러그인 (보안상 권장)
plugin = database_api witness_api

# 선택사항: 트랜잭션 브로드캐스트가 필요한 경우 추가
# plugin = network_broadcast_api

# ==================== 저장소 설정 ====================

# 공유 메모리 파일 위치
shared-file-dir = blockchain

# 공유 메모리 크기 (증인은 54GB 권장)
shared-file-size = 54G

# 플러시 간격 (0 = 매 블록마다 플러시, 증인에 권장)
flush-state-interval = 0

# ==================== 네트워크 설정 ====================

# P2P 엔드포인트 - 피어 연결 수신
# 형식: IP:PORT (0.0.0.0 = 모든 인터페이스에서 수신)
p2p-endpoint = 0.0.0.0:2001

# 시드 노드 - 네트워크의 시드 노드로 교체하세요
p2p-seed-node = seed1.example.com:2001
p2p-seed-node = seed2.example.com:2001
p2p-seed-node = 192.168.1.100:2001

# 최대 P2P 연결 수 (증인은 10-30 권장)
p2p-max-connections = 20

# ==================== 증인 설정 ====================

# 체인이 정지된 경우에도 블록 생성 활성화 (프로덕션에서는 FALSE)
enable-stale-production = false

# 최소 증인 참여율 (33% 권장)
required-participation = 33

# 증인 계정 이름 - 본인의 증인 계정으로 교체하세요
witness = "your-witness-account"

# 증인 서명 개인키 - 본인의 개인키로 교체하세요
# 중요: 이 키를 안전하게 보관하세요! 활성키가 아닌 별도의 서명키를 사용하세요
private-key = 5K... # 증인 서명키 (WIF 형식)

# ==================== API 설정 (선택사항) ====================

# WebServer HTTP 엔드포인트 (API를 비활성화하려면 주석 처리)
# webserver-http-endpoint = 127.0.0.1:8090

# WebServer WebSocket 엔드포인트 (비활성화하려면 주석 처리)
# webserver-ws-endpoint = 127.0.0.1:8090

# ==================== 성능 튜닝 ====================

# 서명 검증에 사용 가능한 모든 코어 사용
# (지정하지 않으면 자동 감지)
# webserver-thread-pool-size = 4
```

### 설정 참고사항

**중요 설정**:
- `p2p-seed-node`: 네트워크에서 신뢰할 수 있는 시드 노드를 최소 2-3개 추가
- `witness`: 등록된 증인 계정 이름
- `private-key`: 증인 서명키 (활성/소유자 키가 아님)
- `enable-stale-production`: 프로덕션에서는 반드시 `false`
- `flush-state-interval`: 증인은 `0`으로 설정 (매 블록 플러시)

**보안 모범 사례**:
- 프로덕션 증인에서는 API 엔드포인트 비활성화 (`webserver-http-endpoint`, `webserver-ws-endpoint`)
- P2P 포트(2001)를 제외한 모든 포트에 방화벽 설정
- 계정 키와 별도의 전용 서명키 사용
- config.ini를 제한적인 권한으로 저장: `chmod 600 config.ini`

## 단계 3: 시드 노드 정보 획득

### 네트워크 운영자로부터

네트워크 관리자에게 다음 정보를 요청합니다:
- 시드 노드 주소 (IP:port 또는 domain:port)
- 네트워크 ID/체인 ID (검증용)
- 제네시스 블록 해시 (검증용)

### 기존 노드로부터

기존 노드에 접근할 수 있는 경우:

```bash
# 네트워크 정보 확인
curl -X POST http://existing-node:8090 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_config"}' \
  | jq -r '.result.GRAPHENE_CHAIN_ID'

# 활성 피어 확인
curl -X POST http://existing-node:8090 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"network_broadcast_api.get_info"}' \
  | jq
```

### 퍼블릭 메인넷 시드

Steem 메인넷의 경우 퍼블릭 시드 노드를 사용합니다:

```ini
p2p-seed-node = seed.steemnodes.com:2001
p2p-seed-node = seed.steem.com:2001
p2p-seed-node = seed1.blockbrothers.io:2001
```

## 단계 4: 초기 블록체인 동기화

### 처음 노드 시작

```bash
# steemd 시작 (P2P 네트워크로부터 동기화)
./programs/steemd/steemd --data-dir=~/witness_node_data_dir
```

### 동기화 진행상황

초기 동기화는 시드 노드로부터 블록체인을 다운로드합니다. 진행상황 모니터링:

```bash
# 동기화 상태 확인 (다른 터미널에서)
watch -n 5 'curl -s -X POST http://localhost:8090 \
  -H "Content-Type: application/json" \
  -d "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"database_api.get_dynamic_global_properties\"}" \
  | jq -r ".result | \"\(.head_block_number) / \(.time)\""'
```

**동기화 시간 예상**:
- 소규모 네트워크 (<10K 블록): 수분~수시간
- Steem 메인넷 (>50M 블록): 네트워크 속도에 따라 수일~수주

### 로그 모니터링

```bash
# 로그 확인
tail -f ~/witness_node_data_dir/logs/steemd.log

# 확인할 내용:
# - "Syncing blockchain" 메시지
# - P2P 연결 상태
# - 블록 번호 증가
```

### 동기화 완료 확인

다음의 경우 노드가 동기화된 것입니다:
1. 블록 번호가 네트워크 헤드 블록과 일치
2. 블록 타임스탬프가 최신 (3-5초 이내)
3. 로그에서 "Syncing blockchain" 메시지 중단

```bash
# 동기화 확인
curl -s -X POST http://localhost:8090 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_dynamic_global_properties"}' \
  | jq -r '.result | "Block: \(.head_block_number)\nTime: \(.time)"'
```

## 단계 5: 증인으로 등록

### 사전 요구사항

등록 전 확인사항:
- 증인 노드가 완전히 동기화됨
- 블록체인에 증인 계정이 존재함
- 서명 키 쌍이 생성됨

### 서명키 생성

**중요**: 증인 서명키는 **계정 키(owner/active/posting)와 완전히 별개**입니다. 이 키는 블록 서명 전용입니다.

**왜 별도로 관리하나요?**
- 계정 Active 키: 증인 등록 및 설정 변경용 (가능한 오프라인 보관)
- 서명키: 24/7 블록 서명용 (config.ini에 저장, 서버에 보관)
- 보안: 서명키가 유출되어도 계정 자금은 안전함

아직 서명키가 없는 경우:

```bash
# cli_wallet 사용
./programs/cli_wallet/cli_wallet --server-rpc-endpoint=ws://localhost:8090

# cli_wallet 프롬프트에서 - 새 서명키 생성 (계정 키가 아님!)
>>> suggest_brain_key
{
  "brain_priv_key": "긴 브레인 키 구문...",
  "wif_priv_key": "5K...",  # ← config.ini의 private-key에 입력
  "pub_key": "STM..."       # ← update_witness 호출 시 사용
}

# 두 키를 모두 안전하게 저장 (계정 키와 분리하여 보관!)
# cli_wallet 종료
>>> exit
```

### 블록체인에 증인 등록

`cli_wallet`을 사용하여 증인 등록을 브로드캐스트합니다:

```bash
# database_api 및 network_broadcast_api가 있는 풀 노드에 연결된 cli_wallet 시작
./programs/cli_wallet/cli_wallet --server-rpc-endpoint=ws://full-node:8090

# 지갑 잠금 해제
>>> set_password "your-wallet-password"
>>> unlock "your-wallet-password"

# 증인 계정의 ACTIVE 키 가져오기 (서명키가 아님!)
# update_witness 작업을 승인하기 위해 필요
>>> import_key your-witness-account 5K...active-private-key...

# 증인 등록 (또는 기존 증인 업데이트)
# 주의: "STM...public-signing-key..."는 위에서 suggest_brain_key로 생성한 공개키
>>> update_witness "your-witness-account" "https://yourwebsite.com" "STM...public-signing-key..." {"account_creation_fee":"3.000 STEEM","maximum_block_size":65536,"sbd_interest_rate":1000} true

# 등록 확인
>>> get_witness "your-witness-account"
```

### config.ini에 서명키 업데이트

등록 후, 증인 노드의 `config.ini`에 서명 개인키를 추가합니다:

```ini
witness = "your-witness-account"
private-key = 5K...wif-priv-key-from-suggest_brain_key...
```

### 증인 노드 재시작

```bash
# 노드 중지 (Ctrl+C 또는 프로세스 종료)
pkill -INT steemd

# 증인 설정으로 재시작
./programs/steemd/steemd --data-dir=~/witness_node_data_dir
```

## 단계 6: 증인 운영 확인

### 증인 상태 확인

```bash
# 증인 객체 조회
curl -s -X POST http://localhost:8090 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_witness","params":{"account":"your-witness-account"}}' \
  | jq

# 예상 필드:
# - "signing_key": 공개키와 일치해야 함
# - "votes": 증인에 대한 총 투표
# - "total_missed": 놓친 블록 수
# - "last_confirmed_block_num": 마지막으로 생성한 블록
```

### 블록 생성 모니터링

다음의 경우 증인이 블록을 생성합니다:
1. 증인이 활성 증인 세트에 포함됨 (상위 21 + 예정된 백업 증인)
2. 증인 스케줄에서 차례가 도래함
3. 노드가 동기화되고 실행 중임

```bash
# 로그에서 블록 생성 확인
tail -f ~/witness_node_data_dir/logs/steemd.log | grep "Generated block"

# 다음과 같이 표시되어야 함:
# Generated block #12345678 with timestamp 2024-01-15T12:00:00 by your-witness-account
```

### 증인 스케줄 확인

```bash
# 현재 증인 스케줄 조회
curl -s -X POST http://localhost:8090 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_witness_schedule"}' \
  | jq -r '.result.current_shuffled_witnesses'

# 증인 계정이 목록에 나타나면 곧 블록을 생성합니다
```

## 단계 7: 프로덕션 배포

### 시스템 서비스로 실행 (Linux)

systemd 서비스 파일 생성 `/etc/systemd/system/steemd.service`:

```ini
[Unit]
Description=Steem Witness Node
After=network.target

[Service]
Type=simple
User=steem
Group=steem
WorkingDirectory=/home/steem/steem
ExecStart=/home/steem/steem/build/programs/steemd/steemd --data-dir=/home/steem/witness_node_data_dir
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal
SyslogIdentifier=steemd

# 보안 설정
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=full
ProtectHome=true

# 리소스 제한
LimitNOFILE=65536
LimitNPROC=4096

[Install]
WantedBy=multi-user.target
```

서비스 활성화 및 시작:

```bash
# systemd 리로드
sudo systemctl daemon-reload

# 서비스 활성화 (부팅 시 시작)
sudo systemctl enable steemd

# 서비스 시작
sudo systemctl start steemd

# 상태 확인
sudo systemctl status steemd

# 로그 확인
sudo journalctl -u steemd -f
```

### 설정 백업

별도 인프라에 백업 증인을 생성합니다:

```bash
# config.ini를 백업 서버로 복사
scp ~/witness_node_data_dir/config.ini backup-server:~/witness_node_data_dir/

# 블록체인 데이터 동기화 (선택사항 - P2P로 동기화 가능)
rsync -avz --progress ~/witness_node_data_dir/blockchain/ \
  backup-server:~/witness_node_data_dir/blockchain/

# 백업 증인 시작 (동일한 설정, 다른 서버)
# 한 번에 하나의 인스턴스만 증인 플러그인을 활성화해야 합니다
```

### 방화벽 설정

```bash
# P2P 포트 허용 (2001)
sudo ufw allow 2001/tcp

# API 포트 차단 (설정에서 활성화된 경우)
sudo ufw deny 8090/tcp

# 방화벽 활성화
sudo ufw enable
sudo ufw status
```

## 문제 해결

### 노드가 동기화되지 않음

**증상**: 블록 번호가 증가하지 않음, P2P 연결 없음

**해결방법**:
```bash
# P2P 연결 확인
grep "p2p" ~/witness_node_data_dir/logs/steemd.log

# 시드 노드 접근 가능 여부 확인
nc -zv seed1.example.com 2001

# 다른 시드 노드 시도
# config.ini를 편집하고 p2p-seed-node 항목 추가

# 방화벽 확인
sudo ufw status
```

### 증인이 블록을 생성하지 않음

**증상**: 증인은 등록되었지만 블록이 생성되지 않음

**체크리스트**:
1. **증인이 활성 세트에 포함되어 있나요?**
   ```bash
   # 증인 스케줄 확인
   curl -X POST http://localhost:8090 -H "Content-Type: application/json" \
     -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_witness_schedule"}' \
     | jq -r '.result.current_shuffled_witnesses' | grep "your-witness-account"
   ```

2. **서명키가 올바른가요?**
   ```bash
   # 증인 객체의 서명키가 config.ini와 일치하는지 확인
   curl -X POST http://localhost:8090 -H "Content-Type: application/json" \
     -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_witness","params":{"account":"your-witness-account"}}' \
     | jq -r '.result.signing_key'
   ```

3. **노드가 완전히 동기화되었나요?**
   ```bash
   # 블록 타임스탬프가 최신인지 확인
   curl -X POST http://localhost:8090 -H "Content-Type: application/json" \
     -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_dynamic_global_properties"}' \
     | jq -r '.result.time'
   ```

4. **증인 플러그인이 활성화되어 있나요?**
   ```bash
   grep "plugin = witness" ~/witness_node_data_dir/config.ini
   ```

### 높은 메모리 사용량

**증상**: OOM kill, 스왑 사용

**해결방법**:
```bash
# 공유 메모리 크기 확인
ls -lh ~/witness_node_data_dir/blockchain/shared_memory.bin

# 너무 크면 shared-file-size 축소 (증인은 54G 권장)
# config.ini 편집:
# shared-file-size = 54G

# --replay-blockchain로 노드 재시작하여 공유 메모리 재구성
./programs/steemd/steemd --data-dir=~/witness_node_data_dir --replay-blockchain
```

### 블록 누락

**증상**: 증인 객체에서 `total_missed` 증가

**일반적인 원인**:
- 네트워크 지연 (느린 P2P 연결)
- 노드가 동기화되지 않음
- 시계 불일치 (서버 시간 부정확)
- 불충분한 CPU/디스크 I/O

**해결방법**:
```bash
# 시스템 시간 동기화 확인
timedatectl status

# 동기화되지 않은 경우 NTP 활성화
sudo timedatectl set-ntp true

# 디스크 I/O 확인 (SSD 사용해야 함)
iostat -x 5

# P2P 지연 확인
grep "p2p" ~/witness_node_data_dir/logs/steemd.log | grep "latency"

# 동기화 안된 경우 노드 재시작
sudo systemctl restart steemd
```

## 모범 사례

### 보안

1. **서명키 격리**
   - 전용 서명키 생성 (활성/소유자 키 사용 금지)
   - 증인 서버의 config.ini에만 서명키 저장
   - 활성/소유자 키는 하드웨어 지갑/HSM 사용

2. **네트워크 보안**
   - P2P 포트(2001)를 제외한 모든 포트에 방화벽 설정
   - 프로덕션 증인에서는 API 엔드포인트 비활성화
   - 원격 접근에 VPN 사용
   - P2P 엔드포인트에 DDoS 보호

3. **접근 제어**
   - SSH 접근 제한 (키 기반 인증만)
   - steemd 프로세스에 별도 사용자 계정 사용
   - config.ini에 `chmod 600` 적용 (개인키 포함)

### 신뢰성

1. **이중화**
   - 별도 인프라에 주 증인 + 백업 증인 실행
   - 한 번에 하나의 인스턴스에만 증인 플러그인 활성화
   - 모니터링 스크립트를 통한 자동 장애조치

2. **모니터링**
   - 블록 누락 알림
   - 블록 생성 시간 모니터링
   - P2P 연결 수 추적
   - 시스템 리소스 모니터링 (CPU, RAM, 디스크)

3. **유지보수**
   - 트래픽이 적은 시간에 업그레이드 예약
   - 백업 증인에서 먼저 업그레이드 테스트
   - 주요 업그레이드 전 블록 로그 백업 유지

### 성능

1. **하드웨어**
   - SSD 저장소 (NVMe 선호)
   - 64GB+ RAM
   - 4+ CPU 코어
   - 저지연 네트워크 연결

2. **시스템 튜닝**
   - 스왑 비활성화 (RAM만 사용)
   - 파일 디스크립터 제한 증가
   - SSD용 deadline/noop I/O 스케줄러 사용

3. **Linux VM 튜닝** (초기 동기화/재생성용)
   ```bash
   echo    75 | sudo tee /proc/sys/vm/dirty_background_ratio
   echo  1000 | sudo tee /proc/sys/vm/dirty_expire_centisecs
   echo    80 | sudo tee /proc/sys/vm/dirty_ratio
   echo 30000 | sudo tee /proc/sys/vm/dirty_writeback_centisecs
   ```

### 가격 피드 업데이트

상위 20 증인은 정기적으로 SBD 가격 피드를 발행해야 합니다:

```bash
# cli_wallet 사용
>>> publish_feed "your-witness-account" {"base":"1.000 SBD","quote":"0.500 STEEM"} true
```

외부 소스(예: CoinGecko, CMC)에서 가격 피드를 업데이트하는 자동화 스크립트를 설정하세요.

## 모니터링 및 메트릭

### 필수 메트릭

증인 상태를 위한 메트릭 모니터링:

```bash
# 1. 놓친 블록 (일정하게 유지되어야 함)
curl -s -X POST http://localhost:8090 -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_witness","params":{"account":"your-witness-account"}}' \
  | jq -r '.result.total_missed'

# 2. 블록 생성
tail -f ~/witness_node_data_dir/logs/steemd.log | grep "Generated block"

# 3. P2P 연결 (활성 피어 5-20개)
grep "active peers" ~/witness_node_data_dir/logs/steemd.log | tail -1

# 4. 동기화 상태 (head_block_age는 <3초)
curl -s -X POST http://localhost:8090 -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_dynamic_global_properties"}' \
  | jq -r '.result | "Block: \(.head_block_number)\nTime: \(.time)"'
```

### 알림 설정

모니터링 스크립트 생성 `~/witness_monitor.sh`:

```bash
#!/bin/bash

WITNESS_ACCOUNT="your-witness-account"
ALERT_EMAIL="your-email@example.com"

# 놓친 블록 확인
MISSED=$(curl -s -X POST http://localhost:8090 -H "Content-Type: application/json" \
  -d "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"database_api.get_witness\",\"params\":{\"account\":\"$WITNESS_ACCOUNT\"}}" \
  | jq -r '.result.total_missed')

MISSED_LAST=$(cat /tmp/witness_missed_last 2>/dev/null || echo 0)

if [ "$MISSED" -gt "$MISSED_LAST" ]; then
  echo "경고: 증인이 블록을 놓쳤습니다! 총: $MISSED" | mail -s "증인 알림" $ALERT_EMAIL
fi

echo $MISSED > /tmp/witness_missed_last

# 동기화 상태 확인
BLOCK_TIME=$(curl -s -X POST http://localhost:8090 -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_dynamic_global_properties"}' \
  | jq -r '.result.time')

CURRENT_TIME=$(date -u +%s)
BLOCK_TIMESTAMP=$(date -d "$BLOCK_TIME" +%s)
BLOCK_AGE=$((CURRENT_TIME - BLOCK_TIMESTAMP))

if [ "$BLOCK_AGE" -gt 60 ]; then
  echo "경고: 노드 동기화 안됨! 블록 age: ${BLOCK_AGE}초" | mail -s "증인 알림" $ALERT_EMAIL
fi
```

crontab에 추가:

```bash
# 5분마다 모니터링 실행
crontab -e
*/5 * * * * /home/steem/witness_monitor.sh
```

## 추가 자료

- [노드 타입 가이드](node-types-guide.md) - 다양한 노드 설정
- [Steem 빌드](building.md) - 상세 빌드 지침
- [CLI 지갑 가이드](cli-wallet-guide.md) - 지갑 사용법
- [하드포크 절차](../../operations/hardfork-procedure.md) - 업그레이드 절차
- [DDoS 보호](../../operations/ddos-protection.md) - 네트워크 보안

## 도움 받기

- GitHub Issues: [https://github.com/steemit/steem/issues](https://github.com/steemit/steem/issues)
- 개발자 문서: [docs/development/](../../development/)
- 커뮤니티 포럼: Steem 증인 채널

## 라이선스

[LICENSE.md](../../../LICENSE.md) 참조
