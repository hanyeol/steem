# 빠른 시작

이 가이드는 Docker를 사용하여 Steem 노드를 빠르게 시작하는 방법을 안내합니다.

## 사전 요구사항

- Docker 설치 (v20.10 이상 권장)
- 충분한 디스크 공간 (최소 100GB, 노드 타입에 따라 다름)
- 안정적인 인터넷 연결

## Docker 이미지 빌드

소스에서 Docker 이미지를 빌드합니다:

```bash
git clone https://github.com/hanyeol/steem.git
cd steem
git submodule update --init --recursive
docker build -t hanyeol/steem .
```

**참고:** 빌드 시 3가지 바이너리가 생성됩니다:
- `/usr/local/steemd-low/bin/steemd` - 저메모리 노드
- `/usr/local/steemd-high/bin/steemd` - 전체 API 노드
- `/usr/local/steemd-testnet/bin/steemd` - 테스트넷 노드

## Docker 볼륨 생성

블록체인 데이터를 저장할 Docker 볼륨을 생성합니다:

```bash
docker volume create steem-data
```

**Docker 볼륨 사용의 장점:**
- Docker가 자동으로 관리
- 백업 및 마이그레이션 용이
- 권한 문제 없음
- 성능 최적화

## 노드 실행

### 저메모리 노드 (기본)

시드 노드, 증인 노드, 거래소 노드에 적합합니다.

```bash
docker run \
    -d \
    --name steemd-low \
    -p 2001:2001 -p 8090:8090 \
    --restart unless-stopped \
    -v steem-data:/var/lib/steemd \
    hanyeol/steem
```

**리소스 요구사항:**
- RAM: 4-8GB
- 디스크: 50GB+
- 공유 메모리: 5.5GB

### 전체 API 노드

모든 API 기능이 필요한 경우 사용합니다.

```bash
docker run \
    -d \
    --name steemd-high \
    -p 2001:2001 -p 8090:8090 \
    --env USE_HIGH_MEMORY=1 \
    --env USE_FULL_WEB_NODE=1 \
    --restart unless-stopped \
    -v steem-data:/var/lib/steemd \
    hanyeol/steem
```

**리소스 요구사항:**
- RAM: 16GB+
- 디스크: 110GB+
- 공유 메모리: 65GB+

### 거래소 노드

특정 계정의 거래 기록을 추적합니다.

```bash
docker run \
    -d \
    --name steemd-exchange \
    -p 2001:2001 -p 8090:8090 \
    --env TRACK_ACCOUNT="yourexchangeid" \
    --restart unless-stopped \
    -v steem-data:/var/lib/steemd \
    hanyeol/steem
```

**리소스 요구사항:**
- RAM: 8GB+
- 디스크: 50GB+
- 공유 메모리: 16GB+

### 테스트넷 노드

개발 및 테스트 목적으로 사용합니다.

```bash
docker run \
    -d \
    --name steemd-testnet \
    -p 2001:2001 -p 8090:8090 \
    --restart unless-stopped \
    -v steem-testnet-data:/var/lib/steemd \
    hanyeol/steem \
    /usr/local/steemd-testnet/bin/steemd
```

**참고:** 테스트넷은 별도의 볼륨(`steem-testnet-data`)을 사용하는 것이 좋습니다.

## 노드 상태 확인

### 로그 확인

```bash
# 실시간 로그 보기
docker logs -f steemd-low

# 마지막 100줄 보기
docker logs --tail 100 steemd-low
```

### API 동작 확인

```bash
# API 응답 확인
curl -s http://localhost:8090 | jq

# 동적 글로벌 속성 조회
curl -s -X POST http://localhost:8090 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_dynamic_global_properties"}' \
  | jq
```

## 설정 파일 커스터마이징

### 기본 설정 파일 추출

Docker 이미지에 포함된 설정 파일을 추출하여 수정할 수 있습니다:

```bash
# 저메모리 노드 설정
docker run --rm hanyeol/steem cat /etc/steemdig.ini > my-config.ini

# 전체 노드 설정
docker run --rm hanyeol/steem cat /etc/steemd/fullnode.config.ini > my-config.ini

# 테스트넷 설정
docker run --rm hanyeol/steem cat /etc/steemd/testnet.config.ini > my-config.ini

# 거래소 설정 (계정 기록)
docker run --rm hanyeol/steem cat /etc/steemd/ahnode.config.ini > my-config.ini
```

### 방법 1: 볼륨에 설정 파일 복사

