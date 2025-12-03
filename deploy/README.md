# Deploy Directory

This directory contains deployment scripts and configuration files for running Steem nodes in various environments.

## Quick Reference

### Bootstrap Scripts (Direct Execution)

- **[steemd-entrypoint.sh](steemd-entrypoint.sh)** - Docker container entrypoint script
- **[steemd-paas-bootstrap.sh](steemd-paas-bootstrap.sh)** - PaaS environment (AWS Elastic Beanstalk) bootstrap
- **[steemd-test-bootstrap.sh](steemd-test-bootstrap.sh)** - Private test network initialization
- **[steemd-healthcheck.sh](steemd-healthcheck.sh)** - Node health check script

### Service Management

- **[runit/](runit/)** - Runit process supervisor scripts
- **[nginx/](nginx/)** - NGINX reverse proxy configurations

## Docker Deployment

### steemd-entrypoint.sh

Docker container entrypoint script that configures and starts steemd based on environment variables.

**Environment Variables:**
- `USE_HIGH_MEMORY` - Enable high-memory mode with in-memory account history (uses steemd-high binary)
- `STEEMD_NODE_MODE` - Node type: `fullnode`, `broadcast`, `ahnode`, or `witness` (sets config file)
- `USE_NGINX_PROXY` - Enable NGINX reverse proxy with `/health` endpoint
- `USE_PUBLIC_SHARED_MEMORY` - Download public shared memory snapshot on startup
- `USE_PUBLIC_BLOCKLOG` - Download public block log on startup
- `HOME` - Data directory path (default: `/var/lib/steemd`)
- `STEEMD_SEED_NODES` - Whitespace-delimited list of P2P seed nodes
- `STEEMD_WITNESS_NAME` - Witness account name (for witness mode)
- `STEEMD_PRIVATE_KEY` - Witness private key (for witness mode)
- `STEEMD_TRACK_ACCOUNT` - Account to track in account history
- `DISABLE_SCALE_MEM` - Disable shared memory scaling (omit for default scaling)
- `STEEMD_EXTRA_OPTS` - Additional command line options

**Example:**
```bash
# Run as full node with NGINX proxy
docker run -e STEEMD_NODE_MODE=fullnode -e USE_NGINX_PROXY=1 steem/steem

# Run high-memory node
docker run -e USE_HIGH_MEMORY=1 -e STEEMD_NODE_MODE=fullnode steem/steem
```

## PaaS Deployment

### steemd-paas-bootstrap.sh

Bootstrap script for PaaS environments (AWS Elastic Beanstalk).

**Features:**
- Download blockchain snapshots from S3
- Configure and launch steemd for cloud environments
- Automatic snapshot management and uploads

**Environment Variables:**
- `USE_PAAS=true` - Enable PaaS mode
- `S3_BUCKET=<bucket-name>` - S3 bucket for snapshots
- `SYNC_TO_S3=true` - Enable snapshot upload mode
- `STEEMD_SEED_NODES` - Whitespace-delimited list of seed nodes

**Example:**
```bash
USE_PAAS=true S3_BUCKET=my-steem-snapshots ./steemd-paas-bootstrap.sh
```

## Test Network

### steemd-test-bootstrap.sh

Initialize a private test network with witness accounts and genesis block.

**Features:**
- Initialize fastgen node for transaction generation
- Configure 21 witness accounts
- Setup test environment with custom genesis

**Example:**
```bash
./steemd-test-bootstrap.sh
```

## Health Monitoring

### steemd-healthcheck.sh

Node health check script for monitoring blockchain sync status.

**Features:**
- Verify blockchain synchronization status
- Measure time difference from latest block
- Return HTTP 200/503 response codes

**Usage:**
```bash
./steemd-healthcheck.sh
# Returns: exit 0 (healthy) or exit 1 (unhealthy)
```

## Service Management

### Runit Scripts

See [runit/README.md](runit/README.md) for process supervisor configuration.

### NGINX Configuration

See [nginx/README.md](nginx/README.md) for reverse proxy setup.

## Additional Resources

- **[Configuration Files](../configs/)** - Node configuration examples
- **[Node Modes Guide](../docs/getting-started/node-modes-guide.md)** - Detailed guide to different node modes
- **[Quick Start Guide](../docs/getting-started/quick-start.md)** - Docker quick start
- **[Deployment Documentation](../docs/operations/)** - Production deployment guides
