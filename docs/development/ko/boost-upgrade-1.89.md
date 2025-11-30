# Boost 1.58-1.89 호환성 계획

이 문서는 Steem에서 Boost 버전 1.58부터 1.89까지 지원하기 위한 호환성 고려사항을 설명합니다.

## 개요

**지원 범위**: Boost 1.58.0 - 1.89.0
**현재 테스트**: Boost 1.74.0 (최근 1.58-1.60에서 업그레이드)
**예상 작업량**: 중간 복잡도
**주요 변경사항**: 6개 주요 영역 확인됨 (1.74 → 1.89)

## 동기

- **광범위한 플랫폼 지원**: 레거시 시스템(Ubuntu 16.04 with Boost 1.58)과 최신 플랫폼(Ubuntu 24.04, macOS Homebrew with Boost 1.83-1.89) 모두 지원
- **유연성**: 특정 버전을 강제하지 않고 시스템에서 제공하는 Boost 버전으로 빌드 가능
- **버그 수정 및 성능**: 31개의 Boost 릴리스(1.58-1.89)에 걸친 개선사항 활용
- **보안 업데이트**: 최신 시스템의 사용자가 중요한 보안 수정이 포함된 버전 사용 가능
- **C++17 호환성**: 새로운 Boost 버전에서 C++17 기능에 대한 더 나은 지원 (Steem은 최근 C++17로 업그레이드)

## 주요 변경사항 (1.74 → 1.89)

### 1. C++ 표준 요구사항

**변경사항**: 많은 라이브러리에서 최소 C++ 표준 요구사항 증가

- **Boost.System** (1.85): 이제 최소 C++11 필요 (GCC 4.8+, MSVC 14.0/VS2015+)
- **Boost.Math** (1.76): C++03 지원 중단, C++11 필요
- **Boost.Math** (1.80): C++11 deprecated, 최소 C++14 권장
- **Boost.PFR** (1.85): 이제 명시적으로 C++14 필요

**영향**: ✅ **호환 가능** - Steem은 C++17 사용 (최근 C++14에서 업그레이드)

**필요 조치**: 없음 - Steem은 이미 이러한 요구사항을 충족

---

### 2. Boost.Asio 변경사항 (io_service → io_context)

**변경사항**: `boost::asio::io_service`가 `boost::asio::io_context`로 이름 변경 (Boost 1.66+)

**타임라인**:
- Boost 1.58-1.65: `io_service`만 존재
- Boost 1.66-1.86: `io_service` (typedef로) 와 `io_context` 모두 존재
- Boost 1.87+: `io_service` 제거, `io_context`만 존재

**영향받는 파일**:
- `src/base/fc/include/fc/asio.hpp` - Line 75: `boost::asio::io_service& default_io_service()`
- `src/base/fc/src/asio.cpp` - io_service 구현
- `src/plugins/webserver/webserver_plugin.cpp` - io_service 사용
- `src/plugins/witness/witness_plugin.cpp` - io_service 사용
- `src/base/appbase/include/appbase/application.hpp` - io_service 사용 가능

**필요 조치**:
모든 Boost 버전(1.58-1.89)을 지원하기 위한 버전 기반 조건부 컴파일 구현:

1. **`fc/asio.hpp`에 호환성 typedef 생성**:
   ```cpp
   #include <boost/version.hpp>

   #if BOOST_VERSION >= 106600  // Boost 1.66.0+
   namespace fc {
       using io_service_t = boost::asio::io_context;
   }
   #else
   namespace fc {
       using io_service_t = boost::asio::io_service;
   }
   #endif
   ```

2. **모든 사용처 업데이트**:
   - `boost::asio::io_service`를 `fc::io_service_t`로 교체
   - `boost::asio::io_context`를 `fc::io_service_t`로 교체
   - 이를 통해 Boost 1.58-1.89와 컴파일 가능

3. **대체 접근법** (최소 변경을 선호하는 경우):
   - Boost 1.87+의 경우, 호환성 typedef 추가:
   ```cpp
   #if BOOST_VERSION >= 108700  // Boost 1.87.0+
   namespace boost { namespace asio {
       using io_service = io_context;
   }}
   #endif
   ```

