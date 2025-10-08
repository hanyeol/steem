# Quick Basic Test Suite (Smoketest)

Automated integration testing framework for comparing Steem node implementations.

## Overview

Smoketest is a test harness that:
- Runs two steemd instances (test vs reference) with identical blockchain data
- Compares API responses between the instances
- Validates functionality across multiple test groups
- Generates detailed failure reports with JSON diff outputs

**Use Case**: Validate node upgrades, plugin changes, or API modifications don't break existing functionality.

## Quick Start

### Running Smoketest

From the smoketest directory:

```bash
./smoketest.sh \
    ~/working/steemd \
    ~/reference/steemd \
    ~/working_blockchain_dir \
    ~/reference_blockchain_dir \
    3000000 \
    12
```

### Parameters

```bash
./smoketest.sh <test_steemd> <ref_steemd> <test_blockchain_dir> <ref_blockchain_dir> <block_limit> [jobs] [--dont-copy-config]
```

| Parameter | Required | Description |
|-----------|----------|-------------|
| `test_steemd` | Yes | Path to steemd binary being tested |
| `ref_steemd` | Yes | Path to reference (known-good) steemd binary |
| `test_blockchain_dir` | Yes | Working directory for test instance |
| `ref_blockchain_dir` | Yes | Working directory for reference instance |
| `block_limit` | Yes | Maximum block number to replay/test |
| `jobs` | No | Parallel jobs (default: nproc) |
| `--dont-copy-config` | No | Skip copying config.ini files |

### Example

```bash
# Test new build against stable release
./smoketest.sh \
    /path/to/build/programs/steemd/steemd \
    /path/to/release/programs/steemd/steemd \
    ./test_data_dir \
    ./ref_data_dir \
    5000000 \
    8
```

## Test Groups

Smoketest runs multiple test groups, each validating different functionality:

| Test Group | Purpose | Config |
|------------|---------|--------|
| **[account_history/](account_history/)** | Account history API validation | [config.ini](account_history/config.ini) |
| **[ah_limit_account/](ah_limit_account/)** | Account history account limiting | Config |
| **[ah_limit_account_operation/](ah_limit_account_operation/)** | Account+operation filtering | Config |
| **[ah_limit_operations/](ah_limit_operations/)** | Operation type filtering | Config |
| **[votes/](votes/)** | Vote tracking and retrieval | Config |
| **[scripts/](scripts/)** | Helper scripts and utilities | - |

### Test Group Structure

Each test group directory contains:
- `test_group.sh` - Main test script
- `config.ini` - steemd configuration (or `config_ref.ini`/`config_test.ini` for separate configs)
- Test-specific scripts and data

## Test Group Interface

### test_group.sh

Each test group must implement `test_group.sh` with this interface:

```bash
#!/bin/bash
# Arguments:
#   $1 = JOBS (number of parallel jobs)
#   $2 = TEST_ADDRESS (test instance address)
#   $3 = TEST_PORT (test instance port)
#   $4 = REF_ADDRESS (reference instance address)
#   $5 = REF_PORT (reference instance port)
#   $6 = BLOCK_LIMIT (maximum block to process)

JOBS=$1
TEST_ADDRESS=$2
TEST_PORT=$3
REF_ADDRESS=$4
REF_PORT=$5
BLOCK_LIMIT=$6

# Run tests
# ...

# Return 0 on success, non-zero on failure
exit $EXIT_CODE
```

## Output and Logging

### Log Directory Structure

```
test_group/
├── log/                    # Created during test run
│   ├── account1.json      # Response diff for account1
│   ├── account2.json      # Response diff for account2
│   └── errors.log         # Error summary
├── config.ini             # Test configuration
└── test_group.sh          # Test script
```

### Log Files

**JSON Response Files**: Generated only when:
- API request fails
- Responses differ between test and reference

**Location**: `log/` folder inside each test group directory

**Format**: JSON diff showing expected vs actual responses

## Workflow

1. **Setup Phase**:
   - Copy blockchain data to working directories
   - Copy config files (unless `--dont-copy-config`)
   - Validate paths and binaries

2. **Execution Phase**:
   - Start test steemd instance
   - Start reference steemd instance
   - Wait for both to sync/replay

3. **Testing Phase**:
   - For each test group:
     - Run `test_group.sh`
     - Collect results
     - Generate logs on failure

4. **Reporting Phase**:
   - Aggregate results across groups
   - Print summary statistics
   - Exit with appropriate code

### Statistics Tracked

- `GROUP_TOTAL` - Total test groups
- `GROUP_PASSED` - Successful test groups
- `GROUP_SKIPPED` - Skipped test groups
- `GROUP_FAILURE` - Failed test groups

## Configuration

### Global Configuration

From [smoketest.sh](smoketest.sh):

```bash
API_TEST_PATH=../../python_scripts/tests/api_tests  # API test scripts location
BLOCK_SUBPATH=blockchain/block_log                  # Block log path
GROUP_TEST_SCRIPT=test_group.sh                     # Test script name
```

### Per-Group Configuration

#### Single config.ini

For identical configuration between test and reference:

```ini
# test_group/config.ini
plugin = chain p2p account_history account_history_api
webserver-http-endpoint = 127.0.0.1:8090
```

#### Separate Configurations

For different configurations:

```ini
# test_group/config_test.ini
plugin = chain p2p account_history_rocksdb
webserver-http-endpoint = 127.0.0.1:8090
```

```ini
# test_group/config_ref.ini
plugin = chain p2p account_history
webserver-http-endpoint = 127.0.0.1:8091
```

