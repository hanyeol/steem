# Ubuntu 22.04 Docker에서 빌드 및 테스트

이 가이드는 Ubuntu 22.04를 사용하여 Docker에서 Steem을 빌드하고 테스트하는 방법을 설명하며, Boost 버전 호환성 문제로 인해 네이티브로 빌드할 수 없는 macOS ARM64 (Apple Silicon M1/M2/M3) 사용자에게 유용합니다.

**왜 Docker인가?** macOS Homebrew는 breaking change가 있는 Boost 1.89를 제공합니다 (예: `io_service` 제거). Ubuntu 22.04는 주요 업그레이드 대상인 Boost 1.74를 제공합니다.

## 사전 요구사항

- Docker Desktop for Mac (Apple Silicon) 설치됨
- 최소 4GB의 여유 디스크 공간
- Docker에 할당된 4GB+ RAM
- Rosetta 2는 필요하지 않음 (네이티브 ARM64 Ubuntu 이미지 사용)

## 빠른 시작

### 1. Ubuntu 22.04 컨테이너 시작 (ARM64)

Steem 프로젝트 디렉토리에서:

```bash
docker run -it --rm \
  --platform linux/arm64 \
  -v $(pwd):/steem \
  -w /steem \
  ubuntu:22.04 \
  bash
```

**옵션:**
- `--platform linux/arm64`: 네이티브 ARM64 아키텍처 사용 (Apple Silicon용)
- `-it`: 인터랙티브 터미널
- `--rm`: 종료 후 컨테이너 제거
- `-v $(pwd):/steem`: 현재 디렉토리를 컨테이너의 `/steem`에 마운트
- `-w /steem`: 작업 디렉토리를 `/steem`으로 설정

### 2. 의존성 설치

컨테이너 내부에서:

```bash
# 패키지 목록 업데이트
apt update

# 빌드 필수 요소 설치
apt install -y \
  build-essential \
  cmake \
  git \
  libboost-all-dev \
  libssl-dev \
  libsnappy-dev \
  libbz2-dev \
  python3-jinja2 \
  autoconf \
  automake \
  libtool \
  pkg-config

# Boost 버전 확인 (1.74여야 함)
dpkg -s libboost-dev | grep Version
```

### 3. Steem 빌드

```bash
# git 서브모듈 초기화 (아직 수행하지 않은 경우)
git submodule update --init --recursive

# 빌드 디렉토리 생성
rm -rf build && mkdir build && cd build

# CMake로 구성
cmake -DCMAKE_BUILD_TYPE=Release ..

# steemd 빌드 (코어 2개의 경우 -j2 사용, 사용 가능한 CPU에 따라 조정)
make -j2 steemd

# cli_wallet 빌드
make -j2 cli_wallet
```

**빌드 시간:** CPU 코어에 따라 약 30-60분.

### 4. 단위 테스트 실행

```bash
# 테스트넷 빌드 구성
cd /steem
rm -rf build && mkdir build && cd build
cmake -DBUILD_STEEM_TESTNET=ON -DCMAKE_BUILD_TYPE=Debug ..

# 테스트 실행 파일 빌드
make -j2 chain_test
make -j2 plugin_test

# 모든 테스트 실행
./tests/chain_test
./tests/plugin_test

# 특정 테스트 스위트 실행
./tests/chain_test --run_test=basic_tests
./tests/chain_test --run_test=block_tests
./tests/chain_test --run_test=operation_tests

# 단일 테스트 실행
./tests/chain_test -t basic_tests/parse_size_test

# 상세 출력
./tests/chain_test --log_level=all

# 사용 가능한 모든 테스트 나열
./tests/chain_test --list_content
```

## 고급 사용법

### 영구 컨테이너

컨테이너를 유지하고 재사용하려면:

```bash
# 이름이 지정된 컨테이너 생성
docker run -it \
  --platform linux/arm64 \
  --name steem-build \
  -v $(pwd):/steem \
  -w /steem \
  ubuntu:22.04 \
  bash

# 종료 후 컨테이너 재시작
docker start -i steem-build

# 완료되면 컨테이너 제거
docker rm steem-build
```

### 다양한 옵션으로 빌드

#### Low Memory Node (합의 전용)

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DLOW_MEMORY_NODE=ON ..
make -j2 steemd
```

#### Skip Transaction ID Index (재인덱스 중 65% CPU 절약)

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DSKIP_BY_TX_ID=ON ..
make -j2 steemd
```

