# Boost 1.58-1.89 Compatibility Plan

This document outlines the compatibility considerations for supporting Boost versions 1.58 through 1.89 in Steem.

## Overview

**Supported Range**: Boost 1.58.0 - 1.89.0
**Current Testing**: Boost 1.74.0 (as of recent upgrade from 1.58-1.60)
**Estimated Effort**: Medium complexity
**Breaking Changes**: 6 major areas identified (1.74 → 1.89)

## Motivation

- **Wide Platform Support**: Support both legacy systems (Ubuntu 16.04 with Boost 1.58) and modern platforms (Ubuntu 24.04, macOS Homebrew with Boost 1.83-1.89)
- **Flexibility**: Allow users to build with system-provided Boost versions without forcing specific versions
- **Bug Fixes & Performance**: Benefit from improvements across 31 Boost releases (1.58-1.89)
- **Security Updates**: Enable users on modern systems to use versions with critical security fixes
- **C++17 Compatibility**: Better support for C++17 features in newer Boost versions (Steem recently upgraded to C++17)

## Major Breaking Changes (1.74 → 1.89)

### 1. C++ Standard Requirements

**Change**: Minimum C++ standard requirements increased across many libraries

- **Boost.System** (1.85): Now requires C++11 minimum (GCC 4.8+, MSVC 14.0/VS2015+)
- **Boost.Math** (1.76): Dropped C++03 support, requires C++11
- **Boost.Math** (1.80): C++11 deprecated, C++14 minimum recommended
- **Boost.PFR** (1.85): Now explicitly requires C++14

**Impact**: ✅ **COMPATIBLE** - Steem uses C++17 (as of recent upgrade from C++14)

**Action Required**: None - Steem already meets these requirements

---

### 2. Boost.Asio Changes (io_service → io_context)

**Change**: `boost::asio::io_service` renamed to `boost::asio::io_context` (Boost 1.66+)

**Timeline**:
- Boost 1.58-1.65: Only `io_service` exists
- Boost 1.66-1.86: Both `io_service` (as typedef) and `io_context` exist
- Boost 1.87+: `io_service` removed, only `io_context` exists

**Files Affected**:
- `src/base/fc/include/fc/asio.hpp` - Line 75: `boost::asio::io_service& default_io_service()`
- `src/base/fc/src/asio.cpp` - Implementation of io_service
- `src/plugins/webserver/webserver_plugin.cpp` - Uses io_service
- `src/plugins/witness/witness_plugin.cpp` - Uses io_service
- `src/base/appbase/include/appbase/application.hpp` - May use io_service

**Action Required**:
Implement version-based conditional compilation to support all Boost versions (1.58-1.89):

1. **Create compatibility typedef in `fc/asio.hpp`**:
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

2. **Update all usages**:
   - Replace `boost::asio::io_service` with `fc::io_service_t`
   - Replace `boost::asio::io_context` with `fc::io_service_t`
   - This allows compilation with Boost 1.58-1.89

3. **Alternative approach** (if minimal changes preferred):
   - For Boost 1.87+, add compatibility typedef:
   ```cpp
   #if BOOST_VERSION >= 108700  // Boost 1.87.0+
   namespace boost { namespace asio {
       using io_service = io_context;
   }}
   #endif
   ```

**Priority**: HIGH - Core networking functionality, blocks Boost 1.87+ support

---

### 3. Boost.Filesystem v4

**Change**: Boost 1.77 introduced Filesystem v4 with breaking API changes

**Key Changes**:
- `copy_directory()` deprecated in 1.74, replaced by `create_directory()` overload
- Path comparison behavior changes
- Better std::filesystem compatibility (C++17)

**Default Behavior**:
- Boost 1.74-1.88: Uses Filesystem v3 by default
- Future versions: Will default to v4

**Control Macro**: `BOOST_FILESYSTEM_VERSION` (set to 3 or 4)

**Action Required**:
1. Search for `boost::filesystem::copy_directory` usage (if any)
2. Keep using Filesystem v3 for compatibility with all Boost versions (1.58-1.89)
   - v3 is default in Boost 1.58-1.88
   - Can explicitly define `BOOST_FILESYSTEM_VERSION=3` in CMake if needed
3. Test filesystem operations thoroughly across different Boost versions

**Priority**: MEDIUM - Impact depends on filesystem usage

---

### 4. Boost.Hash Changes

