# Steem 빌드하기

이 가이드는 다양한 플랫폼에서 Steem을 소스로부터 빌드하는 방법을 설명합니다.

## 사전 요구사항

### 공통 요구사항

모든 플랫폼에서 다음이 필요합니다:
- Git (저장소 클론용)
- CMake 3.x 이상
- C++14 호환 컴파일러 (GCC 4.8+, Clang 3.3+ 또는 최신 버전)
- Python 3 및 Jinja2
- Boost 라이브러리
- OpenSSL
- 추가 압축 라이브러리 (bzip2, snappy, zlib)

## Docker로 빌드하기

일반적인 노드 타입 바이너리를 모두 빌드하는 Dockerfile을 제공합니다.

```bash
git clone https://github.com/hanyeol/steem
cd steem
docker build -t hanyeol/steem .
```

## Ubuntu 22.04에서 빌드하기

### 의존성 설치

필수 패키지 설치:

```bash
# 핵심 빌드 도구 및 라이브러리
sudo apt-get install -y \
    autoconf \
    automake \
    cmake \
    g++ \
    git \
    libbz2-dev \
    libsnappy-dev \
    libssl-dev \
    libtool \
    make \
    pkg-config \
    python3 \
    python3-jinja2
```

Boost 라이브러리 설치:

```bash
sudo apt-get install -y \
    libboost-chrono-dev \
    libboost-context-dev \
    libboost-coroutine-dev \
    libboost-date-time-dev \
    libboost-filesystem-dev \
    libboost-iostreams-dev \
    libboost-locale-dev \
    libboost-program-options-dev \
    libboost-serialization-dev \
    libboost-signals-dev \
    libboost-system-dev \
    libboost-test-dev \
    libboost-thread-dev
```

더 나은 개발 경험을 위한 선택적 패키지:

```bash
sudo apt-get install -y \
    doxygen \
    libncurses5-dev \
    libreadline-dev \
    perl
```

### 클론 및 빌드

```bash
git clone https://github.com/hanyeol/steem
cd steem
git checkout stable
git submodule update --init --recursive
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) steemd
make -j$(nproc) cli_wallet
```

선택 사항: 바이너리를 시스템 전역에 설치:

```bash
sudo make install  # 기본적으로 /usr/local에 설치됩니다
```

## macOS에서 빌드하기

### Xcode 설치

https://guide.macports.org/#installing.xcode 에서 Xcode와 명령줄 도구를 설치하세요.

macOS 10.11 (El Capitan) 이상에서는 터미널에서 개발자 명령을 실행하면 개발자 도구 설치 안내가 표시됩니다.

Xcode 라이선스 동의:

```bash
sudo xcodebuild -license accept
```

### Homebrew 설치

http://brew.sh/ 에서 Homebrew를 설치하세요.

Homebrew 초기화:

```bash
brew doctor
brew update
```

### 의존성 설치

필수 패키지 설치:

```bash
brew install \
    autoconf \
    automake \
    cmake \
    git \
    boost \
    libtool \
    openssl \
    snappy \
    zlib \
    python3

pip3 install --user jinja2
```

참고: 코드베이스가 최신 Boost 버전과 호환되도록 업데이트되었습니다 (Boost 1.89.0에서 테스트됨).

선택적 패키지:

```bash
# LevelDB에서 TCMalloc으로 더 나은 성능 제공
brew install google-perftools

# 더 나은 readline 지원을 위한 cli_wallet
brew install --force readline
brew link --force readline
```

### 클론 및 빌드

```bash
git clone https://github.com/hanyeol/steem.git
cd steem
git checkout stable
git submodule update --init --recursive

export BOOST_ROOT=$(brew --prefix boost)
export OPENSSL_ROOT_DIR=$(brew --prefix openssl)

mkdir build && cd build
cmake -DBOOST_ROOT="$BOOST_ROOT" -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.logicalcpu)
```

### 특정 타겟 빌드

전체가 아닌 특정 타겟만 빌드할 수 있습니다:

```bash
# steemd만 빌드
make -j$(sysctl -n hw.logicalcpu) steemd

# cli_wallet만 빌드
make -j$(sysctl -n hw.logicalcpu) cli_wallet

# 테스트 스위트 빌드
make -j$(sysctl -n hw.logicalcpu) chain_test
```

## 다른 플랫폼에서 빌드하기

### Windows

Windows 빌드 지침은 아직 제공되지 않습니다.

### 컴파일러 지원

- **공식 지원**: GCC 및 Clang (핵심 개발자가 사용)
- **커뮤니티 지원**: MinGW, Intel C++, Microsoft Visual C++
  - 작동할 수 있지만 개발자가 정기적으로 테스트하지 않습니다
  - 이러한 컴파일러의 경고/오류를 수정하는 풀 리퀘스트는 환영합니다

## CMake 빌드 옵션

CMake에 옵션을 전달하여 빌드를 구성할 수 있습니다:

### CMAKE_BUILD_TYPE=[Release/Debug]

빌드 타입을 지정합니다:
- `Release`: 디버그 심볼 없이 최적화된 빌드 (프로덕션 권장)
- `Debug`: 디버그 심볼을 포함한 최적화되지 않은 빌드 (개발 및 디버깅용)

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

### LOW_MEMORY_NODE=[OFF/ON]

합의 전용 저메모리 노드 빌드:
- 합의에 필요하지 않은 데이터와 필드를 객체 데이터베이스에서 제외
- 증인(witness)과 시드 노드에 권장

```bash
cmake -DLOW_MEMORY_NODE=ON ..
```

### CLEAR_VOTES=[ON/OFF]

더 이상 합의에 필요하지 않은 오래된 투표를 메모리에서 제거합니다.

```bash
cmake -DCLEAR_VOTES=ON ..
```

### BUILD_STEEM_TESTNET=[OFF/ON]

프라이빗 테스트넷에서 사용하기 위해 빌드합니다. 유닛 테스트 빌드에도 필요합니다.

```bash
cmake -DBUILD_STEEM_TESTNET=ON ..
```

### SKIP_BY_TX_ID=[OFF/ON]

트랜잭션 ID 기반 인덱싱 건너뛰기:
- 재인덱싱 시 약 65%의 CPU 시간 절약
- account history 플러그인의 트랜잭션 ID 조회 기능 비활성화
- 트랜잭션 ID 조회가 필요하지 않다면 적극 권장

```bash
cmake -DSKIP_BY_TX_ID=ON ..
```

## 다음 단계

빌드 완료 후:
- 첫 노드 실행은 [quick-start.md](quick-start.md)를 참조하세요
- 지갑 사용법은 [cli-wallet-guide.md](cli-wallet-guide.md)를 참조하세요
- 다양한 노드 구성은 [node-types-guide.md](node-types-guide.md)를 참조하세요
