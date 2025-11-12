# Quickstart

## Get Current steemd
Use docker:
```
docker run \
    -d -p 2001:2001 -p 8090:8090 --name steemd-default \
    --restart unless-stopped hanyeol/steem
```

### Low Memory Node

Above runs low memory node, which is suitable for:

- Seed nodes
- Witness nodes
- Exchanges

For full api node use:

```
docker run \
    --env USE_WAY_TOO_MUCH_RAM=1 --env USE_FULL_WEB_NODE=1 \
    -d -p 2001:2001 -p 8090:8090 --name steemd-full \
    --restart unless-stopped \
    hanyeol/steem
```

## Configure for Your Use Case

### Full API Node

You need to use `USE_WAY_TOO_MUCH_RAM=1` and `USE_FULL_WEB_NODE=1` as stated above. You can use `contrib/fullnode.config.ini` as a base for your `config.ini` file.

### Exchanges

Use low memory node. Also make sure that your `config.ini` contains:
```
enable-plugin = account_history
public-api = database_api login_api
track-account-range = ["yourexchangeid", "yourexchangeid"]
```

**Note:** Do not add other APIs or plugins unless you know what you are doing.

This configuration exists in Docker with the following command:

```bash
docker run -d --env TRACK_ACCOUNT="yourexchangeid" \
    --name steemd \
    --restart unless-stopped \
    hanyeol/steem
```

## Resources Usage

Please make sure that you have enough resources available. Check `shared-file-size =` in your `config.ini` to reflect your needs. Set it to at least 25% more than current size.

**Note:** Provided values are expected to grow significantly over time. Blockchain data takes over **16GB** of storage space.

### Full Node

Shared memory file for full node uses over **65GB**.

### Exchange Node

Shared memory file for exchange node uses over **16GB** (tracked history for single account).

### Seed Node

Shared memory file for seed node uses over **5.5GB**.

### Other Use Cases

Shared memory file size varies depending on your specific configuration, but it is expected to be somewhere between "seed node" and "full node" usage.
