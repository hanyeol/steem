# 디렉토리 구조 재구성 제안

## 요약

이 문서는 Steem 코드베이스의 디렉토리 구성을 전면적으로 재구성하여 명확성, 유지보수성, 업계 표준과의 일치성을 향상시키는 제안입니다.

**현재 구조의 문제점:**
- `libraries/`는 그 내용을 정확하게 반영하지 못하는 모호한 이름입니다
- `external_plugins/`는 외부 코드를 암시하지만 실제로는 내장되어 있습니다
- 플러그인, 지갑, 핵심 컴포넌트가 단일 "libraries" 디렉토리 아래에 혼재되어 있습니다
- 블록체인 특화 코드와 범용 코드 간의 명확한 구분이 없습니다

**제안하는 해결책:**
`src/` 아래에 `core/` (블록체인 특화), `base/` (범용 프레임워크), `plugins/`, `wallet/`, `vendor/`를 포함하는 명확한 계층 구조로 재구성하고, 사용자 확장을 위한 `extensions/`를 추가합니다.

## 현재 구조

```
steem/
├── libraries/              # 모호한 이름, 혼재된 내용
│   ├── protocol/          # 프로토콜 정의
│   ├── chain/             # 블록체인 구현
│   ├── chainbase/         # 데이터베이스
│   ├── fc/                # 기초 C++ 라이브러리
│   ├── appbase/           # 애플리케이션 프레임워크
│   ├── net/               # 네트워킹
│   ├── utils/             # 유틸리티
│   ├── manifest/          # 매니페스트
│   ├── schema/            # 스키마
│   ├── plugins/           # 20개 이상의 플러그인 (왜 "libraries" 아래에?)
│   ├── wallet/            # 지갑 (실제로 "library"가 아님)
│   └── vendor/            # 서드파티 의존성
├── external_plugins/       # 오해의 소지가 있는 이름 (바이너리에 빌드됨)
├── example_plugins/        # 예제 플러그인
├── programs/              # 실행 파일
└── tests/                 # 테스트
```

### 현재 구조의 문제점

1. **"libraries"가 너무 일반적**
   - 핵심 블록체인과 지원 라이브러리를 구분하지 못함
   - 플러그인은 전통적인 의미의 "libraries"가 아님
   - 지갑은 기능 모듈이지 재사용 가능한 라이브러리가 아님

2. **"external_plugins"가 오해의 소지**
   - 이름이 외부 소스의 코드를 암시함
   - 실제로는 CMake를 통해 바이너리로 컴파일됨
   - 사용자가 여기에 확장을 추가하므로 "extensions"가 더 명확할 것

3. **명확한 계층이 없음**
   - `libraries/` 아래의 모든 것이 중요도에서 동등해 보임
   - 한눈에 의존성을 파악하기 어려움
   - 핵심 블록체인 로직이 지원 인프라와 혼재됨

4. **발견 가능성이 낮음**
   - 새로운 개발자가 코드 구성을 이해하기 어려움
   - 새로운 컴포넌트를 추가할 위치에 대한 명확한 패턴이 없음
   - 컴포넌트 간의 관계가 불분명함

## 제안하는 구조

```
steem/
├── src/
│   ├── core/              # 블록체인 특화 핵심 컴포넌트
│   │   ├── protocol/      # Steem 프로토콜 정의
│   │   ├── chain/         # 블록체인 구현
│   │   └── chainbase/     # 블록체인 데이터베이스
│   ├── base/              # 범용 기본 라이브러리
│   │   ├── fc/            # 기초 C++ 라이브러리
│   │   ├── appbase/       # 애플리케이션 프레임워크
│   │   ├── net/           # P2P 네트워킹
│   │   ├── utils/         # 공통 유틸리티
│   │   ├── manifest/      # 매니페스트 시스템
│   │   └── schema/        # 스키마 정의
│   ├── plugins/           # 플러그인 확장
│   │   ├── chain/
│   │   ├── witness/
│   │   ├── account_history/
│   │   ├── apis/
│   │   │   ├── database_api/
│   │   │   └── ...
│   │   └── ...
│   ├── wallet/            # 지갑 기능
│   └── vendor/            # 서드파티 의존성
│       └── rocksdb/
├── extensions/            # 사용자 확장
│   └── examples/          # 예제 확장
├── programs/              # 실행 프로그램
│   ├── steemd/           # 메인 데몬
│   ├── cli_wallet/       # 커맨드라인 지갑
│   └── util/             # 유틸리티
└── tests/                # 테스트 스위트
```

## 제안하는 구조의 장점