**Change**: Deprecations and reorganization of hash-related headers

**Affected**:
- `boost/container_hash/detail/container_fwd.hpp` - Deprecation warning added (1.74)
- `boost::unordered::hash_is_avalanching` - Now a using-declaration (1.89)
- `<boost/unordered/hash_traits.hpp>` - Will be removed in future

**Action Required**:
1. Search for usage of deprecated hash headers
2. Replace with recommended alternatives
3. Monitor compiler warnings during build

**Priority**: LOW - Likely minimal usage

---

### 5. Boost.Iterator

**Change**: `boost/function_output_iterator.hpp` deprecated (1.74)

**Replacement**: `boost/iterator/function_output_iterator.hpp`

**Action Required**:
```bash
grep -r "boost/function_output_iterator.hpp" src/
```
Replace deprecated include with new location.

**Priority**: LOW

---

### 6. Boost.System

**Change**:
- `boost/system/cygwin_error.hpp` removed (Boost 1.85)
- Original MinGW (mingw.org, 32-bit only) no longer supported (Boost 1.85)
- MinGW-w64 (32-bit/64-bit) continues to be supported

**Impact**:
- ✅ **No Action Needed** for most users
- Boost.System widely used in codebase:
  - `boost::system::error_code` - ASIO error handling
  - `boost::system::system_error` - Network/filesystem exceptions
  - Used in: `fc/asio.cpp`, `fc/network/*.cpp`, `fc/filesystem.cpp`

**Verification**:
1. ✅ **Cygwin header**: Not used (verified via grep)
2. ⚠️ **MinGW support**: CMakeLists.txt has MinGW build configuration (line 196)
   - If building on Windows with MinGW, ensure using **MinGW-w64** (not original MinGW)
   - MinGW-w64 supports both 32-bit (`i686-w64-mingw32`) and 64-bit (`x86_64-w64-mingw32`)

**Action Required**:
- **Linux/macOS**: No action needed
- **Windows MinGW users**:
  1. Verify using MinGW-w64 (not original mingw.org)
  2. Update documentation to specify MinGW-w64 requirement

**Priority**: LOW - Platform-specific, no code changes needed

---

## Dependencies Analysis

### Current Boost Components Used

From `CMakeLists.txt` lines 43-53:
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

### Component-Specific Considerations

| Component | Status | Notes |
|-----------|--------|-------|
| `thread` | ⚠️ REVIEW | Monitor promise/future API changes |
| `date_time` | ✅ STABLE | No major breaking changes |
| `filesystem` | ⚠️ REVIEW | v3→v4 transition |
| `program_options` | ✅ STABLE | No major breaking changes |
| `serialization` | ✅ STABLE | No major breaking changes |
| `chrono` | ✅ STABLE | No major breaking changes |
| `unit_test_framework` | ⚠️ REVIEW | API changes in 1.80 |
| `context` | ✅ STABLE | No major breaking changes |
| `locale` | ✅ STABLE | No major breaking changes |
| `coroutine` | ⚠️ DEPRECATED | Consider migrating to `coroutine2` |

### Coroutine Deprecation

**Issue**: `boost::coroutine` is deprecated in favor of `boost::coroutine2`

**Action Required**:
1. Search for `boost/coroutine/` includes
2. Evaluate migration effort to `coroutine2`
3. Consider whether coroutines are still needed (may use C++20 coroutines in future)

---

## Implementation Plan

### Phase 1: Analysis & Preparation (Week 1)

1. **Search for deprecated API usage**
   ```bash
   # io_service usage
   grep -r "io_service" src/ programs/ extensions/

   # Filesystem v3 specific APIs
   grep -r "boost::filesystem::copy_directory" src/

   # Deprecated headers
   grep -r "boost/function_output_iterator.hpp" src/
   grep -r "boost/system/cygwin_error.hpp" src/

   # Coroutine usage
   grep -r "boost/coroutine/" src/
   ```

2. **Update Docker build environment**
   - Test with multiple Boost versions (1.58, 1.74, 1.83, 1.89)
   - Update Ubuntu version if needed (24.04 has Boost 1.83)

### Phase 2: CMake Configuration (Week 1)

