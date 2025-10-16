# Steem Blockchain System Architecture

This document describes the minimum system configuration required to operate a Steem blockchain node.

## Table of Contents

1. [System Overview](#system-overview)
2. [Node Types](#node-types)
3. [Minimum Hardware Requirements](#minimum-hardware-requirements)
4. [System Architecture Diagrams](#system-architecture-diagrams)
5. [Network Configuration](#network-configuration)
6. [Storage Layout](#storage-layout)
7. [Deployment Scenarios](#deployment-scenarios)

## System Overview

The Steem blockchain requires different system configurations depending on the node type and use case. The system consists of the core blockchain daemon (`steemd`), optional wallet interface (`cli_wallet`), and various plugins for extended functionality.

```mermaid
graph TB
    subgraph "Core Components"
        A[steemd Daemon]
        B[Chainbase Database]
        C[Block Log]
    end

    subgraph "Optional Components"
        D[cli_wallet]
        E[Web API Server]
        F[NGINX Reverse Proxy]
    end

    subgraph "External Services"
        G[P2P Network]
        H[RPC Clients]
        I[External Applications]
    end

    A --> B
    A --> C
    A <--> G
    D --> A
    E --> A
    F --> E
    H --> E
    I --> E
    I --> F
```

## Node Types

### 1. Witness Node (Block Producer)

Minimal configuration for producing blocks.

**Purpose:** Produce blocks, validate transactions, participate in consensus

**Required Components:**
- `steemd` with `witness` plugin
- Low memory mode recommended

**System Requirements:**
- **CPU:** 2-4 cores (3+ GHz recommended)
- **RAM:** 4-8 GB minimum
- **Storage:** 30-50 GB SSD
- **Network:** 100 Mbps+ stable connection, low latency

```mermaid
graph LR
    subgraph "Witness Node"
        A[steemd<br/>witness plugin]
        B[Chainbase<br/>24GB]
        C[Block Log<br/>27GB+]
    end

    D[P2P Network] <--> A
    A --> B
    A --> C
```

### 2. Seed Node (P2P Node)

Provides P2P connectivity for network health.

**Purpose:** Relay blocks and transactions, maintain network connectivity

**Required Components:**
- `steemd` with `p2p` plugin
- Minimal plugins for reduced resource usage

**System Requirements:**
- **CPU:** 2 cores
- **RAM:** 4 GB minimum (24GB for state file)
- **Storage:** 30-50 GB SSD
- **Network:** 1 Gbps+ recommended, high bandwidth

```mermaid
graph TB
    subgraph "Seed Node"
        A[steemd<br/>p2p plugin]
        B[Chainbase]
        C[Block Log]
    end

    D[Peer 1] <--> A
    E[Peer 2] <--> A
    F[Peer 3] <--> A
    G[Peer N...] <--> A

    A --> B
    A --> C
```

### 3. API Node (Full Node)

Provides comprehensive API access for applications.

**Purpose:** Serve API requests, provide blockchain data to applications

**Required Components:**
- `steemd` with API plugins (`database_api`, `condenser_api`, `account_history_api`, `follow_api`, etc.)
- `webserver` plugin for HTTP/WebSocket endpoints
- Optional: NGINX reverse proxy for load balancing

**System Requirements:**
- **CPU:** 4-8 cores
- **RAM:** 16-32 GB minimum
- **Storage:** 110+ GB SSD (recommended NVMe)
- **Network:** 1 Gbps+ recommended

```mermaid
graph TB
    subgraph "API Node"
        A[steemd<br/>Full API plugins]
        B[Chainbase<br/>56GB+]
        C[Block Log<br/>27GB+]
        D[Account History DB<br/>RocksDB/RAM]
        E[Follow DB]
        F[Market History DB]
    end

    subgraph "External Access"
        G[NGINX<br/>Reverse Proxy]
        H[Client Apps]
        I[Exchanges]
        J[Block Explorers]
    end

    A --> B
    A --> C
    A --> D
    A --> E
    A --> F
    G --> A
    H --> G
    I --> G
    J --> G
```

### 4. Exchange Node

Specialized node for cryptocurrency exchanges.

**Purpose:** Monitor deposits/withdrawals, track account balances

**Required Components:**
- `steemd` with `account_by_key_api`, `condenser_api`, `database_api`
- `account_history_rocksdb` plugin for efficient account history queries
- `cli_wallet` for transaction signing

**System Requirements:**
- **CPU:** 4-8 cores
- **RAM:** 16-32 GB
- **Storage:** 110+ GB SSD + RocksDB storage
- **Network:** 100 Mbps+ stable connection

```mermaid
graph LR
    subgraph "Exchange Infrastructure"
        A[steemd<br/>API Node]
        B[cli_wallet]
        C[RocksDB<br/>Account History]
    end

    subgraph "Exchange Backend"
        D[Deposit Monitor]
        E[Withdrawal Service]
        F[Balance Tracker]
    end

    A --> C
    B --> A
    D --> A
    E --> B
    F --> A
```

## Minimum Hardware Requirements

### Summary Table

| Node Type | CPU | RAM | Storage | Network | Monthly Cost Estimate |
|-----------|-----|-----|---------|---------|----------------------|
| Witness Node | 2-4 cores | 4-8 GB | 50 GB SSD | 100 Mbps | $20-50 |
| Seed Node | 2 cores | 4 GB | 50 GB SSD | 1 Gbps | $40-80 |
| API Node | 4-8 cores | 16-32 GB | 110+ GB NVMe | 1 Gbps | $80-200 |
| Exchange Node | 4-8 cores | 16-32 GB | 150+ GB SSD | 100 Mbps | $80-200 |

### Detailed Requirements

#### CPU
- **Minimum:** x86_64 architecture, 2 cores @ 2.5+ GHz
- **Recommended:** 4+ cores @ 3+ GHz
- **Optimal:** 8+ cores for API nodes with high traffic
- AVX2 instruction set support recommended for cryptographic operations

#### RAM
- **Witness/Seed Node:** 4 GB minimum, 8 GB recommended
- **API Node:** 16 GB minimum, 32+ GB recommended
- **Note:** Memory-mapped state file (up to 80GB configured) uses virtual memory

#### Storage
- **Type:** SSD required (NVMe recommended for API nodes)
- **Minimum Space:**
  - Witness/Seed: 50 GB (30 GB with pruning)
  - API Node: 110 GB minimum, 150+ GB recommended
- **IOPS:** 3,000+ IOPS for sync/replay operations
- **Components:**
  - Shared memory file: 56 GB (up to 80 GB configured)
  - Block log: 27+ GB (growing continuously)
  - RocksDB (if enabled): 20-50+ GB

#### Network
- **Bandwidth:**
  - Witness: 100 Mbps minimum
  - Seed: 1 Gbps+ recommended
  - API: 1 Gbps+ recommended
- **Latency:** <50ms to major peers (critical for witnesses)
- **Ports:**
  - P2P: 2001 (default, configurable)
  - WebSocket: 8090 (default, configurable)
  - HTTP RPC: 8091 (default, configurable)

## System Architecture Diagrams

### Complete System Data Flow

```mermaid
flowchart TB
    subgraph "Network Layer"
        P2P[P2P Network<br/>Port 2001]
    end

    subgraph "Application Layer"
        WS[WebSocket API<br/>Port 8090]
        HTTP[HTTP RPC API<br/>Port 8091]
        NGINX[NGINX Reverse Proxy<br/>Port 80/443]
    end

    subgraph "Core Process: steemd"
        CHAIN[Chain Plugin]
        P2PP[P2P Plugin]
        WEB[Webserver Plugin]
        WITNESS[Witness Plugin]
        API[API Plugins]
    end

    subgraph "Storage Layer"
        CB[(Chainbase<br/>Memory-mapped DB<br/>56-80 GB)]
        BL[(Block Log<br/>Append-only<br/>27+ GB)]
        ROCKS[(RocksDB<br/>Account History<br/>20-50 GB)]
    end

    subgraph "Clients"
        WALLET[cli_wallet]
        APP[Applications]
        EX[Exchanges]
    end

    P2P <--> P2PP
    P2PP --> CHAIN

    WALLET --> WS
    APP --> HTTP
    EX --> NGINX

    NGINX --> WEB
    WS --> WEB
    HTTP --> WEB
    WEB --> API

    WITNESS --> CHAIN
    API --> CHAIN

    CHAIN --> CB
    CHAIN --> BL
    API --> ROCKS
```

### Plugin Architecture

```mermaid
flowchart TD
    START[Plugin System]

    START --> CORE_LAYER[Core Layer]

    CORE_LAYER --> P2P[p2p<br/>Network Communication]
    CORE_LAYER --> CHAIN[chain<br/>Blockchain State]
    CORE_LAYER --> WEB[webserver<br/>API Server]

    CHAIN --> STATE_LAYER[State Tracking Layer]

    STATE_LAYER --> AH[account_history<br/>Track account operations]
    STATE_LAYER --> AHDB[account_history_rocksdb<br/>RocksDB-based history]
    STATE_LAYER --> FOLLOW[follow<br/>Social graph tracking]
    STATE_LAYER --> MARKET[market_history<br/>Market data]
    STATE_LAYER --> TAGS[tags<br/>Content categorization]

    STATE_LAYER --> API_LAYER[API Layer]
    WEB --> API_LAYER

    API_LAYER --> DBA[database_api<br/>Core blockchain queries]
    API_LAYER --> CA[condenser_api<br/>Legacy API compatibility]
    API_LAYER --> BA[block_api<br/>Block data access]

    AH --> AHA[account_history_api]
    AHDB --> AHA
    FOLLOW --> FA[follow_api]
    MARKET --> MA[market_history_api]
    TAGS --> TA[tags_api]

    API_LAYER --> AHA
    API_LAYER --> FA
    API_LAYER --> MA
    API_LAYER --> TA
    API_LAYER --> ABK[account_by_key_api]

    CHAIN --> SPECIAL_LAYER[Specialized Plugins]

    SPECIAL_LAYER --> WIT[witness<br/>Block production]
    SPECIAL_LAYER --> RC[rc<br/>Resource credits]
    SPECIAL_LAYER --> DBGNODE[debug_node<br/>Development tools]
```

### Memory Layout

```mermaid
graph LR
    subgraph "Physical RAM"
        APP[Application<br/>steemd Process<br/>2-4 GB]
        CACHE[OS Cache<br/>File Cache<br/>Variable]
    end

    subgraph "Virtual Memory / Disk"
        SHARED[Shared Memory File<br/>Memory-mapped<br/>56-80 GB]
        BLOCKLOG[Block Log<br/>Append-only<br/>27+ GB]
        ROCKS[RocksDB<br/>20-50 GB]
    end

    APP <-.Mmap.-> SHARED
    CACHE <-.Cache.-> SHARED
    CACHE <-.Cache.-> BLOCKLOG
    APP --> ROCKS
```

## Network Configuration

### Port Configuration

```mermaid
graph LR
    subgraph "External Network"
        PEER1[Peer Nodes]
        CLIENT[API Clients]
    end

    subgraph "Firewall"
        FW{Firewall Rules}
    end

    subgraph "steemd Process"
        P2P[P2P Service<br/>:2001]
        WS[WebSocket<br/>:8090]
        HTTP[HTTP RPC<br/>:8091]
    end

    PEER1 <-->|TCP 2001| FW
    CLIENT -->|TCP 8090/8091| FW

    FW <--> P2P
    FW --> WS
    FW --> HTTP
```

### Firewall Rules (iptables example)

```bash
# P2P port (required for all nodes)
iptables -A INPUT -p tcp --dport 2001 -j ACCEPT

# WebSocket API (for API nodes)
iptables -A INPUT -p tcp --dport 8090 -j ACCEPT

# HTTP RPC API (for API nodes)
iptables -A INPUT -p tcp --dport 8091 -j ACCEPT

# NGINX reverse proxy (optional)
iptables -A INPUT -p tcp --dport 80 -j ACCEPT
iptables -A INPUT -p tcp --dport 443 -j ACCEPT
```

## Storage Layout

### Recommended Directory Structure

```
/var/lib/steemd/
├── blockchain/
│   ├── block_log              # Immutable blockchain data (27+ GB)
│   └── block_log.index        # Block index for fast lookup
├── shared_mem.bin              # Memory-mapped state (56-80 GB)
├── shared_mem.meta             # Metadata for shared memory
├── account_history.rocksdb/    # RocksDB account history (optional, 20-50 GB)
└── config.ini                  # Node configuration
```

### Storage Optimization

```mermaid
graph TB
    subgraph "Storage Tiers"
        RAMDISK[RAM Disk<br/>tmpfs/ramfs<br/>Fastest<br/>For shared_mem.bin]
        NVME[NVMe SSD<br/>Very Fast<br/>For RocksDB + block_log]
        SSD[SATA SSD<br/>Fast<br/>For block_log]
        HDD[HDD<br/>Slow<br/>Not recommended]
    end

    subgraph "Performance Impact"
        SYNC[Initial Sync<br/>IOPS Critical]
        REPLAY[Chain Replay<br/>IOPS Critical]
        NORMAL[Normal Operation<br/>Moderate IOPS]
    end

    RAMDISK -.Best.-> SYNC
    RAMDISK -.Best.-> REPLAY
    RAMDISK -.Best.-> NORMAL

    NVME -.Good.-> SYNC
    NVME -.Good.-> REPLAY
    NVME -.Good.-> NORMAL

    SSD -.Acceptable.-> SYNC
    SSD -.Acceptable.-> REPLAY
    SSD -.Good.-> NORMAL

    HDD -.Poor.-> SYNC
    HDD -.Poor.-> REPLAY
    HDD -.Acceptable.-> NORMAL
```

### Linux VM Settings for Optimal Performance

For initial sync and chain replay operations:

```bash
# Increase dirty background ratio
echo 75 | sudo tee /proc/sys/vm/dirty_background_ratio

# Decrease dirty expire time
echo 1000 | sudo tee /proc/sys/vm/dirty_expire_centisecs

# Increase dirty ratio
echo 80 | sudo tee /proc/sys/vm/dirty_ratio

# Increase dirty writeback time
echo 30000 | sudo tee /proc/sys/vm/dirty_writeback_centisecs
```

## Deployment Scenarios

### Scenario 1: Single Witness Node

Minimal setup for block production.

```mermaid
graph TB
    VPS[VPS/Cloud Instance<br/>4 GB RAM, 2 cores<br/>50 GB SSD]

    subgraph "steemd Configuration"
        CONFIG["config.ini:<br/>plugin = witness<br/>plugin = chain<br/>plugin = p2p<br/>witness = 'your-witness-name'<br/>private-key = WIF_PRIVATE_KEY"]
    end

    VPS --> CONFIG
```

**Estimated Cost:** $20-50/month

### Scenario 2: API Node with Load Balancer

Production setup for serving applications.

```mermaid
graph TB
    subgraph "Load Balancer"
        LB[NGINX Load Balancer<br/>SSL Termination<br/>Rate Limiting]
    end

    subgraph "API Nodes (2+)"
        API1[steemd Node 1<br/>16GB RAM, 4 cores<br/>150 GB NVMe]
        API2[steemd Node 2<br/>16GB RAM, 4 cores<br/>150 GB NVMe]
    end

    subgraph "Monitoring"
        MON[Prometheus + Grafana<br/>Health Checks]
    end

    CLIENT[Clients] --> LB
    LB --> API1
    LB --> API2

    MON --> API1
    MON --> API2
    MON --> LB
```

**Estimated Cost:** $200-500/month

### Scenario 3: Exchange Infrastructure

Dedicated setup for cryptocurrency exchanges.

```mermaid
graph TB
    subgraph "DMZ"
        LB[Load Balancer<br/>DDoS Protection]
    end

    subgraph "Application Tier"
        DEPOSIT[Deposit Monitor]
        WITHDRAW[Withdrawal Service]
        WALLET[Hot Wallet<br/>cli_wallet]
    end

    subgraph "Blockchain Tier"
        API1[steemd API Node 1<br/>32GB RAM, 8 cores<br/>200 GB NVMe<br/>RocksDB enabled]
        API2[steemd API Node 2<br/>Redundant]
    end

    subgraph "Cold Storage"
        COLD[Cold Wallet<br/>Offline Keys]
    end

    LB --> DEPOSIT
    LB --> WITHDRAW

    DEPOSIT --> API1
    DEPOSIT --> API2
    WITHDRAW --> WALLET
    WALLET --> API1

    COLD -.Manual Transfer.-> WALLET
```

**Estimated Cost:** $300-800/month (excluding security infrastructure)

### Scenario 4: Development/Testing Environment

Local setup for development and testing.

```mermaid
graph LR
    subgraph "Docker Environment"
        DEV[steemd Container<br/>BUILD_STEEM_TESTNET=ON<br/>8 GB RAM<br/>50 GB volume]
        TEST[Test Container<br/>chain_test<br/>plugin_test]
    end

    subgraph "Development Machine"
        IDE[IDE/Editor]
        CLI[Terminal]
    end

    IDE --> DEV
    CLI --> DEV
    CLI --> TEST
    DEV -.Shared Volume.-> TEST
```

**Requirements:** Local machine or VM with 8+ GB RAM, Docker installed

## Configuration Examples

### Witness Node (config.ini)

```ini
# Data directory
data-dir = /var/lib/steemd

# P2P endpoint
p2p-endpoint = 0.0.0.0:2001

# Enable required plugins
plugin = chain p2p witness

# Witness configuration
witness = "your-witness-name"
private-key = 5JYourPrivateKeyInWIFFormat...

# Seed nodes
seed-node = seed-node1.example.com:2001
seed-node = seed-node2.example.com:2001

# Resource optimization
shared-file-size = 24G
shared-file-dir = /var/lib/steemd

# Enable low memory mode
plugin = witness
```

### API Node (config.ini)

```ini
# Data directory
data-dir = /var/lib/steemd

# P2P endpoint
p2p-endpoint = 0.0.0.0:2001

# API endpoints
webserver-http-endpoint = 0.0.0.0:8091
webserver-ws-endpoint = 0.0.0.0:8090

# Enable plugins
plugin = chain p2p webserver
plugin = database_api condenser_api block_api
plugin = account_history_rocksdb account_history_api
plugin = follow follow_api
plugin = market_history market_history_api
plugin = account_by_key_api

# Seed nodes
seed-node = seed-node1.example.com:2001
seed-node = seed-node2.example.com:2001

# Resource configuration
shared-file-size = 80G
shared-file-dir = /var/lib/steemd

# RocksDB configuration
account-history-rocksdb-path = /var/lib/steemd/account_history.rocksdb

# API limits
webserver-thread-pool-size = 32
```

## Monitoring and Health Checks

### Key Metrics to Monitor

```mermaid
graph TB
    subgraph "System Metrics"
        CPU[CPU Usage<br/>Target: <80%]
        MEM[Memory Usage<br/>RSS + Cache]
        DISK[Disk I/O<br/>IOPS, Throughput]
        NET[Network<br/>Bandwidth, Latency]
    end

    subgraph "Blockchain Metrics"
        SYNC[Sync Status<br/>Current Block Height]
        PEERS[P2P Peers<br/>Active Connections]
        TPS[Transactions/sec<br/>Processing Rate]
        MISSED[Missed Blocks<br/>Witness only]
    end

    subgraph "API Metrics"
        REQ[Request Rate<br/>API calls/sec]
        LAT[Response Latency<br/>p50, p95, p99]
        ERR[Error Rate<br/>4xx, 5xx responses]
    end

    MON[Monitoring System<br/>Prometheus/Grafana] --> CPU
    MON --> MEM
    MON --> DISK
    MON --> NET
    MON --> SYNC
    MON --> PEERS
    MON --> TPS
    MON --> MISSED
    MON --> REQ
    MON --> LAT
    MON --> ERR
```

### Health Check Endpoints

When using NGINX frontend (`USE_NGINX_FRONTEND=1`), the following endpoints are available:

- `GET /health` - Basic health check (200 if node is responsive)
- `GET /.well-known/healthcheck.json` - Detailed health status

## Backup and Disaster Recovery

### Backup Strategy

```mermaid
graph LR
    subgraph "Primary Node"
        NODE[Running steemd]
        SHARED[shared_mem.bin<br/>State file]
        BLOCKLOG[block_log<br/>Blockchain data]
        CONFIG[config.ini<br/>Configuration]
    end

    subgraph "Backup Storage"
        S3[S3/Cloud Storage<br/>Block log backups<br/>Weekly/Monthly]
        LOCAL[Local Backup<br/>Config files<br/>Daily]
    end

    subgraph "Recovery Node"
        STANDBY[Standby Server<br/>Ready to sync]
    end

    BLOCKLOG -.Weekly.-> S3
    CONFIG -.Daily.-> LOCAL

    S3 -.Restore.-> STANDBY
    LOCAL -.Restore.-> STANDBY

    NODE -.Failover.-> STANDBY
```

### Recovery Options

1. **Fast Recovery:** Use snapshot from S3 + replay recent blocks
2. **Full Recovery:** Replay entire chain from genesis or trusted block log
3. **State File Sharing:** Download pre-synced shared memory file (PaaS mode with `SYNC_TO_S3`)

## Security Considerations

### Network Security

```mermaid
graph TB
    subgraph "Public Internet"
        ATK[Potential Attackers<br/>DDoS, Exploits]
    end

    subgraph "Security Layer"
        FW[Firewall<br/>Port filtering]
        LB[DDoS Protection<br/>Rate limiting]
        IDS[Intrusion Detection]
    end

    subgraph "Application Layer"
        NGINX[NGINX<br/>SSL/TLS]
        API[steemd API<br/>Limited exposure]
    end

    subgraph "Internal Network"
        WITNESS[Witness Node<br/>No public API]
        DB[Database Services<br/>Internal only]
    end

    ATK --> FW
    FW --> LB
    LB --> IDS
    IDS --> NGINX
    NGINX --> API

    WITNESS -.P2P only.-> FW
    DB -.No external access.-> API
```

### Best Practices

1. **Witness Nodes:**
   - Do not expose API endpoints publicly
   - Use firewall to restrict P2P to known peers
   - Keep private keys encrypted and backed up securely
   - Monitor for missed blocks

2. **API Nodes:**
   - Use HTTPS/WSS with valid SSL certificates
   - Implement rate limiting to prevent abuse
   - Enable CORS carefully with whitelist
   - Regular security updates

3. **Key Management:**
   - Store witness private keys encrypted
   - Use separate keys for different purposes (active/posting/owner)
   - Implement key rotation policy
   - Never commit keys to version control

## Conclusion

The minimum system configuration depends on the node type:

- **Witness Node:** 2-4 cores, 4-8 GB RAM, 50 GB SSD, reliable network
- **Seed Node:** 2 cores, 4 GB RAM, 50 GB SSD, high bandwidth
- **API Node:** 4-8 cores, 16-32 GB RAM, 110+ GB NVMe SSD, high bandwidth
- **Exchange Node:** 4-8 cores, 16-32 GB RAM, 150+ GB SSD, RocksDB enabled

Always use SSD storage (NVMe preferred for API nodes) and ensure adequate network connectivity. For production deployments, implement redundancy, monitoring, and disaster recovery procedures.

For detailed build instructions, see [docs/getting-started/building.md](../getting-started/building.md).
For exchange-specific setup, see [docs/getting-started/exchange-quick-start.md](../getting-started/exchange-quick-start.md).
