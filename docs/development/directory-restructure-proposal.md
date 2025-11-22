# Directory Structure Restructure Proposal

## Executive Summary

This document proposes a comprehensive restructuring of the Steem codebase directory organization to improve clarity, maintainability, and alignment with industry standards.

**Current Structure Issues:**
- `libraries/` is a vague name that doesn't accurately reflect its contents
- `external_plugins/` suggests external code but is actually built-in
- Plugins, wallet, and core components are mixed under a single "libraries" directory
- No clear separation between blockchain-specific and general-purpose code

**Proposed Solution:**
Reorganize into a clear hierarchical structure with `src/` containing `core/` (blockchain-specific), `base/` (general-purpose frameworks), `plugins/`, `wallet/`, and `vendor/`.

## Current Structure

```
steem/
├── libraries/              # Vague name, mixed contents
│   ├── protocol/          # Protocol definitions
│   ├── chain/             # Blockchain implementation
│   ├── chainbase/         # Database
│   ├── fc/                # Foundational C++ library
│   ├── appbase/           # Application framework
│   ├── net/               # Networking
│   ├── utils/             # Utilities
│   ├── jsonball/          # JSON processing
│   ├── manifest/          # Manifest
│   ├── schema/            # Schema
│   ├── plugins/           # 20+ plugins (why under "libraries"?)
│   ├── wallet/            # Wallet (not really a "library")
│   └── vendor/            # Third-party dependencies
├── external_plugins/       # Misleading name (built into binary)
├── example_plugins/        # Example plugins
├── programs/              # Executables
└── tests/                 # Tests
```

### Problems with Current Structure

1. **"libraries" is too generic**
   - Doesn't distinguish between core blockchain and supporting libraries
   - Plugins are not "libraries" in the traditional sense
   - Wallet is a feature module, not a reusable library

2. **"external_plugins" is misleading**
   - Name suggests code from external sources
   - Actually compiled into the binary via CMake
   - Users add custom plugins here, so "custom_plugins" would be clearer

3. **No clear hierarchy**
   - Everything under `libraries/` appears equal in importance
   - Hard to understand dependencies at a glance
   - Core blockchain logic mixed with supporting infrastructure

4. **Poor discoverability**
   - New developers struggle to understand code organization
   - No clear pattern for where to add new components
   - Relationship between components unclear

## Proposed Structure

```
steem/
├── src/
│   ├── core/              # Blockchain-specific core components
│   │   ├── protocol/      # Steem protocol definitions
│   │   ├── chain/         # Blockchain implementation
│   │   └── chainbase/     # Blockchain database
│   ├── base/              # General-purpose base libraries
│   │   ├── fc/            # Foundational C++ library
│   │   ├── appbase/       # Application framework
│   │   ├── net/           # P2P networking
│   │   ├── utils/         # Common utilities
│   │   ├── jsonball/      # JSON processing
│   │   ├── manifest/      # Manifest system
│   │   └── schema/        # Schema definitions
│   ├── plugins/           # Plugin extensions
│   │   ├── chain/
│   │   ├── witness/
│   │   ├── account_history/
│   │   ├── apis/
│   │   │   ├── database_api/
│   │   │   ├── condenser_api/
│   │   │   └── ...
│   │   └── ...
│   ├── wallet/            # Wallet functionality
│   └── vendor/            # Third-party dependencies
│       └── rocksdb/
├── custom_plugins/        # User-added custom plugins
├── example_plugins/       # Example plugins for learning
├── programs/              # Executable programs
│   ├── steemd/           # Main daemon
│   ├── cli_wallet/       # Command-line wallet
│   └── util/             # Utilities
└── tests/                # Test suites
```

## Benefits of Proposed Structure

### 1. Clear Component Roles