1. **Update CMakeLists.txt**

   Current (line 159):
   ```cmake
   FIND_PACKAGE(Boost 1.58 REQUIRED COMPONENTS ${BOOST_COMPONENTS})
   ```

   Keep minimum version at 1.58, add compatibility note:
   ```cmake
   FIND_PACKAGE(Boost 1.58 REQUIRED COMPONENTS ${BOOST_COMPONENTS})
   # Note: Tested with Boost 1.58-1.89
   ```

2. **Add version compatibility checks**
   ```cmake
   if(Boost_VERSION VERSION_GREATER_EQUAL 1.58 AND Boost_VERSION VERSION_LESS 1.90)
       message(STATUS "Boost version ${Boost_VERSION} is supported")
   else()
       message(WARNING "Boost version ${Boost_VERSION} may not be fully tested")
   endif()
   ```

3. **Add conditional compilation flags (if needed)**
   ```cmake
   # Filesystem v3 is default in Boost 1.58-1.88, only override if necessary
   # add_definitions(-DBOOST_FILESYSTEM_VERSION=3)
   ```

### Phase 3: Code Modifications (Week 2-3)

1. **Implement io_service/io_context compatibility layer**
   - Add version-based typedef to `src/base/fc/include/fc/asio.hpp`
   - Update all files to use `fc::io_service_t` instead of `boost::asio::io_service`
   - Test with Boost 1.58, 1.66, 1.86, 1.87, 1.89 to verify compatibility

2. **Review deprecated includes**
   - Search for deprecated header paths
   - Ensure code works with both old and new Boost versions
   - Monitor compiler warnings during build

3. **Test each component individually**
   - Build with multiple Boost versions (1.58, 1.74, 1.83, 1.89)
   - Run unit tests with each version
   - Ensure no behavioral changes across versions

### Phase 4: Testing (Week 3-4)

1. **Build on multiple Boost versions**
   - Boost 1.58 (minimum supported, io_service only)
   - Boost 1.60 (previous baseline)
   - Boost 1.66 (io_context introduced)
   - Boost 1.74 (current testing baseline)
   - Boost 1.77 (filesystem v4 introduction)
   - Boost 1.80 (multiple changes)
   - Boost 1.85 (System library changes)
   - Boost 1.86 (last version with io_service typedef)
   - Boost 1.87 (io_service removed)
   - Boost 1.89 (latest supported)

2. **Run full test suite**
   ```bash
   # Build tests
   cmake -DBUILD_STEEM_TESTNET=ON -DCMAKE_BUILD_TYPE=Debug ..
   make -j$(nproc) chain_test plugin_test

   # Run all tests
   ./tests/chain_test --log_level=all
   ./tests/plugin_test --log_level=all
   ```

3. **Platform-specific testing**
   - Ubuntu 16.04 (Boost 1.58) - minimum supported
   - Ubuntu 22.04 (Boost 1.74) - current testing baseline
   - Ubuntu 24.04 (Boost 1.83) - modern Linux
   - macOS Homebrew (Boost 1.89) - modern macOS
   - Docker containers for reproducibility

4. **Regression testing**
   - Test all major operations (transfer, vote, witness update, etc.)
   - Test all plugins individually
   - Test API endpoints
   - Test P2P networking

### Phase 5: Documentation Updates (Week 4)

1. **Update build documentation**
   - `README.md` - Update Boost version requirements (1.58-1.89)
   - `docs/getting-started/building.md` - Update for all platforms
   - `docs/development/docker-build-ubuntu22.md` - Update with multi-version support
   - `CLAUDE.md` - Update dependency information

2. **Create compatibility guide**
   - Document known compatibility considerations
   - Provide notes for building with different Boost versions
   - Document any version-specific workarounds if needed

3. **Update CI/CD**
   - Update build scripts to test multiple Boost versions
   - Update Docker images with different Boost versions
   - Test on CI platforms with version matrix

### Phase 6: Rollout (Week 5)

1. **Staged deployment**
   - Merge to development branch
   - Test on testnets with different Boost versions
   - Community testing period
   - Merge to main branch

2. **Communication**
   - Announce wide version support in release notes
   - Update installation guides
   - Notify users that system-provided Boost versions can be used

---

## Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Version-specific API issues | MEDIUM | HIGH | Keep using backward-compatible APIs (io_service, Filesystem v3) |
| Hidden API deprecations | MEDIUM | MEDIUM | Enable all compiler warnings, review warnings carefully |
| Performance differences | LOW | MEDIUM | Benchmark across different Boost versions |
| Platform-specific issues | MEDIUM | HIGH | Test on all target platforms with multiple versions |
| Test failures on specific versions | MEDIUM | HIGH | Fix before merging, ensure 100% pass rate on all versions |

