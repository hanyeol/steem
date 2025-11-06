# Steem Blockchain

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.md)

Steem is a Delegated Proof of Stake (DPoS) blockchain that uses a "Proof of Brain" social consensus algorithm for token allocation.

**Currency Symbol**: STEEM

**Consensus**: Delegated Proof of Stake (DPoS) with 21 witnesses

**Inflation Distribution**:
- 75% to "Proof of Brain" social consensus algorithm
- 15% to stake holders (interest on vesting)
- 10% to block producers (witnesses)

**Inflation Rate**: 10% APR narrowing to 1% APR over 20 years

## Documentation

| Resource | Description |
|----------|-------------|
| **[Whitepaper](https://steem.io/SteemWhitePaper.pdf)** | Technical design and economic model |
| **[Quickstart Guide](docs/exchangequickstart.md)** | Get running quickly with Docker |
| **[Building Guide](docs/building.md)** | Build from source (Linux/macOS) |
| **[Testing Guide](docs/testing.md)** | Run and write tests |
| **[Plugin Development](docs/plugin.md)** | Create custom plugins |
| **[API Documentation](docs/api-notes.md)** | API usage notes |

## Quick Start

### Docker (Recommended)

#### Run a P2P Node

Minimal node for blockchain sync (requires ~2GB RAM):

```bash
docker run \
    -d -p 2001:2001 -p 8090:8090 --name steemd-default \
    hanyeol/steem

docker logs -f steemd-default
```

#### Run a Full API Node

Full node with all APIs enabled (requires ~16GB RAM, ~110GB disk):

```bash
docker run \
    --env USE_WAY_TOO_MUCH_RAM=1 \
    --env USE_FULL_WEB_NODE=1 \
    -d -p 2001:2001 -p 8090:8090 --name steemd-full \
    hanyeol/steem

docker logs -f steemd-full
```

### Build from Source

#### Ubuntu 16.04

```bash
# Install dependencies
sudo apt-get update && sudo apt-get install -y \
    autoconf automake cmake g++ git \
    libbz2-dev libsnappy-dev libssl-dev libtool \
    make pkg-config python3 python3-jinja2 \
    libboost-all-dev

# Clone and build
git clone https://github.com/hanyeol/steem.git
cd steem
git submodule update --init --recursive
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) steemd
make -j$(nproc) cli_wallet
```

#### macOS

```bash
# Install dependencies
brew install autoconf automake cmake git libtool \
    boost@1.60 openssl@1.0

# Build
export OPENSSL_ROOT_DIR=$(brew --prefix)/Cellar/openssl/1.0.2h_1/
export BOOST_ROOT=$(brew --prefix)/Cellar/boost@1.60/1.60.0/
git clone https://github.com/hanyeol/steem.git
cd steem
git submodule update --init --recursive
mkdir build && cd build
cmake -DBOOST_ROOT="$BOOST_ROOT" -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.logicalcpu) steemd cli_wallet
```

See [doc/building.md](doc/building.md) for detailed build instructions.

## Configuration

### Generate Default Config

```bash
# Run steemd once to generate config
./programs/steemd/steemd

# Config will be created at:
# witness_node_data_dir/config.ini
```

### Example Configurations

Located in [`contrib/`](contrib/):

- [`docker.config.ini`](contrib/docker.config.ini) - Consensus node (witnesses/seed nodes)
- [`fullnode.config.ini`](contrib/fullnode.config.ini) - Full API node
- [`ahnode.config.ini`](contrib/ahnode.config.ini) - Account history node
- [`broadcaster.config.ini`](contrib/broadcaster.config.ini) - Broadcast node
- [`testnet.config.ini`](contrib/testnet.config.ini) - Private testnet

### Seed Nodes

Seed node list: [doc/seednodes.txt](doc/seednodes.txt)

Override via environment variable:
```bash
STEEMD_SEED_NODES="seed1.example.com:2001 seed2.example.com:2001"
```

## Docker Environment Variables

| Variable | Description |
|----------|-------------|
| `USE_WAY_TOO_MUCH_RAM` | Enable full node with all data |
| `USE_FULL_WEB_NODE` | Enable full API set with default config |
| `USE_NGINX_FRONTEND` | Enable NGINX reverse proxy with `/health` endpoint |
| `USE_MULTICORE_READONLY` | Enable multi-reader mode (experimental, 4 readers per core) |
| `HOME` | Data directory path (default: `/var/lib/steemd`) |
| `USE_PAAS` | PaaS mode for AWS Elastic Beanstalk |
| `S3_BUCKET` | S3 bucket for shared memory files (PaaS mode) |
| `SYNC_TO_S3` | Generate and upload shared memory files to S3 |
| `STEEMD_SEED_NODES` | Override default seed nodes (whitespace-delimited) |

See [README.md environment variables section](#environment-variables) for more details.

## CLI Wallet

The command-line wallet connects to `steemd` via WebSocket.

### Requirements

Node must have:
- `account_by_key_api` plugin enabled
- `condenser_api` plugin enabled
- WebSocket endpoint configured: `webserver-ws-endpoint`

### Usage

```bash
# Start cli_wallet
./programs/cli_wallet/cli_wallet -s ws://127.0.0.1:8090

# Wallet is self-documented
>>> help
>>> gethelp <command>
```

## System Requirements

### Full Web Node

- **Disk**: 110GB+ available (constantly growing)
- **RAM**: 16GB+ minimum
- **Storage**: SSD strongly recommended
- **CPU**: Any CPU with decent single-core performance

### Seed Node (P2P)

- **Disk**: ~30GB (block log + state)
- **RAM**: 4GB minimum
- **Storage**: HDD acceptable

### Memory-Mapped Files

- **Shared memory**: 56GB data, up to 80GB configured
- **Block log**: 27GB+ (as of August 2017, constantly growing)
- **Performance tip**: Use `--shared-file-dir=/path` to place on ramdisk or fast SSD

### Linux VM Tuning

For initial sync and replays only:

```bash
echo    75 | sudo tee /proc/sys/vm/dirty_background_ratio
echo  1000 | sudo tee /proc/sys/vm/dirty_expire_centisecs
echo    80 | sudo tee /proc/sys/vm/dirty_ratio
echo 30000 | sudo tee /proc/sys/vm/dirty_writeback_centisecs
```

## Repository Structure

```
steem/
├── libraries/              # Core libraries
│   ├── chain/             # Blockchain logic and consensus
│   ├── protocol/          # Protocol definitions and operations
│   ├── plugins/           # Plugin implementations
│   ├── fc/                # Foundational C++ utilities
│   ├── chainbase/         # Memory-mapped database
│   └── appbase/           # Application plugin framework
├── programs/              # Executable programs
│   ├── steemd/            # Main blockchain node daemon
│   ├── cli_wallet/        # Command-line wallet
│   └── util/              # Utility programs
├── tests/                 # Test suite
├── contrib/               # Configuration examples and scripts
├── docs/                   # Documentation
└── example_plugins/       # Example plugin implementations
```

## Development

### Building Tests

```bash
cd build
cmake -DBUILD_STEEM_TESTNET=ON -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc) chain_test plugin_test
```

### Running Tests

```bash
# All tests
./tests/chain_test

# Specific test suite
./tests/chain_test --run_test=basic_tests
./tests/chain_test --run_test=operation_tests

# Plugin tests
./tests/plugin_test
```

See [tests/README.md](tests/README.md) and [doc/testing.md](doc/testing.md) for more.

### CMake Build Options

| Option | Values | Description |
|--------|--------|-------------|
| `CMAKE_BUILD_TYPE` | Release/Debug | Build type (use Release unless debugging) |
| `BUILD_STEEM_TESTNET` | ON/OFF | Build for private testnet (required for tests) |
| `LOW_MEMORY_NODE` | ON/OFF | Consensus-only node (recommended for witnesses) |
| `CLEAR_VOTES` | ON/OFF | Clear old votes from memory |
| `SKIP_BY_TX_ID` | ON/OFF | Disable tx-by-id queries (~65% faster reindex) |
| `ENABLE_COVERAGE_TESTING` | ON/OFF | Enable code coverage analysis |
| `ENABLE_SMT_SUPPORT` | ON/OFF | Enable Smart Media Token support |

### Plugin Development

```bash
# Generate new plugin skeleton
./programs/util/newplugin.py
```

See [doc/plugin.md](doc/plugin.md) for plugin development guide.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on:
- Bug reports
- Feature suggestions
- Pull requests
- Code standards

## Community & Support

- **GitHub Issues**: Bug reports and implementation discussion only
- **Bitcointalk**: [Original announcement thread](https://bitcointalk.org/index.php?topic=1410943.new)

## License

This software is provided "as is", without warranty of any kind. See [LICENSE.md](LICENSE.md) for full terms.

```
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
```

## Additional Resources

- [Steem Whitepaper](https://steem.io/SteemWhitePaper.pdf)
- [Exchange Quickstart](doc/exchangequickstart.md)
- [Building Guide](doc/building.md)
- [Testing Guide](doc/testing.md)
- [Plugin Development](doc/plugin.md)
- [API Documentation](doc/api-notes.md)
- [Git Guidelines](doc/git-guildelines.md)
- [Python Debug Node](doc/Python-Debug-Node.md)
- [Seed Nodes](doc/seednodes.txt)