**src/core/** - Blockchain Core
- Steem-specific blockchain logic
- Not reusable in other projects
- Contains: protocol, chain, chainbase

**src/base/** - Base Infrastructure
- General-purpose frameworks and libraries
- Could be reused in other projects
- Contains: fc, appbase, net, utilities, etc.

**src/plugins/** - Plugins
- Extension modules for blockchain functionality
- Depends on core + base
- Clearly separated from core libraries

**src/wallet/** - Wallet Module
- Wallet-specific functionality
- Not a "library" but a feature module
- Relationship to cli_wallet program is clear

**src/vendor/** - Third-party Code
- External dependencies
- Clearly marked as not our code

**custom_plugins/** - User Extensions
- Custom plugins added by users
- Name clearly indicates purpose
- Relationship to src/plugins/ is obvious

### 2. Dependency Hierarchy

```
vendor/           (Level 0: External dependencies)
    ↓
base/             (Level 1: Base frameworks)
    ↓
core/             (Level 2: Blockchain engine)
    ↓
plugins/ + wallet/ (Level 3: Extensions and features)
    ↓
programs/         (Level 4: Executables)
```

### 3. Industry Alignment

**Bitcoin Core:**
```
bitcoin/src/
├── consensus/    (similar to our core/)
├── crypto/       (similar to our base/)
├── wallet/       (similar to our wallet/)
└── util/         (similar to our base/utilities/)
```

**Ethereum:**
```
ethereum/core/
├── blockchain/
├── state/
└── ...
```

**Standard C++ Projects:**
```
project/src/
├── core/
├── lib/
└── ...
```

### 4. Better Organization

| Aspect | Current | Proposed | Improvement |
|--------|---------|----------|-------------|
| Clarity | ⭐⭐ | ⭐⭐⭐⭐⭐ | Roles immediately clear |
| Consistency | ⭐⭐ | ⭐⭐⭐⭐⭐ | All source under src/ |
| Discoverability | ⭐⭐ | ⭐⭐⭐⭐⭐ | Intuitive navigation |
| Standards | ⭐⭐ | ⭐⭐⭐⭐⭐ | Matches industry patterns |
| Extensibility | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | Clear where to add code |

## Component Classification

### Core vs Base: Decision Criteria

**Use `src/core/` if:**
- Component is Steem blockchain-specific
- Cannot be reused in other blockchain projects
- Implements Steem protocol or consensus
- Examples: protocol operations, chain evaluators, Steem-specific database objects

**Use `src/base/` if:**
- Component is general-purpose
- Could be used in other projects
- Provides infrastructure or framework
- Examples: serialization library, plugin framework, networking stack

### Plugin vs Core: Decision Criteria

**Use `src/core/` if:**
- Component is required for blockchain operation
- Part of consensus logic
- Cannot be disabled

**Use `src/plugins/` if:**
- Component is optional
- Extends blockchain functionality
- Can be enabled/disabled at runtime
- Provides API or additional features

## Migration Plan

### Phase 1: Preparation (Week 1)

1. **Create Migration Branch**
   ```bash
   git checkout -b refactor/directory-restructure
   ```

2. **Document Current State**
   - List all files and their purposes
   - Document dependencies between components
   - Identify potential breaking changes

3. **Prepare Communication**
   - Announce planned changes to community
   - Create migration guide for developers
   - Update CI/CD configurations

### Phase 2: Directory Creation (Week 2)

1. **Create New Directory Structure**
   ```bash
   mkdir -p src/core src/base src/plugins src/wallet src/vendor
   ```

2. **Set Up CMake Structure**
   - Create CMakeLists.txt for each new directory
   - Configure build order based on dependencies

### Phase 3: Component Migration (Week 3-4)

Execute moves using `git mv` to preserve history:

**Core Components:**
```bash
git mv libraries/protocol src/core/protocol
git mv libraries/chain src/core/chain
git mv libraries/chainbase src/core/chainbase
```

**Base Components:**
```bash
git mv libraries/fc src/base/fc
git mv libraries/appbase src/base/appbase
git mv libraries/net src/base/net
git mv libraries/utilities src/base/utilities
git mv libraries/jsonball src/base/jsonball
git mv libraries/manifest src/base/manifest
git mv libraries/schema src/base/schema
```

**Plugins, Wallet, Vendor:**
```bash
git mv libraries/plugins src/plugins
git mv libraries/wallet src/wallet
git mv libraries/vendor src/vendor
```

**Clean Up:**
```bash
rmdir libraries
git mv external_plugins custom_plugins
```

### Phase 4: Build System Updates (Week 5)

1. **Root CMakeLists.txt**
   ```cmake
   # Old
   add_subdirectory( external_plugins )
   add_subdirectory( libraries )

   # New
   add_subdirectory( custom_plugins )
   add_subdirectory( src )
   ```

2. **src/CMakeLists.txt** (new file)
   ```cmake
   # Build order: dependencies first
   add_subdirectory( vendor )
   add_subdirectory( base )
   add_subdirectory( core )
   add_subdirectory( plugins )
   add_subdirectory( wallet )
   ```

3. **src/base/CMakeLists.txt** (new file)
   ```cmake
   add_subdirectory( fc )
   add_subdirectory( schema )
   add_subdirectory( jsonball )
   add_subdirectory( appbase )
   add_subdirectory( net )
   add_subdirectory( utilities )
   add_subdirectory( manifest )
   ```

4. **src/core/CMakeLists.txt** (new file)
   ```cmake
   add_subdirectory( chainbase )
   add_subdirectory( protocol )
   add_subdirectory( chain )
   ```

### Phase 5: Code Updates (Week 6-7)

1. **Update Include Paths**
   - Most include paths can remain unchanged if using proper CMake target_include_directories
   - Update any hard-coded paths in scripts or documentation

2. **Update Build Scripts**
   - CI/CD pipelines
   - Docker build scripts
   - Development helper scripts

3. **Test Thoroughly**
   ```bash
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_STEEM_TESTNET=ON ..
   make -j$(nproc)
   ./tests/chain_test
   ./tests/plugin_test
   ```

### Phase 6: Documentation (Week 8)

1. **Update Core Documentation**
   - README.md
   - CLAUDE.md
   - All getting-started guides
   - Development guides

2. **Create Migration Guide**
   - Document all changes
   - Provide script for updating local repositories
   - List breaking changes for external tools

3. **Update API Documentation**
   - Doxygen configurations
   - API reference paths
   - Example code snippets

### Phase 7: Review and Release (Week 9-10)

1. **Code Review**
   - Full review of all changes
   - Verify all tests pass
   - Check for any missed references

2. **Community Testing**
   - Beta release for community testing
   - Gather feedback
   - Fix any discovered issues

3. **Final Release**
   - Merge to main branch
   - Tag new version (e.g., v1.0.0)
   - Publish release notes

## Risk Assessment and Mitigation

### High Risk: Breaking External Tools

**Risk:** External tools and scripts that reference old paths will break.

**Mitigation:**
- Create temporary symlinks for one release cycle
- Provide automated migration script
- Clearly document breaking changes in release notes
- Maintain compatibility layer for one major version

**Example Symlink Setup:**
```bash
# Provide temporary backward compatibility
ln -s src/core/protocol libraries/protocol
ln -s src/core/chain libraries/chain
# ... etc
```

### Medium Risk: Git History Complexity

**Risk:** File moves might make git blame harder to follow.

**Mitigation:**
- Use `git mv` to preserve history
- Document the restructure commit hash
- Use `git log --follow` to track file history
- Update contributing guidelines with history tracking tips

### Medium Risk: Build System Issues

**Risk:** CMake configuration errors could break builds.

**Mitigation:**
- Test on multiple platforms (Linux, macOS, Docker)
- Test different build configurations (Release, Debug, TestNet)
- Maintain parallel build support temporarily
- Comprehensive CI/CD testing before merge

### Low Risk: Developer Confusion

**Risk:** Developers might not know where to put new code.

**Mitigation:**
- Clear documentation with decision trees
- Update CLAUDE.md with new structure
- Provide examples in contribution guide
- Active monitoring of PRs for guidance

## Timeline

| Phase | Duration | Description |
|-------|----------|-------------|
| 1. Preparation | 1 week | Planning and communication |
| 2. Directory Creation | 1 week | Set up new structure |
| 3. Component Migration | 2 weeks | Move files with git mv |
| 4. Build System | 1 week | Update CMake |
| 5. Code Updates | 2 weeks | Fix paths and test |
| 6. Documentation | 1 week | Update all docs |
| 7. Review & Release | 2 weeks | Testing and release |
| **Total** | **10 weeks** | **Complete migration** |

## Success Criteria

### Must Have
- ✅ All components successfully moved
- ✅ All tests pass (chain_test, plugin_test)
- ✅ Builds successfully on all supported platforms
- ✅ Documentation fully updated
- ✅ No regression in functionality

### Should Have
- ✅ Migration guide for external developers
- ✅ Backward compatibility symlinks
- ✅ Updated CI/CD pipelines
- ✅ Community approval

### Nice to Have
- ✅ Automated migration script for forks
- ✅ Video walkthrough of new structure
- ✅ Updated architecture diagrams

## Post-Migration Improvements

Once the restructure is complete, these follow-up improvements become easier:

1. **Modular Build System**
   - Enable building only specific components
   - Faster incremental builds
   - Better component isolation

2. **Plugin Development Kit**
   - Extract base/ as standalone SDK
   - Easier third-party plugin development
   - Clearer plugin API boundaries

3. **Code Organization Rules**
   - Automated checks for proper component placement
   - Dependency cycle detection
   - Layer violation detection

4. **Documentation Generation**
   - Automatic component diagrams
   - Dependency graphs
   - API documentation by layer

## Comparison with Alternatives

### Alternative 1: Minimal Change (Keep "libraries")

**Pros:**
- No migration needed
- No breaking changes
- Zero risk

**Cons:**
- Continues confusing structure
- Technical debt remains
- Poor developer experience

**Verdict:** ❌ Not recommended - doesn't solve core problems

### Alternative 2: Flat "src" Directory

```
src/
├── protocol/
├── chain/
├── plugins/
└── ...
```

**Pros:**
- Simple, flat structure
- Easy to navigate
- No deep nesting

**Cons:**
- No logical grouping
- Doesn't scale well
- Hard to understand relationships

**Verdict:** ❌ Not recommended - loses hierarchy benefits

### Alternative 3: Proposed Structure (src/core + src/base)

**Pros:**
- Clear hierarchy and grouping
- Aligns with industry standards
- Scales well
- Self-documenting structure

**Cons:**
- Requires migration effort
- Temporary disruption

**Verdict:** ✅ **Recommended** - best long-term solution

## Frequently Asked Questions

### Q: Why not just rename "libraries" to "src"?

A: Simply renaming doesn't solve the organizational problems. The internal structure (mixing core, base, plugins, wallet) still needs reorganization for clarity.

### Q: Why separate "core" and "base"?

A: This separation makes dependencies clear. "core" is Steem-specific blockchain code, while "base" contains reusable frameworks. This helps developers understand what code is what, and enables potential extraction of "base" as a standalone library in the future.

### Q: Will this break my build?

A: Not if you pull the updated code and rebuild from scratch. Incremental builds might have issues, so we recommend: `rm -rf build && mkdir build && cd build && cmake ..`

### Q: Can I keep using the old structure?

A: For one release cycle, we'll provide symlinks for backward compatibility. However, you should migrate to the new structure as these symlinks will be removed in the next major version.

### Q: What about my custom plugins?

A: Custom plugins in `external_plugins/` should be moved to `custom_plugins/`. The build system will automatically detect them in the new location. We'll provide a migration script.

### Q: When will this happen?

A: Proposed for the next major version release (v1.0 or v2.0). This allows proper planning and gives the community time to prepare.

## Appendix: Detailed File Mapping

### Core Components
| Old Path | New Path | Reason |
|----------|----------|--------|
| libraries/protocol/ | src/core/protocol/ | Steem protocol definitions |
| libraries/chain/ | src/core/chain/ | Blockchain implementation |
| libraries/chainbase/ | src/core/chainbase/ | Blockchain database |

### Base Components
| Old Path | New Path | Reason |
|----------|----------|--------|
| libraries/fc/ | src/base/fc/ | General-purpose C++ library |
| libraries/appbase/ | src/base/appbase/ | Plugin framework |
| libraries/net/ | src/base/net/ | Networking stack |
| libraries/utilities/ | src/base/utilities/ | Common utilities |
| libraries/jsonball/ | src/base/jsonball/ | JSON processing |
| libraries/manifest/ | src/base/manifest/ | Manifest system |
| libraries/schema/ | src/base/schema/ | Schema definitions |

### Other Components
| Old Path | New Path | Reason |
|----------|----------|--------|
| libraries/plugins/ | src/plugins/ | Plugin modules |
| libraries/wallet/ | src/wallet/ | Wallet functionality |
| libraries/vendor/ | src/vendor/ | Third-party code |
| external_plugins/ | custom_plugins/ | User custom plugins |

## Conclusion

This restructuring addresses fundamental organizational issues in the Steem codebase. While it requires upfront effort, the benefits in clarity, maintainability, and alignment with industry standards make it a worthwhile investment for the project's long-term health.

The proposed structure:
- ✅ Clearly separates concerns (core vs base vs plugins)
- ✅ Follows industry best practices
- ✅ Improves developer experience
- ✅ Enables future modularization
- ✅ Makes dependencies explicit

**Recommendation:** Proceed with this restructuring in the next major version release, following the outlined migration plan.

---

**Document Version:** 1.0
**Created:** 2025-11-12
**Status:** Proposal
**Target Version:** v1.0.0 or v2.0.0
