# Test Scripts

Helper scripts for running and managing tests.

## Overview

Utility scripts for:
- Generating test data
- Running comparative tests
- Account and block data extraction
- Test automation

## Scripts

| Script | Purpose |
|--------|---------|
| **[create_account_list.py](create_account_list.py)** | Extract all account names from blockchain |
| **[test_ah_get_account_history.sh](test_ah_get_account_history.sh)** | Run account history comparison tests |
| **[test_ah_get_ops_in_block.sh](test_ah_get_ops_in_block.sh)** | Run block operations comparison tests |

## create_account_list.py

Extract complete list of blockchain accounts to file.

### Usage

```bash
./create_account_list.py <server_address> <output_filename>

# Example
./create_account_list.py http://127.0.0.1:8090 accounts.txt
```

### Output

Creates text file with one account name per line:
```
steemit
alice
bob
charlie
...
```

### Use Cases

- Generate account list for test scripts
- Create targeted test datasets
- Account enumeration for testing

## test_ah_get_account_history.sh

Wrapper script for account history API testing.

### Usage

```bash
./test_ah_get_account_history.sh <args>
```

Calls [../api_tests/test_ah_get_account_history.py](../api_tests/test_ah_get_account_history.py) with provided arguments.

## test_ah_get_ops_in_block.sh

Wrapper script for block operations API testing.

### Usage

```bash
./test_ah_get_ops_in_block.sh <args>
```

Calls [../api_tests/test_ah_get_ops_in_block.py](../api_tests/test_ah_get_ops_in_block.py) with provided arguments.

## Common Workflows

### Generate Test Data

```bash
# Extract accounts
./create_account_list.py http://127.0.0.1:8090 accounts.txt

# Use in tests
../api_tests/test_ah_get_account_history.py 4 \
    http://node1:8090 \
    http://node2:8090 \
    ./results \
    accounts.txt
```

### Automated Testing

```bash
#!/bin/bash
# Generate account list
./create_account_list.py http://127.0.0.1:8090 accounts.txt

# Run account history tests
./test_ah_get_account_history.sh 4 \
    http://127.0.0.1:8090 \
    http://127.0.0.1:8091 \
    ./ah_results \
    accounts.txt

# Run block operations tests
./test_ah_get_ops_in_block.sh 4 \
    http://127.0.0.1:8090 \
    http://127.0.0.1:8091 \
    1 \
    10000 \
    ./ops_results
```

## Dependencies

### Python Scripts

```bash
# Python 3.6+
pip install requests  # For HTTP requests
```

### Shell Scripts

```bash
# Ensure executable
chmod +x *.sh
chmod +x *.py
```

## Additional Resources

- [API Tests](../api_tests/README.md)
- [Python API Tests](../../python_scripts/tests/api_tests/README.md)
- [Smoketest](../smoketest/README.md)

## License

See [LICENSE](../../LICENSE.md)
