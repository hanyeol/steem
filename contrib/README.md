# Contrib Directory

This directory contains deployment scripts and tools for running Steem nodes in various environments.

## Configuration Files

Node configuration files have been moved to the [`configs/`](../configs/) directory. See [`configs/README.md`](../configs/README.md) for detailed configuration documentation.

## Deployment Scripts

### Docker & PaaS

#### `steemd-entrypoint.sh`
Docker container entrypoint script

#### `steemd.run`
Runit service script for standard steemd execution
- Direct steemd process execution
- Managed by runit process supervisor

#### `steemd-paas-bootstrap.sh`
PaaS environment bootstrap script
- Download blockchain snapshots from S3
- Configure and launch steemd for AWS Elastic Beanstalk
- Environment variables:
  - `USE_PAAS=true`
  - `S3_BUCKET=<bucket-name>`
  - `SYNC_TO_S3=true` (snapshot upload mode)

#### `steemd-paas-monitor.run`
Runit monitoring script for PaaS environments
- Monitors steemd process health
- Collects core dumps on crash
- Triggers container restart on failure
- Runs every 10 seconds

#### `steemd-snapshot-uploader.run`
Runit script for blockchain snapshot management
- Monitors blockchain synchronization status
- Compresses and uploads snapshots when fully synced
- Handles crash recovery with core dump collection
- Supports S3 and compatible storage backends
- Runs every 60 seconds

### Health Check

#### `steemd-healthcheck.sh`
Node health check script
- Verify blockchain sync status
- Measure time difference from latest block
- Return HTTP 200/503 response

#### `steemd-proxy.conf.template`
NGINX reverse proxy configuration template with health check endpoint

### NGINX

#### `steemd.nginx.conf`
- NGINX reverse proxy configuration for steemd
- WebSocket connection proxy
- Load balancing (for multicore readonly mode)
- Provides `/health` endpoint

### Test

#### `steemd-test-bootstrap.sh`
Private test network bootstrap script
- Initialize fastgen node for transaction generation
- Configure 21 witness accounts
- Setup test environment with genesis block

## Usage Examples

See [`configs/README.md`](../configs/README.md) for configuration examples and [`docs/getting-started/node-types-guide.md`](../docs/getting-started/node-types-guide.md) for comprehensive deployment guides.

## Additional Resources

- [Configuration Files](../configs/) - Node configuration examples
- [Node Types Guide](../docs/getting-started/node-types-guide.md) - Detailed guide to different node types
- [Quick Start Guide](../docs/getting-started/quick-start.md) - Docker quick start
- [Deployment Documentation](../docs/operations/) - Production deployment guides