#### 코드 커버리지 활성화

```bash
cmake -DBUILD_STEEM_TESTNET=ON \
      -DENABLE_COVERAGE_TESTING=ON \
      -DCMAKE_BUILD_TYPE=Debug ..
make -j2 chain_test

# 테스트 실행
./tests/chain_test

# 커버리지 리포트 생성
apt install -y lcov
lcov --capture --initial --directory . --output-file base.info --no-external
./tests/chain_test
lcov --capture --directory . --output-file test.info --no-external
lcov --add-tracefile base.info --add-tracefile test.info --output-file total.info
lcov -o interesting.info -r total.info tests/\*
mkdir -p lcov
genhtml interesting.info --output-directory lcov --prefix $(pwd)

# 커버리지 리포트는 lcov/index.html에 있음
# 브라우저에서 보려면 호스트로 복사:
# 컨테이너 종료 후: docker cp steem-build:/steem/build/lcov ./lcov
```

## Docker Compose (선택 사항)

프로젝트 루트에 `docker-compose.yml` 생성:

```yaml
version: '3.8'
services:
  steem-build:
    image: ubuntu:22.04
    platform: linux/arm64
    volumes:
      - .:/steem
    working_dir: /steem
    stdin_open: true
    tty: true
    command: bash
```

그런 다음 사용:

```bash
docker-compose run --rm steem-build
```

## 문제 해결

### 메모리 부족

"killed" 또는 메모리 부족 오류로 빌드가 실패하는 경우:

```bash
# 더 적은 병렬 작업 사용
make -j1 steemd  # 단일 스레드 빌드
```

또는 Docker Desktop 설정에서 Docker의 메모리 제한을 늘리십시오.

### 서브모듈 문제

```bash
# 서브모듈 정리 및 재초기화
git submodule deinit -f .
git submodule update --init --recursive
```

### 클린 빌드

```bash
# 빌드 디렉토리 제거 및 새로 시작
cd /steem
rm -rf build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j2 steemd
```

## Boost 1.74 호환성 테스트

Boost 1.74 호환성을 위한 변경 후:

```bash
# 1. 컨테이너 시작 (Apple Silicon용 ARM64)
docker run -it --rm --platform linux/arm64 -v $(pwd):/steem -w /steem ubuntu:22.04 bash

# 2. 의존성 설치
apt update && apt install -y build-essential cmake git libboost-all-dev \
  libssl-dev libsnappy-dev libbz2-dev python3-jinja2 autoconf automake libtool pkg-config

# 3. Boost 버전 확인
dpkg -s libboost-dev | grep Version
# 표시되어야 함: Version: 1.74.0.3ubuntu7

# 4. 클린 빌드
git submodule update --init --recursive
rm -rf build && mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j2 steemd

# 5. 테스트 실행
cd /steem
rm -rf build && mkdir build && cd build
cmake -DBUILD_STEEM_TESTNET=ON -DCMAKE_BUILD_TYPE=Debug ..
make -j2 chain_test plugin_test
./tests/chain_test
./tests/plugin_test
```

## 예상 결과

### 성공적인 빌드 출력

```
[100%] Built target steemd
```

### 성공적인 테스트 출력

```
*** No errors detected
```

## 참고사항

- **아키텍처**: Apple Silicon Mac용 네이티브 ARM64 Ubuntu 이미지 사용 (에뮬레이션 오버헤드 없음)
- **Boost 버전**: Ubuntu 22.04는 이번 업그레이드의 주요 대상인 Boost 1.74.0을 사용
- **macOS Boost 문제**: macOS Homebrew는 breaking change가 있는 Boost 1.89를 제공 (`io_service` → `io_context`)
- **빌드 아티팩트**: 빌드된 바이너리는 `build/programs/steemd/steemd` 및 `build/programs/cli_wallet/cli_wallet`에 있음
- **테스트 바이너리**: `build/tests/chain_test` 및 `build/tests/plugin_test`에 있음
- **성능**: ARM64 네이티브 실행은 Apple Silicon에서 최적의 빌드 성능 제공

## 참고

- [Building Steem](../getting-started/building.md) - 일반 빌드 지침
- [Testing Guide](testing.md) - 포괄적인 테스트 문서
