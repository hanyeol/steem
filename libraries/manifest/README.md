# Manifest Library

Plugin manifest system that aggregates all available plugins.

## Overview

The `manifest` library:
- Collects all plugins from `libraries/plugins/`
- Creates the `steem_plugins` build target
- Enables dynamic plugin discovery
- Integrates external plugins

## Purpose

Provides a single linkage point for all plugins, allowing applications to:
- Link against all available plugins with one dependency
- Discover plugins at build time
- Integrate external plugins seamlessly

## How It Works

The manifest iterates through plugin directories and:
1. Finds all plugin CMakeLists.txt files
2. Adds them to the `steem_plugins` target
3. Makes them available to `steemd` and other programs

## Build

```bash
cd build
make steem_plugins  # Builds all plugins via manifest
```

## Usage

Applications link to the manifest:

```cmake
target_link_libraries( steemd steem_plugins )
```

This automatically includes all available plugins.

## Dependencies

- All plugins in `libraries/plugins/`
- All plugins in `external_plugins/`