```bash
# 1. 설정 파일 추출 및 수정
docker run --rm hanyeol/steem cat /etc/steemd/fullnode.config.ini > my-config.ini
vi my-config.ini

# 2. 볼륨에 설정 파일 복사
docker run --rm \
    -v steem-data:/data \
    -v $(pwd)/my-config.ini:/config.ini:ro \
    alpine cp /config.ini /data/config.ini

# 3. 노드 실행
docker run -d \
    --name steemd-custom \
    -v steem-data:/var/lib/steemd \
    -p 2001:2001 -p 8090:8090 \
    hanyeol/steem
```

### 방법 2: 설정 파일 직접 마운트

```bash
# 1. 설정 파일 준비
docker run --rm hanyeol/steem cat /etc/steemdig.ini > my-config.ini
vi my-config.ini

# 2. 설정 파일을 직접 마운트하여 실행
docker run -d \
    --name steemd-custom \
    -v $(pwd)/my-config.ini:/var/lib/steemd/config.ini:ro \
    -v steem-data:/var/lib/steemd \
    -p 2001:2001 -p 8090:8090 \
    hanyeol/steem
```

**참고:**
- 설정 파일 마운트(`-v config.ini:...`)를 데이터 볼륨(`-v steem-data:...`)보다 **뒤에** 배치해야 올바르게 적용됩니다
- `:ro` 플래그는 읽기 전용 마운트를 의미합니다

## 환경 변수

Docker 컨테이너 실행 시 다음 환경 변수를 사용할 수 있습니다:

| 환경 변수 | 설명 | 사용 예시 |
|----------|------|----------|
| `USE_HIGH_MEMORY=1` | 고메모리 모드 (인메모리 계정 히스토리) | `-e USE_HIGH_MEMORY=1` |
| `USE_FULL_WEB_NODE=1` | 전체 API 설정 사용 | `-e USE_FULL_WEB_NODE=1` |
| `TRACK_ACCOUNT="account"` | 특정 계정 추적 | `-e TRACK_ACCOUNT="exchange"` |
| `USE_NGINX_PROXY=1` | NGINX 프록시 활성화 | `-e USE_NGINX_PROXY=1` |
| `USE_MULTICORE_READONLY=1` | 멀티리더 모드 (실험적) | `-e USE_MULTICORE_READONLY=1` |

## Docker 볼륨 관리

### 볼륨 정보 확인

```bash
# 볼륨 목록 조회
docker volume ls

# 볼륨 상세 정보 확인
docker volume inspect steem-data

# 볼륨 실제 경로 확인
docker volume inspect steem-data | jq -r '.[0].Mountpoint'
```

### 볼륨 크기 확인

```bash
# Docker 볼륨 크기 확인
docker run --rm -v steem-data:/data alpine du -sh /data

# 상세 내용 확인
docker run --rm -v steem-data:/data alpine ls -lh /data
```

### 볼륨 백업

```bash
# 볼륨을 tar.gz로 백업
docker run --rm \
    -v steem-data:/data \
    -v $(pwd):/backup \
    alpine tar czf /backup/steem-backup-$(date +%Y%m%d).tar.gz -C /data .

# 백업 크기 확인
ls -lh steem-backup-*.tar.gz
```

### 볼륨 복원

```bash
# 새 볼륨 생성
docker volume create steem-data-restored

# 백업에서 복원
docker run --rm \
    -v steem-data-restored:/data \
    -v $(pwd):/backup \
    alpine tar xzf /backup/steem-backup-20250103.tar.gz -C /data

# 복원된 볼륨으로 노드 실행
docker run -d \
    --name steemd-restored \
    -v steem-data-restored:/var/lib/steemd \
    -p 2001:2001 -p 8090:8090 \
    hanyeol/steem
```

### 볼륨 삭제

```bash
# 주의: 볼륨을 삭제하면 모든 블록체인 데이터가 삭제됩니다!

# 먼저 컨테이너 중지 및 삭제
docker stop steemd-lite
docker rm steemd-lite

# 볼륨 삭제
docker volume rm steem-data
```

## 컨테이너 관리

### 기본 명령어

```bash
# 컨테이너 목록 확인
docker ps
docker ps -a  # 중지된 컨테이너 포함

# 로그 확인
docker logs -f steemd-low

# 컨테이너 중지
docker stop steemd-low

# 컨테이너 시작
docker start steemd-low

# 컨테이너 재시작
docker restart steemd-low

# 컨테이너 삭제 (볼륨은 유지됨)
docker stop steemd-low
docker rm steemd-low

# 컨테이너 상태 확인
docker ps -a | grep steemd
```

