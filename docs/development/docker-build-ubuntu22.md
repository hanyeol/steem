# Building and Testing on Ubuntu 22.04 Docker

This guide explains how to build and test Steem on Ubuntu 22.04 using Docker, which is useful for macOS ARM64 (Apple Silicon M1/M2/M3) users who cannot build natively due to Boost version compatibility issues.

**Why Docker?** macOS Homebrew provides Boost 1.89, which has breaking changes (e.g., `io_service` removed). Ubuntu 22.04 provides Boost 1.74, which is the primary upgrade target.

## Prerequisites

- Docker Desktop for Mac (Apple Silicon) installed
- At least 4GB of free disk space
- 4GB+ RAM allocated to Docker
- Rosetta 2 is NOT required (uses native ARM64 Ubuntu image)

## Quick Start

### 1. Start Ubuntu 22.04 Container (ARM64)

From your Steem project directory:

```bash
docker run -it --rm \
  --platform linux/arm64 \
  -v $(pwd):/steem \
  -w /steem \
  ubuntu:22.04 \
  bash
```

**Options:**
- `--platform linux/arm64`: Use native ARM64 architecture (for Apple Silicon)
- `-it`: Interactive terminal
- `--rm`: Remove container after exit
- `-v $(pwd):/steem`: Mount current directory to `/steem` in container
- `-w /steem`: Set working directory to `/steem`

### 2. Install Dependencies

Inside the container:

```bash
# Update package list
apt update

# Install build essentials
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

# Verify Boost version (should be 1.74)
dpkg -s libboost-dev | grep Version
```

### 3. Build Steem

```bash
# Initialize git submodules (if not already done)
git submodule update --init --recursive

# Create build directory
rm -rf build && mkdir build && cd build

# Configure with CMake
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build steemd (use -j2 for 2 cores, adjust based on available CPU)
make -j2 steemd

# Build cli_wallet
make -j2 cli_wallet
```

**Build time:** Approximately 30-60 minutes depending on CPU cores.

### 4. Run Unit Tests

```bash
# Configure for testnet build
cd /steem
rm -rf build && mkdir build && cd build
cmake -DBUILD_STEEM_TESTNET=ON -DCMAKE_BUILD_TYPE=Debug ..

# Build test executables
make -j2 chain_test
make -j2 plugin_test

# Run all tests
./tests/chain_test
./tests/plugin_test

# Run specific test suite
./tests/chain_test --run_test=basic_tests
./tests/chain_test --run_test=block_tests
./tests/chain_test --run_test=operation_tests

# Run single test
./tests/chain_test -t basic_tests/parse_size_test

# Verbose output
./tests/chain_test --log_level=all

# List all available tests
./tests/chain_test --list_content
```

## Advanced Usage

### Persistent Container

If you want to keep the container and reuse it:

```bash
# Create named container
docker run -it \
  --platform linux/arm64 \
  --name steem-build \
  -v $(pwd):/steem \
  -w /steem \
  ubuntu:22.04 \
  bash

# After exiting, restart the container
docker start -i steem-build

# Remove container when done
docker rm steem-build
```

### Build with Different Options

#### Low Memory Node (Consensus Only)

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DLOW_MEMORY_NODE=ON ..
make -j2 steemd
```

#### Skip Transaction ID Index (65% CPU savings during reindex)

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DSKIP_BY_TX_ID=ON ..
make -j2 steemd
```

#### Enable Code Coverage

```bash
cmake -DBUILD_STEEM_TESTNET=ON \
      -DENABLE_COVERAGE_TESTING=ON \
      -DCMAKE_BUILD_TYPE=Debug ..
make -j2 chain_test

# Run tests
./tests/chain_test

# Generate coverage report
apt install -y lcov
lcov --capture --initial --directory . --output-file base.info --no-external
./tests/chain_test
lcov --capture --directory . --output-file test.info --no-external
lcov --add-tracefile base.info --add-tracefile test.info --output-file total.info
lcov -o interesting.info -r total.info tests/\*
mkdir -p lcov
genhtml interesting.info --output-directory lcov --prefix $(pwd)

# Coverage report is in lcov/index.html
# Copy to host to view in browser:
# exit container, then: docker cp steem-build:/steem/build/lcov ./lcov
```

## Docker Compose (Optional)

Create `docker-compose.yml` in your project root:

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

Then use:

```bash
docker-compose run --rm steem-build
```

## Troubleshooting

### Out of Memory

If build fails with "killed" or out of memory errors:

```bash
# Use fewer parallel jobs
make -j1 steemd  # Single thread build
```

Or increase Docker's memory limit in Docker Desktop settings.

### Submodule Issues

```bash
# Clean and reinitialize submodules
git submodule deinit -f .
git submodule update --init --recursive
```

### Clean Build

```bash
# Remove build directory and start fresh
cd /steem
rm -rf build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j2 steemd
```

## Testing Boost 1.74 Compatibility

After making changes for Boost 1.74 compatibility:

```bash
# 1. Start container (ARM64 for Apple Silicon)
docker run -it --rm --platform linux/arm64 -v $(pwd):/steem -w /steem ubuntu:22.04 bash

# 2. Install dependencies
apt update && apt install -y build-essential cmake git libboost-all-dev \
  libssl-dev libsnappy-dev libbz2-dev python3-jinja2 autoconf automake libtool pkg-config

# 3. Verify Boost version
dpkg -s libboost-dev | grep Version
# Should show: Version: 1.74.0.3ubuntu7

# 4. Clean build
git submodule update --init --recursive
rm -rf build && mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j2 steemd

# 5. Run tests
cd /steem
rm -rf build && mkdir build && cd build
cmake -DBUILD_STEEM_TESTNET=ON -DCMAKE_BUILD_TYPE=Debug ..
make -j2 chain_test plugin_test
./tests/chain_test
./tests/plugin_test
```

## Expected Results

### Successful Build Output

```
[100%] Built target steemd
```

### Successful Test Output

```
*** No errors detected
```

## Notes

- **Architecture**: Uses native ARM64 Ubuntu image for Apple Silicon Macs (no emulation overhead)
- **Boost Version**: Ubuntu 22.04 uses Boost 1.74.0, which is the primary target for this upgrade
- **macOS Boost Issue**: macOS Homebrew provides Boost 1.89 with breaking changes (`io_service` → `io_context`)
- **Build Artifacts**: Built binaries are located in `build/programs/steemd/steemd` and `build/programs/cli_wallet/cli_wallet`
- **Test Binaries**: Located in `build/tests/chain_test` and `build/tests/plugin_test`
- **Performance**: ARM64 native execution provides optimal build performance on Apple Silicon

## See Also

- [Building Steem](../getting-started/building.md) - General build instructions
- [Testing Guide](testing.md) - Comprehensive testing documentation