**우선순위**: 높음 - 핵심 네트워킹 기능, Boost 1.87+ 지원 차단

---

### 3. Boost.Filesystem v4

**변경사항**: Boost 1.77에서 API 변경사항과 함께 Filesystem v4 도입

**주요 변경사항**:
- `copy_directory()` 1.74에서 deprecated, `create_directory()` 오버로드로 교체
- 경로 비교 동작 변경
- std::filesystem과의 더 나은 호환성 (C++17)

**기본 동작**:
- Boost 1.74-1.88: 기본적으로 Filesystem v3 사용
- 향후 버전: v4가 기본이 될 예정

**제어 매크로**: `BOOST_FILESYSTEM_VERSION` (3 또는 4로 설정)

**필요 조치**:
1. `boost::filesystem::copy_directory` 사용 검색 (있는 경우)
2. 모든 Boost 버전(1.58-1.89)과의 호환성을 위해 Filesystem v3 계속 사용
   - v3는 Boost 1.58-1.88에서 기본값
   - 필요시 CMake에서 명시적으로 `BOOST_FILESYSTEM_VERSION=3` 정의 가능
3. 다양한 Boost 버전에서 파일시스템 작업을 철저히 테스트

**우선순위**: 중간 - 파일시스템 사용에 따라 영향 달라짐

---

### 4. Boost.Hash 변경사항

**변경사항**: 해시 관련 헤더의 deprecation 및 재구성

**영향받는 항목**:
- `boost/container_hash/detail/container_fwd.hpp` - Deprecation 경고 추가 (1.74)
- `boost::unordered::hash_is_avalanching` - 이제 using-declaration (1.89)
- `<boost/unordered/hash_traits.hpp>` - 향후 제거 예정

**필요 조치**:
1. deprecated 해시 헤더 사용 검색
2. 권장 대안으로 교체
3. 빌드 중 컴파일러 경고 모니터링

**우선순위**: 낮음 - 사용이 최소일 가능성

---

### 5. Boost.Iterator

**변경사항**: `boost/function_output_iterator.hpp` deprecated (1.74)

**교체**: `boost/iterator/function_output_iterator.hpp`

**필요 조치**:
```bash
grep -r "boost/function_output_iterator.hpp" src/
```
deprecated include를 새 위치로 교체.

**우선순위**: 낮음

---

### 6. Boost.System

**변경사항**:
- `boost/system/cygwin_error.hpp` 제거 (Boost 1.85)
- 원본 MinGW (mingw.org, 32비트 전용) 더 이상 지원 안함 (Boost 1.85)
- MinGW-w64 (32비트/64비트)는 계속 지원

**영향**:
- ✅ **대부분의 사용자에게 조치 불필요**
- Boost.System은 코드베이스에서 널리 사용됨:
  - `boost::system::error_code` - ASIO 오류 처리
  - `boost::system::system_error` - 네트워크/파일시스템 예외
  - 사용처: `fc/asio.cpp`, `fc/network/*.cpp`, `fc/filesystem.cpp`

**검증**:
1. ✅ **Cygwin 헤더**: 사용 안함 (grep으로 확인)
2. ⚠️ **MinGW 지원**: CMakeLists.txt에 MinGW 빌드 구성 있음 (line 196)
   - Windows에서 MinGW로 빌드하는 경우, **MinGW-w64** 사용 확인 (원본 MinGW 아님)
   - MinGW-w64는 32비트(`i686-w64-mingw32`)와 64비트(`x86_64-w64-mingw32`) 모두 지원

**필요 조치**:
- **Linux/macOS**: 조치 불필요
- **Windows MinGW 사용자**:
  1. MinGW-w64 사용 확인 (mingw.org 원본 아님)
  2. MinGW-w64 요구사항을 명시하도록 문서 업데이트

**우선순위**: 낮음 - 플랫폼별, 코드 변경 불필요

---

## 의존성 분석

### 현재 사용 중인 Boost 컴포넌트

`CMakeLists.txt` lines 43-53에서:
```cmake
BOOST_COMPONENTS:
- thread
- date_time
- filesystem
- program_options
- serialization
- chrono
- unit_test_framework
- context
- locale
- coroutine
```

### 컴포넌트별 고려사항