## Adding New Test Groups

### Step-by-Step

1. **Create test group directory**:
   ```bash
   mkdir smoketest/my_test_group
   cd smoketest/my_test_group
   ```

2. **Create test_group.sh**:
   ```bash
   #!/bin/bash
   JOBS=$1
   TEST_URL="http://$2:$3"
   REF_URL="http://$4:$5"
   BLOCK_LIMIT=$6

   # Run comparison tests
   ../../api_tests/test_my_api.py $JOBS $TEST_URL $REF_URL

   exit $?
   ```

3. **Create config.ini**:
   ```ini
   plugin = chain p2p my_plugin my_api
   webserver-http-endpoint = 127.0.0.1:8090
   ```

4. **Make executable**:
   ```bash
   chmod +x test_group.sh
   ```

5. **Run smoketest**:
   ```bash
   cd ..
   ./smoketest.sh test_steemd ref_steemd test_dir ref_dir 100000
   ```

## Docker Support

### Dockerfile

From [Dockerfile](Dockerfile):

```dockerfile
# Build and run smoketest in Docker
FROM ubuntu:20.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    python3 python3-pip

# Copy test files
COPY . /smoketest
WORKDIR /smoketest

# Run tests
CMD ["./docker_build_and_run.sh"]
```

### docker_build_and_run.sh

Automated Docker-based testing:

```bash
#!/bin/bash
# Build steemd instances in Docker
# Run smoketest
# Report results
```

Usage:
```bash
./docker_build_and_run.sh
```

## Example Test Groups

### account_history

Tests account_history API:
- `get_account_history` - Full account operation history
- Pagination correctness
- Virtual operation generation

### ah_limit_account

Tests account filtering:
- Track specific accounts only
- Verify untracked accounts return empty
- Resource usage optimization

### ah_limit_operations

Tests operation type filtering:
- Track specific operation types only
- Verify filtering correctness
- Performance with selective tracking

### votes

Tests vote tracking:
- `get_active_votes` API
- Vote weight calculations
- Voter list consistency

## Troubleshooting

### Smoketest Won't Start

**Error**: Binary not found
```bash
# Verify paths are absolute
realpath /path/to/steemd

# Ensure binaries are executable
chmod +x /path/to/steemd
```

### Config Copy Failures

**Error**: Cannot copy config.ini
```bash
# Skip config copy and create manually
./smoketest.sh ... --dont-copy-config

# Then create configs in working directories
vim test_blockchain_dir/config.ini
vim ref_blockchain_dir/config.ini
```

### Port Conflicts

**Error**: Address already in use
```bash
# Check for running instances
ps aux | grep steemd

# Kill existing instances
pkill -9 steemd

# Verify ports are free
netstat -tuln | grep 8090
```

### Test Group Failures

**Error**: test_group.sh returns non-zero

**Debug**:
1. Check `log/` directory for JSON diffs
2. Run test_group.sh manually:
   ```bash
   cd test_group
   ./test_group.sh 4 127.0.0.1 8090 127.0.0.1 8091 1000
   ```
3. Enable verbose output in Python scripts

### Python Script Errors

**Error**: Module not found
```bash
# Install dependencies
pip3 install requests

# Verify API test path
ls -la ../../python_scripts/tests/api_tests/
```

## Best Practices

1. **Use Absolute Paths**: Always provide absolute paths to binaries and directories
2. **Clean Working Directories**: Remove old data before testing
3. **Identical Blockchain Data**: Ensure test and reference start with same data
4. **Appropriate Block Limits**: Don't test beyond available blockchain data
5. **Parallel Jobs**: Set based on CPU cores and memory
6. **Review Logs**: Always check `log/` directories for failure details

## Continuous Integration

### Example CI Integration

```yaml
# .github/workflows/smoketest.yml
name: Smoketest
on: [push, pull_request]

jobs:
  smoketest:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2

      - name: Build test binary
        run: |
          mkdir build && cd build
          cmake -DCMAKE_BUILD_TYPE=Release ..
          make -j$(nproc) steemd

      - name: Download reference binary
        run: |
          wget https://github.com/steemit/steem/releases/download/v0.23.0/steemd
          chmod +x steemd

      - name: Setup blockchain data
        run: |
          # Download or generate test blockchain
          mkdir -p test_data ref_data
          # Copy blockchain files...

      - name: Run smoketest
        run: |
          cd tests/smoketest
          ./smoketest.sh \
            ../../build/programs/steemd/steemd \
            ../../steemd \
            ../../test_data \
            ../../ref_data \
            1000000 \
            4
```

## Performance

### Typical Runtime

- 1M blocks, 4 jobs: ~10-30 minutes
- 5M blocks, 8 jobs: ~1-2 hours
- 10M blocks, 16 jobs: ~3-5 hours

**Factors**:
- CPU cores available
- Disk I/O speed (SSD recommended)
- Network latency (for API calls)
- Number of test groups

### Optimization Tips

1. **Use SSD**: Significantly faster block replay
2. **Increase Jobs**: Scale with CPU cores
3. **Limit Block Range**: Test only necessary blocks
4. **Skip Validation**: Use `--skip-validate-invariants` in configs
5. **Disable Unnecessary Plugins**: Minimal plugin set in configs

## Additional Resources

- [API Tests](../api_tests/README.md)
- [Python API Tests](../../python_scripts/tests/api_tests/README.md)
- [Account History Plugin](../../libraries/plugins/account_history/README.md)
- [Testing Guide](../../doc/testing.md)

## License

See [LICENSE](../../LICENSE.md)