---

## Rollback Plan

If critical issues are discovered with specific Boost versions:

1. **Immediate**: Document the problematic version and recommend working versions
2. **Short-term**: Create version-specific workarounds if possible
3. **Long-term**: Address root causes or adjust supported version range

---

## Success Criteria

1. ✅ Builds successfully with Boost 1.58-1.89
2. ✅ All unit tests pass with all tested Boost versions (chain_test, plugin_test)
3. ✅ No critical compiler warnings related to deprecated APIs
4. ✅ Performance benchmarks show no significant regression across versions (>5% slowdown)
5. ✅ Successfully tested on Ubuntu 16.04, 22.04, 24.04, and macOS
6. ✅ Documentation updated and accurate
7. ✅ CI/CD pipeline passing with version matrix

---

## Timeline Summary

| Phase | Duration | Description |
|-------|----------|-------------|
| Phase 1 | Week 1 | Analysis & Preparation |
| Phase 2 | Week 1 | CMake Configuration |
| Phase 3 | Week 2-3 | Code Modifications |
| Phase 4 | Week 3-4 | Testing |
| Phase 5 | Week 4 | Documentation |
| Phase 6 | Week 5 | Rollout |

**Total Estimated Time**: 4-5 weeks

---

## Resources

### Boost Release Notes

- [Boost 1.74.0](https://www.boost.org/users/history/version_1_74_0.html)
- [Boost 1.77.0](https://www.boost.org/users/history/version_1_77_0.html) - Filesystem v4
- [Boost 1.80.0](https://www.boost.org/users/history/version_1_80_0.html) - Hash changes
- [Boost 1.85.0](https://www.boost.org/users/history/version_1_85_0.html) - System changes
- [Boost 1.89.0](https://www.boost.org/releases/latest/) - Latest release

### Migration Guides

- [Boost.Asio Migration Guide](https://www.boost.org/doc/libs/1_89_0/doc/html/boost_asio/overview/core/async.html)
- [Boost.Filesystem v4 Migration](https://www.boost.org/doc/libs/1_89_0/libs/filesystem/doc/v4_design.htm)

---

## Appendix: Quick Reference Commands

### Search for Affected Code

```bash
# io_service usage
grep -rn "io_service" src/ programs/ extensions/ | grep -v ".git"

# Deprecated headers
grep -rn "boost/function_output_iterator.hpp" src/
grep -rn "boost/system/cygwin_error.hpp" src/

# Filesystem copy_directory
grep -rn "copy_directory" src/

# Coroutine usage
grep -rn "boost/coroutine/" src/ | grep -v "coroutine2"

# Custom hash specializations
grep -rn "std::hash.*boost::optional" src/
```

### Build with Different Boost Versions (Docker)

```bash
# Boost 1.58 (Ubuntu 16.04)
docker run -it --rm -v $(pwd):/steem -w /steem ubuntu:16.04 bash

# Boost 1.74 (Ubuntu 22.04)
docker run -it --rm -v $(pwd):/steem -w /steem ubuntu:22.04 bash

# Boost 1.83 (Ubuntu 24.04)
docker run -it --rm -v $(pwd):/steem -w /steem ubuntu:24.04 bash

# Install dependencies and build
apt update && apt install -y build-essential cmake git libboost-all-dev \
  libssl-dev libsnappy-dev libbz2-dev python3-jinja2 autoconf automake libtool
git submodule update --init --recursive
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) steemd
```

### Check Boost Version

```bash
# Ubuntu/Debian
dpkg -s libboost-dev | grep Version

# macOS Homebrew
brew info boost

# CMake detect
echo '#include <boost/version.hpp>
#include <iostream>
int main() { std::cout << BOOST_LIB_VERSION << std::endl; }' > /tmp/boost_version.cpp
g++ /tmp/boost_version.cpp -o /tmp/boost_version && /tmp/boost_version
```

---

## Next Steps

1. Review and approve this upgrade plan
2. Assign engineering resources
3. Set up testing infrastructure
4. Begin Phase 1 analysis
5. Create tracking issues for each major task

---

**Document Version**: 1.0
**Last Updated**: 2025-10-19
**Author**: Steem Development Team