| 컴포넌트 | 상태 | 비고 |
|-----------|--------|-------|
| `thread` | ⚠️ 검토 | promise/future API 변경 모니터링 |
| `date_time` | ✅ 안정 | 주요 변경사항 없음 |
| `filesystem` | ⚠️ 검토 | v3→v4 전환 |
| `program_options` | ✅ 안정 | 주요 변경사항 없음 |
| `serialization` | ✅ 안정 | 주요 변경사항 없음 |
| `chrono` | ✅ 안정 | 주요 변경사항 없음 |
| `unit_test_framework` | ⚠️ 검토 | 1.80에서 API 변경 |
| `context` | ✅ 안정 | 주요 변경사항 없음 |
| `locale` | ✅ 안정 | 주요 변경사항 없음 |
| `coroutine` | ⚠️ DEPRECATED | `coroutine2`로 마이그레이션 고려 |

### Coroutine Deprecation

**이슈**: `boost::coroutine`이 `boost::coroutine2`를 위해 deprecated됨

**필요 조치**:
1. `boost/coroutine/` include 검색
2. `coroutine2`로의 마이그레이션 작업량 평가
3. 코루틴이 여전히 필요한지 고려 (향후 C++20 코루틴 사용 가능)

---

## 구현 계획

### Phase 1: 분석 및 준비 (1주차)

1. **deprecated API 사용 검색**
   ```bash
   # io_service 사용
   grep -r "io_service" src/ programs/ extensions/

   # Filesystem v3 특정 API
   grep -r "boost::filesystem::copy_directory" src/

   # Deprecated 헤더
   grep -r "boost/function_output_iterator.hpp" src/
   grep -r "boost/system/cygwin_error.hpp" src/

   # Coroutine 사용
   grep -r "boost/coroutine/" src/
   ```

2. **Docker 빌드 환경 업데이트**
   - 여러 Boost 버전으로 테스트 (1.58, 1.74, 1.83, 1.89)
   - 필요시 Ubuntu 버전 업데이트 (24.04는 Boost 1.83 포함)

### Phase 2: CMake 구성 (1주차)

1. **CMakeLists.txt 업데이트**

   현재 (line 159):
   ```cmake
   FIND_PACKAGE(Boost 1.58 REQUIRED COMPONENTS ${BOOST_COMPONENTS})
   ```

   최소 버전을 1.58로 유지, 호환성 노트 추가:
   ```cmake
   FIND_PACKAGE(Boost 1.58 REQUIRED COMPONENTS ${BOOST_COMPONENTS})
   # Note: Tested with Boost 1.58-1.89
   ```

2. **버전 호환성 체크 추가**
   ```cmake
   if(Boost_VERSION VERSION_GREATER_EQUAL 1.58 AND Boost_VERSION VERSION_LESS 1.90)
       message(STATUS "Boost version ${Boost_VERSION} is supported")
   else()
       message(WARNING "Boost version ${Boost_VERSION} may not be fully tested")
   endif()
   ```

3. **조건부 컴파일 플래그 추가 (필요시)**
   ```cmake
   # Filesystem v3는 Boost 1.58-1.88에서 기본값, 필요한 경우에만 오버라이드
   # add_definitions(-DBOOST_FILESYSTEM_VERSION=3)
   ```

### Phase 3: 코드 수정 (2-3주차)

1. **io_service/io_context 호환성 레이어 구현**
   - `src/base/fc/include/fc/asio.hpp`에 버전 기반 typedef 추가
   - `boost::asio::io_service` 대신 `fc::io_service_t`를 사용하도록 모든 파일 업데이트
   - Boost 1.58, 1.66, 1.86, 1.87, 1.89로 테스트하여 호환성 검증

2. **deprecated include 검토**
   - deprecated 헤더 경로 검색
   - 새 버전과 기존 Boost 버전 모두에서 코드 작동 확인
   - 빌드 중 컴파일러 경고 모니터링

3. **각 컴포넌트 개별 테스트**
   - 여러 Boost 버전(1.58, 1.74, 1.83, 1.89)으로 빌드
   - 각 버전에서 단위 테스트 실행
   - 버전 간 동작 변경사항 없음 확인

### Phase 4: 테스트 (3-4주차)