### 1. 명확한 컴포넌트 역할

**src/core/** - 블록체인 핵심
- Steem 특화 블록체인 로직
- 다른 프로젝트에서 재사용 불가
- 포함: protocol, chain, chainbase

**src/base/** - 기본 인프라
- 범용 프레임워크 및 라이브러리
- 다른 프로젝트에서 재사용 가능
- 포함: fc, appbase, net, utilities 등

**src/plugins/** - 플러그인
- 블록체인 기능 확장 모듈
- core + base에 의존
- 핵심 라이브러리와 명확히 구분됨

**src/wallet/** - 지갑 모듈
- 지갑 특화 기능
- "library"가 아닌 기능 모듈
- cli_wallet 프로그램과의 관계가 명확함

**src/vendor/** - 서드파티 코드
- 외부 의존성
- 우리 코드가 아님을 명확히 표시

**extensions/** - 사용자 확장
- 사용자가 추가한 확장 기능
- 이름이 목적을 명확히 나타냄
- src/plugins/와의 관계가 명확함
- examples/ 서브디렉토리에 학습용 예제 포함

### 2. 의존성 계층

```
vendor/           (레벨 0: 외부 의존성)
    ↓
base/             (레벨 1: 기본 프레임워크)
    ↓
core/             (레벨 2: 블록체인 엔진)
    ↓
plugins/ + wallet/ (레벨 3: 확장 및 기능)
    ↓
programs/         (레벨 4: 실행 파일)
```

### 3. 업계 표준과의 일치

**Bitcoin Core:**
```
bitcoin/src/
├── consensus/    (우리의 core/와 유사)
├── crypto/       (우리의 base/와 유사)
├── wallet/       (우리의 wallet/과 유사)
└── util/         (우리의 base/utilities/와 유사)
```

**Ethereum:**
```
ethereum/core/
├── blockchain/
├── state/
└── ...
```

**표준 C++ 프로젝트:**
```
project/src/
├── core/
├── lib/
└── ...
```

### 4. 더 나은 구성

| 측면 | 현재 | 제안 | 개선 사항 |
|--------|---------|----------|-------------|
| 명확성 | ⭐⭐ | ⭐⭐⭐⭐⭐ | 역할이 즉시 명확함 |
| 일관성 | ⭐⭐ | ⭐⭐⭐⭐⭐ | 모든 소스가 src/ 아래에 |
| 발견 가능성 | ⭐⭐ | ⭐⭐⭐⭐⭐ | 직관적인 탐색 |
| 표준 준수 | ⭐⭐ | ⭐⭐⭐⭐⭐ | 업계 패턴과 일치 |
| 확장성 | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | 코드 추가 위치가 명확함 |

## 컴포넌트 분류

### Core vs Base: 결정 기준

**`src/core/` 사용 조건:**
- 컴포넌트가 Steem 블록체인에 특화됨
- 다른 블록체인 프로젝트에서 재사용 불가
- Steem 프로토콜 또는 합의 알고리즘을 구현
- 예: 프로토콜 연산, 체인 평가자, Steem 특화 데이터베이스 객체

**`src/base/` 사용 조건:**
- 컴포넌트가 범용적임
- 다른 프로젝트에서 사용 가능
- 인프라 또는 프레임워크를 제공
- 예: 직렬화 라이브러리, 플러그인 프레임워크, 네트워킹 스택

### Plugin vs Core: 결정 기준

**`src/core/` 사용 조건:**
- 컴포넌트가 블록체인 동작에 필수적
- 합의 로직의 일부
- 비활성화 불가

**`src/plugins/` 사용 조건:**
- 컴포넌트가 선택적
- 블록체인 기능을 확장
- 런타임에 활성화/비활성화 가능
- API 또는 추가 기능을 제공

## 마이그레이션 계획

### 1단계: 준비 (1주차)

1. **마이그레이션 브랜치 생성**
   ```bash
   git checkout -b refactor/directory-restructure
   ```

2. **현재 상태 문서화**
   - 모든 파일과 그 목적 나열
   - 컴포넌트 간 의존성 문서화
   - 잠재적 파괴적 변경 사항 식별

3. **커뮤니케이션 준비**
   - 커뮤니티에 계획된 변경 사항 발표
   - 개발자를 위한 마이그레이션 가이드 작성
   - CI/CD 설정 업데이트

### 2단계: 디렉토리 생성 (2주차)

1. **새 디렉토리 구조 생성**
   ```bash
   mkdir -p src/core src/base src/plugins src/wallet src/vendor
   ```

2. **CMake 구조 설정**
   - 각 새 디렉토리에 CMakeLists.txt 생성
   - 의존성 기반 빌드 순서 설정

### 3단계: 컴포넌트 마이그레이션 (3-4주차)

히스토리를 보존하기 위해 `git mv`를 사용하여 이동:

**핵심 컴포넌트:**
```bash
git mv libraries/protocol src/core/protocol
git mv libraries/chain src/core/chain
git mv libraries/chainbase src/core/chainbase
```

**기본 컴포넌트:**
```bash
git mv libraries/fc src/base/fc
git mv libraries/appbase src/base/appbase
git mv libraries/net src/base/net
git mv libraries/utilities src/base/utilities
git mv libraries/manifest src/base/manifest
git mv libraries/schema src/base/schema
```

**플러그인, 지갑, 벤더:**
```bash
git mv libraries/plugins src/plugins
git mv libraries/wallet src/wallet
git mv libraries/vendor src/vendor
```

**확장 및 예제:**
```bash
mkdir -p extensions
git mv example_plugins extensions/examples
git mv external_plugins extensions/
```

**정리:**
```bash
rmdir libraries
```

### 4단계: 빌드 시스템 업데이트 (5주차)

1. **루트 CMakeLists.txt**
   ```cmake
   # 이전
   add_subdirectory( external_plugins )
   add_subdirectory( libraries )

   # 새로운
   add_subdirectory( extensions )
   add_subdirectory( src )
   ```

2. **src/CMakeLists.txt** (새 파일)
   ```cmake
   # 빌드 순서: 의존성 우선
   add_subdirectory( vendor )
   add_subdirectory( base )
   add_subdirectory( core )
   add_subdirectory( plugins )
   add_subdirectory( wallet )
   ```

3. **src/base/CMakeLists.txt** (새 파일)
   ```cmake
   add_subdirectory( fc )
   add_subdirectory( schema )
   add_subdirectory( appbase )
   add_subdirectory( net )
   add_subdirectory( utilities )
   add_subdirectory( manifest )
   ```

4. **src/core/CMakeLists.txt** (새 파일)
   ```cmake
   add_subdirectory( chainbase )
   add_subdirectory( protocol )
   add_subdirectory( chain )
   ```

### 5단계: 코드 업데이트 (6-7주차)

1. **Include 경로 업데이트**
   - 적절한 CMake target_include_directories를 사용하면 대부분의 include 경로를 변경하지 않아도 됨
   - 스크립트나 문서의 하드코딩된 경로 업데이트

2. **빌드 스크립트 업데이트**
   - CI/CD 파이프라인
   - Docker 빌드 스크립트
   - 개발 헬퍼 스크립트

3. **철저한 테스트**
   ```bash
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_STEEM_TESTNET=ON ..
   make -j$(nproc)
   ./tests/chain_test
   ./tests/plugin_test
   ```

### 6단계: 문서화 (8주차)

1. **핵심 문서 업데이트**
   - README.md
   - CLAUDE.md
   - 모든 시작 가이드
   - 개발 가이드

2. **마이그레이션 가이드 작성**
   - 모든 변경 사항 문서화
   - 로컬 리포지토리 업데이트를 위한 스크립트 제공
   - 외부 도구에 대한 파괴적 변경 사항 나열

3. **API 문서 업데이트**
   - Doxygen 설정
   - API 참조 경로
   - 예제 코드 스니펫

### 7단계: 리뷰 및 릴리스 (9-10주차)

1. **코드 리뷰**
   - 모든 변경 사항의 전체 리뷰
   - 모든 테스트가 통과하는지 확인
   - 누락된 참조가 있는지 확인

2. **커뮤니티 테스트**
   - 커뮤니티 테스트를 위한 베타 릴리스
   - 피드백 수집
   - 발견된 문제 해결

3. **최종 릴리스**
   - 메인 브랜치에 병합
   - 새 버전 태그 (예: v1.0.0)
   - 릴리스 노트 게시

## 위험 평가 및 완화

### 고위험: 외부 도구 파괴

**위험:** 이전 경로를 참조하는 외부 도구 및 스크립트가 작동하지 않음.

**완화:**
- 한 릴리스 주기 동안 임시 심볼릭 링크 생성
- 자동화된 마이그레이션 스크립트 제공
- 릴리스 노트에 파괴적 변경 사항을 명확히 문서화
- 한 메이저 버전 동안 호환성 레이어 유지

**심볼릭 링크 설정 예:**
```bash
# 임시 역호환성 제공
ln -s src/core/protocol libraries/protocol
ln -s src/core/chain libraries/chain
# ... 기타 등등
```

### 중위험: Git 히스토리 복잡성

**위험:** 파일 이동으로 인해 git blame 추적이 어려워질 수 있음.

**완화:**
- 히스토리를 보존하기 위해 `git mv` 사용
- 재구성 커밋 해시 문서화
- 파일 히스토리 추적을 위해 `git log --follow` 사용
- 히스토리 추적 팁을 기여 가이드라인에 업데이트

### 중위험: 빌드 시스템 문제

**위험:** CMake 설정 오류로 빌드가 중단될 수 있음.

**완화:**
- 여러 플랫폼에서 테스트 (Linux, macOS, Docker)
- 다양한 빌드 설정 테스트 (Release, Debug, TestNet)
- 일시적으로 병렬 빌드 지원 유지
- 병합 전 종합적인 CI/CD 테스트

### 저위험: 개발자 혼란

**위험:** 개발자가 새 코드를 어디에 넣어야 할지 모를 수 있음.

**완화:**
- 의사결정 트리가 포함된 명확한 문서
- 새 구조로 CLAUDE.md 업데이트
- 기여 가이드에 예제 제공
- PR 가이던스를 위한 적극적인 모니터링

## 일정

| 단계 | 기간 | 설명 |
|-------|----------|-------------|
| 1. 준비 | 1주 | 계획 및 커뮤니케이션 |
| 2. 디렉토리 생성 | 1주 | 새 구조 설정 |
| 3. 컴포넌트 마이그레이션 | 2주 | git mv로 파일 이동 |
| 4. 빌드 시스템 | 1주 | CMake 업데이트 |
| 5. 코드 업데이트 | 2주 | 경로 수정 및 테스트 |
| 6. 문서화 | 1주 | 모든 문서 업데이트 |
| 7. 리뷰 및 릴리스 | 2주 | 테스트 및 릴리스 |
| **전체** | **10주** | **완전한 마이그레이션** |

## 성공 기준

### 필수 사항
- ✅ 모든 컴포넌트가 성공적으로 이동됨
- ✅ 모든 테스트가 통과함 (chain_test, plugin_test)
- ✅ 지원되는 모든 플랫폼에서 빌드 성공
- ✅ 문서가 완전히 업데이트됨
- ✅ 기능 저하 없음

### 권장 사항
- ✅ 외부 개발자를 위한 마이그레이션 가이드
- ✅ 역호환성 심볼릭 링크
- ✅ 업데이트된 CI/CD 파이프라인
- ✅ 커뮤니티 승인

### 선택 사항
- ✅ 포크를 위한 자동화된 마이그레이션 스크립트
- ✅ 새 구조 설명 비디오
- ✅ 업데이트된 아키텍처 다이어그램

## 마이그레이션 후 개선 사항

재구성이 완료되면 다음과 같은 후속 개선 사항이 더 쉬워집니다:

1. **모듈형 빌드 시스템**
   - 특정 컴포넌트만 빌드 가능
   - 더 빠른 증분 빌드
   - 더 나은 컴포넌트 격리

2. **플러그인 개발 키트**
   - base/를 독립형 SDK로 추출
   - 더 쉬운 서드파티 플러그인 개발
   - 더 명확한 플러그인 API 경계

3. **코드 구성 규칙**
   - 적절한 컴포넌트 배치를 위한 자동화된 검사
   - 의존성 순환 감지
   - 레이어 위반 감지

4. **문서 생성**
   - 자동 컴포넌트 다이어그램
   - 의존성 그래프
   - 레이어별 API 문서

## 대안 비교

### 대안 1: 최소 변경 ("libraries" 유지)

**장점:**
- 마이그레이션 불필요
- 파괴적 변경 없음
- 위험 제로

**단점:**
- 혼란스러운 구조 지속
- 기술 부채 유지
- 개발자 경험 저하

**평가:** ❌ 권장하지 않음 - 핵심 문제를 해결하지 못함

### 대안 2: 평면 "src" 디렉토리

```
src/
├── protocol/
├── chain/
├── plugins/
└── ...
```

**장점:**
- 단순하고 평면적인 구조
- 탐색이 쉬움
- 깊은 중첩이 없음

**단점:**
- 논리적 그룹화 없음
- 확장성이 낮음
- 관계 이해가 어려움

**평가:** ❌ 권장하지 않음 - 계층 구조의 장점을 잃음

### 대안 3: 제안된 구조 (src/core + src/base)

**장점:**
- 명확한 계층 및 그룹화
- 업계 표준과 일치
- 확장성이 좋음
- 자체 문서화된 구조

**단점:**
- 마이그레이션 노력 필요
- 일시적인 중단

**평가:** ✅ **권장** - 최선의 장기적 솔루션

## 자주 묻는 질문

### Q: "libraries"를 "src"로 단순히 이름 변경하지 않는 이유는?

A: 단순한 이름 변경은 구성 문제를 해결하지 못합니다. 내부 구조(core, base, plugins, wallet 혼재)는 여전히 명확성을 위해 재구성이 필요합니다.

### Q: "core"와 "base"를 분리하는 이유는?

A: 이 분리는 의존성을 명확하게 만듭니다. "core"는 Steem 특화 블록체인 코드이고, "base"는 재사용 가능한 프레임워크를 포함합니다. 이는 개발자가 코드가 무엇인지 이해하는 데 도움이 되며, 향후 "base"를 독립형 라이브러리로 추출할 수 있게 합니다.

### Q: 내 빌드가 중단되나요?

A: 업데이트된 코드를 가져와서 처음부터 다시 빌드하면 중단되지 않습니다. 증분 빌드는 문제가 있을 수 있으므로 권장 사항: `rm -rf build && mkdir build && cd build && cmake ..`

### Q: 이전 구조를 계속 사용할 수 있나요?

A: 한 릴리스 주기 동안 역호환성을 위한 심볼릭 링크를 제공합니다. 그러나 이러한 심볼릭 링크는 다음 메이저 버전에서 제거되므로 새 구조로 마이그레이션해야 합니다.

### Q: 커스텀 플러그인은 어떻게 되나요?

A: `external_plugins/`의 커스텀 플러그인을 `extensions/`로 이동해야 합니다. 빌드 시스템이 새 위치에서 자동으로 감지합니다. 마이그레이션 스크립트를 제공할 예정입니다.

### Q: 언제 이런 일이 일어나나요?

A: 다음 메이저 버전 릴리스(v1.0 또는 v2.0)에 제안됩니다. 이를 통해 적절한 계획을 세우고 커뮤니티가 준비할 시간을 제공합니다.

## 부록: 상세 파일 매핑

### 핵심 컴포넌트
| 이전 경로 | 새 경로 | 이유 |
|----------|----------|--------|
| libraries/protocol/ | src/core/protocol/ | Steem 프로토콜 정의 |
| libraries/chain/ | src/core/chain/ | 블록체인 구현 |
| libraries/chainbase/ | src/core/chainbase/ | 블록체인 데이터베이스 |

### 기본 컴포넌트
| 이전 경로 | 새 경로 | 이유 |
|----------|----------|--------|
| libraries/fc/ | src/base/fc/ | 범용 C++ 라이브러리 |
| libraries/appbase/ | src/base/appbase/ | 플러그인 프레임워크 |
| libraries/net/ | src/base/net/ | 네트워킹 스택 |
| libraries/utilities/ | src/base/utilities/ | 공통 유틸리티 |
| libraries/manifest/ | src/base/manifest/ | 매니페스트 시스템 |
| libraries/schema/ | src/base/schema/ | 스키마 정의 |

### 기타 컴포넌트
| 이전 경로 | 새 경로 | 이유 |
|----------|----------|--------|
| libraries/plugins/ | src/plugins/ | 플러그인 모듈 |
| libraries/wallet/ | src/wallet/ | 지갑 기능 |
| libraries/vendor/ | src/vendor/ | 서드파티 코드 |
| external_plugins/ | extensions/ | 사용자 확장 |
| example_plugins/ | extensions/examples/ | 예제 확장 |

## 결론

이 재구성은 Steem 코드베이스의 근본적인 구성 문제를 해결합니다. 초기 노력이 필요하지만, 명확성, 유지보수성, 업계 표준과의 일치성 측면에서의 장점은 프로젝트의 장기적 건전성을 위한 가치 있는 투자입니다.

제안된 구조:
- ✅ 관심사를 명확히 분리함 (core vs base vs plugins)
- ✅ 업계 모범 사례를 따름
- ✅ 개발자 경험을 향상시킴
- ✅ 향후 모듈화를 가능하게 함
- ✅ 의존성을 명시적으로 만듦

**권장 사항:** 다음 메이저 버전 릴리스에서 명시된 마이그레이션 계획에 따라 이 재구성을 진행하십시오.

---

**문서 버전:** 1.0
**작성일:** 2025-11-12
**상태:** 제안
**대상 버전:** v1.0.0 또는 v2.0.0
