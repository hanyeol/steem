# Build Helpers

Python modules and utilities for the Steem build process.

## Overview

Build helpers provide Python utilities used during CMake builds:
- Module concatenation (hardfork definitions)
- Code generation helpers
- Build-time utilities

## Components

### steem_build_helpers

Python module with build utilities.

**Key Functions:**
- `cat_parts` - Concatenate file parts into single file
- Used for assembling hardfork definitions from parts

## Usage

### cat_parts

Concatenate parts from a directory:

```python
python3 -m steem_build_helpers.cat_parts input_dir output_file
```

**Example:**

```bash
python3 -m steem_build_helpers.cat_parts \
  libraries/protocol/hardfork.d \
  libraries/protocol/include/steem/protocol/hardfork.hpp
```

This is automatically called by CMake during build.

## Build Integration

Used in CMakeLists.txt:

```cmake
add_custom_command(
  COMMAND ${CMAKE_COMMAND} -E env
    PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/../../programs/build_helpers
  python3 -m steem_build_helpers.cat_parts
    "${CMAKE_CURRENT_SOURCE_DIR}/hardfork.d"
    ${hardfork_hpp_file}
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/include/steem/protocol/hardfork.hpp"
  DEPENDS ${HF_PARTS}
)
```

## Requirements

- Python 3
- No additional dependencies

## Development

Build helpers are automatically available during build.
No manual setup required.