1. **여러 Boost 버전에서 빌드**
   - Boost 1.58 (최소 지원, io_service만)
   - Boost 1.60 (이전 기준선)
   - Boost 1.66 (io_context 도입)
   - Boost 1.74 (현재 테스트 기준선)
   - Boost 1.77 (filesystem v4 도입)
   - Boost 1.80 (여러 변경사항)
   - Boost 1.85 (System 라이브러리 변경)
   - Boost 1.86 (io_service typedef가 있는 마지막 버전)
   - Boost 1.87 (io_service 제거)
   - Boost 1.89 (최신 지원 버전)

2. **전체 테스트 스위트 실행**
   ```bash
   # 테스트 빌드
   cmake -DBUILD_STEEM_TESTNET=ON -DCMAKE_BUILD_TYPE=Debug ..
   make -j$(nproc) chain_test plugin_test

   # 모든 테스트 실행
   ./tests/chain_test --log_level=all
   ./tests/plugin_test --log_level=all
   ```

3. **플랫폼별 테스트**
   - Ubuntu 16.04 (Boost 1.58) - 최소 지원
   - Ubuntu 22.04 (Boost 1.74) - 현재 테스트 기준선
   - Ubuntu 24.04 (Boost 1.83) - 최신 Linux
   - macOS Homebrew (Boost 1.89) - 최신 macOS
   - 재현성을 위한 Docker 컨테이너

4. **회귀 테스트**
   - 모든 주요 작업 테스트 (transfer, vote, witness update 등)
   - 모든 플러그인 개별 테스트
   - API 엔드포인트 테스트
   - P2P 네트워킹 테스트

### Phase 5: 문서 업데이트 (4주차)

1. **빌드 문서 업데이트**
   - `README.md` - Boost 버전 요구사항 업데이트 (1.58-1.89)
   - `docs/getting-started/building.md` - 모든 플랫폼에 대해 업데이트
   - `docs/development/docker-build-ubuntu22.md` - 다중 버전 지원으로 업데이트
   - `CLAUDE.md` - 의존성 정보 업데이트

2. **호환성 가이드 작성**
   - 알려진 호환성 고려사항 문서화
   - 다양한 Boost 버전으로 빌드하기 위한 노트 제공
   - 필요시 버전별 workaround 문서화

3. **CI/CD 업데이트**
   - 여러 Boost 버전을 테스트하도록 빌드 스크립트 업데이트
   - 다양한 Boost 버전으로 Docker 이미지 업데이트
   - 버전 매트릭스로 CI 플랫폼에서 테스트

### Phase 6: 배포 (5주차)

1. **단계적 배포**
   - development 브랜치에 머지
   - 다양한 Boost 버전으로 테스트넷에서 테스트
   - 커뮤니티 테스트 기간
   - main 브랜치에 머지

2. **커뮤니케이션**
   - 릴리스 노트에 광범위한 버전 지원 공지
   - 설치 가이드 업데이트
   - 시스템 제공 Boost 버전 사용 가능함을 사용자에게 알림

---

## 위험 평가

| 위험 | 확률 | 영향 | 완화 방안 |
|------|-------------|--------|------------|
| 버전별 API 이슈 | 중간 | 높음 | 하위 호환 가능한 API 계속 사용 (io_service, Filesystem v3) |
| 숨겨진 API deprecation | 중간 | 중간 | 모든 컴파일러 경고 활성화, 경고 신중히 검토 |
| 성능 차이 | 낮음 | 중간 | 다양한 Boost 버전에서 벤치마크 |
| 플랫폼별 이슈 | 중간 | 높음 | 여러 버전으로 모든 대상 플랫폼에서 테스트 |
| 특정 버전에서 테스트 실패 | 중간 | 높음 | 머지 전 수정, 모든 버전에서 100% 통과율 보장 |

---

## 롤백 계획

특정 Boost 버전에서 치명적인 이슈가 발견되는 경우:

1. **즉시**: 문제가 있는 버전을 문서화하고 작동하는 버전 권장
2. **단기**: 가능한 경우 버전별 workaround 생성
3. **장기**: 근본 원인 해결 또는 지원 버전 범위 조정

---

## 성공 기준