### 리소스 사용량 모니터링

```bash
# 실시간 리소스 사용량
docker stats steemd-low

# 한 번만 확인
docker stats --no-stream steemd-low
```

## 호스트 디렉토리 사용 (대안)

Docker 볼륨 대신 호스트 디렉토리를 사용하려는 경우:

```bash
# 1. 디렉토리 생성
mkdir -p ~/steem-data

# 2. 노드 실행
docker run \
    -d \
    --name steemd-low \
    -p 2001:2001 -p 8090:8090 \
    --restart unless-stopped \
    -v ~/steem-data:/var/lib/steemd \
    hanyeol/steem

# 3. 설정 파일 직접 수정
vi ~/steem-data/config.ini
docker restart steemd-low
```

**호스트 디렉토리 사용 시 장점:**
- 직접 파일에 접근 가능
- 익숙한 경로 사용
- 쉬운 백업 (일반 디렉토리 복사)

**단점:**
- 권한 문제 발생 가능
- Docker가 관리하지 않음

## 성능 최적화

### Linux 가상 메모리 설정

초기 동기화 및 체인 리플레이 시 성능 향상:

```bash
echo    75 | sudo tee /proc/sys/kernel/vm/dirty_background_ratio
echo  1000 | sudo tee /proc/sys/kernel/vm/dirty_expire_centisecs
echo    80 | sudo tee /proc/sys/kernel/vm/dirty_ratio
echo 30000 | sudo tee /proc/sys/kernel/vm/dirty_writeback_centisecs
```

### SSD 사용 권장

SSD에 Docker 볼륨을 저장하려면 Docker의 데이터 루트를 변경하세요:

```bash
# /etc/docker/daemon.json 수정
sudo vi /etc/docker/daemon.json
```

```json
{
  "data-root": "/mnt/ssd/docker"
}
```

```bash
# Docker 재시작
sudo systemctl restart docker
```

## 노드 타입별 요구사항

| 노드 타입 | RAM | 공유 메모리 | 디스크 공간 | 용도 |
|----------|-----|-----------|----------|------|
| 시드 노드 | 4GB+ | 5.5GB+ | 50GB+ | P2P 네트워크 중계 |
| 증인 노드 | 4GB+ | 5.5GB+ | 50GB+ | 블록 생성 |
| 거래소 노드 | 8GB+ | 16GB+ | 50GB+ | 특정 계정 추적 |
| 전체 API 노드 | 16GB+ | 65GB+ | 110GB+ | 모든 API 제공 |

**디스크 공간 구성:**
- 블록 로그 (block_log): 27GB+ (지속적으로 증가)
- 공유 메모리 파일 (blockchain/): 노드 타입에 따라 다름
- 기타 (로그, 설정 등): 1-5GB

## 문제 해결

### 컨테이너가 시작되지 않는 경우

```bash
# 로그 확인
docker logs steemd-low

# 일반적인 원인:
# 1. 포트 충돌 (2001, 8090 포트가 이미 사용 중)
lsof -i :2001
lsof -i :8090

# 2. 디스크 공간 부족
df -h
docker system df

# 3. 메모리 부족
free -h
```

### 동기화가 느린 경우

```bash
# P2P 연결 확인
docker logs steemd-low | grep "p2p"

# 시드 노드 추가
docker run \
    -e STEEMD_SEED_NODES="seed1.example.com:2001 seed2.example.com:2001" \
    -v steem-data:/var/lib/steemd \
    ...
```

### 볼륨이 가득 찬 경우

```bash
# Docker 시스템 정리
docker system prune -a

# 사용하지 않는 볼륨 삭제
docker volume prune

# 특정 볼륨 크기 확인
docker run --rm -v steem-data:/data alpine du -sh /data/*
```

## 추가 리소스

- [노드 타입 가이드](node-types-guide.md) - 다양한 노드 타입에 대한 상세 정보
- [빌드 가이드](building.md) - 소스에서 직접 빌드하기
- [거래소 빠른 시작](exchange-quick-start.md) - 거래소 통합 가이드
- [테스팅 가이드](../development/testing.md) - 테스트 실행 방법

## 다음 단계

1. **증인 운영**: 증인으로 블록을 생성하려면 `witness` 설정을 추가하세요
2. **API 제공**: 애플리케이션을 위한 API를 제공하려면 전체 노드를 운영하세요
3. **모니터링 설정**: Prometheus, Grafana 등을 사용한 모니터링 구축
4. **백업 전략**: 정기적인 데이터 백업 계획 수립