1. ✅ Boost 1.58-1.89로 성공적으로 빌드
2. ✅ 테스트된 모든 Boost 버전에서 모든 단위 테스트 통과 (chain_test, plugin_test)
3. ✅ deprecated API 관련 중요한 컴파일러 경고 없음
4. ✅ 성능 벤치마크에서 버전 간 중대한 회귀 없음 (>5% 느려짐)
5. ✅ Ubuntu 16.04, 22.04, 24.04, macOS에서 성공적으로 테스트
6. ✅ 문서 업데이트 및 정확성
7. ✅ 버전 매트릭스로 CI/CD 파이프라인 통과

---

## 타임라인 요약

| Phase | 기간 | 설명 |
|-------|----------|-------------|
| Phase 1 | 1주차 | 분석 및 준비 |
| Phase 2 | 1주차 | CMake 구성 |
| Phase 3 | 2-3주차 | 코드 수정 |
| Phase 4 | 3-4주차 | 테스트 |
| Phase 5 | 4주차 | 문서화 |
| Phase 6 | 5주차 | 배포 |

**총 예상 시간**: 4-5주

---

## 리소스

### Boost 릴리스 노트

- [Boost 1.74.0](https://www.boost.org/users/history/version_1_74_0.html)
- [Boost 1.77.0](https://www.boost.org/users/history/version_1_77_0.html) - Filesystem v4
- [Boost 1.80.0](https://www.boost.org/users/history/version_1_80_0.html) - Hash 변경
- [Boost 1.85.0](https://www.boost.org/users/history/version_1_85_0.html) - System 변경
- [Boost 1.89.0](https://www.boost.org/releases/latest/) - 최신 릴리스

### 마이그레이션 가이드

- [Boost.Asio Migration Guide](https://www.boost.org/doc/libs/1_89_0/doc/html/boost_asio/overview/core/async.html)
- [Boost.Filesystem v4 Migration](https://www.boost.org/doc/libs/1_89_0/libs/filesystem/doc/v4_design.htm)

---

## 부록: 빠른 참조 명령어

### 영향받는 코드 검색

```bash
# io_service 사용
grep -rn "io_service" src/ programs/ extensions/ | grep -v ".git"

# Deprecated 헤더
grep -rn "boost/function_output_iterator.hpp" src/
grep -rn "boost/system/cygwin_error.hpp" src/

# Filesystem copy_directory
grep -rn "copy_directory" src/

# Coroutine 사용
grep -rn "boost/coroutine/" src/ | grep -v "coroutine2"

# 커스텀 hash 특수화
grep -rn "std::hash.*boost::optional" src/
```

### 다양한 Boost 버전으로 빌드 (Docker)

```bash
# Boost 1.58 (Ubuntu 16.04)
docker run -it --rm -v $(pwd):/steem -w /steem ubuntu:16.04 bash

# Boost 1.74 (Ubuntu 22.04)
docker run -it --rm -v $(pwd):/steem -w /steem ubuntu:22.04 bash

# Boost 1.83 (Ubuntu 24.04)
docker run -it --rm -v $(pwd):/steem -w /steem ubuntu:24.04 bash

# 의존성 설치 및 빌드
apt update && apt install -y build-essential cmake git libboost-all-dev \
  libssl-dev libsnappy-dev libbz2-dev python3-jinja2 autoconf automake libtool
git submodule update --init --recursive
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) steemd
```

### Boost 버전 확인

```bash
# Ubuntu/Debian
dpkg -s libboost-dev | grep Version

# macOS Homebrew
brew info boost

# CMake 감지
echo '#include <boost/version.hpp>
#include <iostream>
int main() { std::cout << BOOST_LIB_VERSION << std::endl; }' > /tmp/boost_version.cpp
g++ /tmp/boost_version.cpp -o /tmp/boost_version && /tmp/boost_version
```

---

## 다음 단계

1. 이 업그레이드 계획 검토 및 승인
2. 엔지니어링 리소스 할당
3. 테스트 인프라 설정
4. Phase 1 분석 시작
5. 각 주요 작업에 대한 추적 이슈 생성

---

**문서 버전**: 1.0
**최종 업데이트**: 2025-10-19
**작성자**: Steem Development Team
