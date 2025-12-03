#!/bin/bash

VERSION=`cat /etc/steemd-version`

# clean out data dir since it may contain stale data from previous runs
rm -rf $HOME/*

if [[ "$USE_HIGH_MEMORY" ]]; then
  STEEMD="/usr/local/steemd-high/bin/steemd"
else
  STEEMD="/usr/local/steemd-low/bin/steemd"
fi

# copy config from image (data dir is cleaned on every PaaS restart)
if [[ "$STEEMD_NODE_MODE" == "fullnode" ]]; then
  cp /etc/steemd/fullnode.config.ini $HOME/config.ini
elif [[ "$STEEMD_NODE_MODE" == "broadcast" ]]; then
  cp /etc/steemd/broadcast.config.ini $HOME/config.ini
elif [[ "$STEEMD_NODE_MODE" == "ahnode" ]]; then
  cp /etc/steemd/ahnode.config.ini $HOME/config.ini
elif [[ "$STEEMD_NODE_MODE" == "witness" ]]; then
  cp /etc/steemd/witness.config.ini $HOME/config.ini
elif [[ -f "/etc/steemd/config.ini" ]]; then
  cp /etc/steemd/config.ini $HOME/config.ini
else
  cp /etc/steemd/fullnode.config.ini $HOME/config.ini
fi

chown -R steemd:steemd $HOME

ARGS=""

# if user did pass in desired seed nodes, use the ones the user specified
if [[ ! -z "$STEEMD_SEED_NODES" ]]; then
    for NODE in $STEEMD_SEED_NODES ; do
        ARGS+=" --p2p-seed-node=$NODE"
    done
fi

NOW=`date +%s`

STEEMD_FEED_START_TIME=`expr $NOW - 1209600`
ARGS+=" --follow-start-feeds=$STEEMD_FEED_START_TIME"

STEEMD_PROMOTED_START_TIME=`expr $NOW - 604800`
ARGS+=" --tags-start-promoted=$STEEMD_PROMOTED_START_TIME"

if [[ ! "$DISABLE_BLOCK_API" ]]; then
   ARGS+=" --plugin=block_api"
fi

cd $HOME

mv /etc/nginx/nginx.conf /etc/nginx/nginx.original.conf
cp /etc/nginx/steemd.nginx.conf /etc/nginx/nginx.conf

# get blockchain state from an S3 bucket
echo steemd: beginning download and decompress of s3://$S3_BUCKET/blockchain-$VERSION-latest.tar.lz4
finished=0
count=1
if [[ "$USE_RAMDISK" ]]; then
  mkdir -p /mnt/ramdisk
  mount -t ramfs -o size=${RAMDISK_SIZE_IN_MB:-51200}m ramfs /mnt/ramdisk
  ARGS+=" --shared-file-dir=/mnt/ramdisk/blockchain"
  # try five times to pull in shared memory file
  while [[ $count -le 5 ]] && [[ $finished == 0 ]]
  do
    rm -rf $HOME/blockchain/*
    rm -rf /mnt/ramdisk/blockchain/*
    if [[ "$STEEMD_NODE_MODE" == "broadcast" ]]; then
      aws s3 cp s3://$S3_BUCKET/broadcast-$VERSION-latest.tar.lz4 - | lz4 -d | tar x --wildcards 'blockchain/block*' -C /mnt/ramdisk 'blockchain/shared*'
    elif [[ "$STEEMD_NODE_MODE" == "ahnode" ]]; then
      aws s3 cp s3://$S3_BUCKET/ahnode-$VERSION-latest.tar.lz4 - | lz4 -d | tar x --wildcards 'blockchain/block*' 'blockchain/*rocksdb-storage*' -C /mnt/ramdisk 'blockchain/shared*'
    else
      aws s3 cp s3://$S3_BUCKET/blockchain-$VERSION-latest.tar.lz4 - | lz4 -d | tar x --wildcards 'blockchain/block*' -C /mnt/ramdisk 'blockchain/shared*'
    fi
    if [[ $? -ne 0 ]]; then
      sleep 1
      echo notifyalert steemd: unable to pull blockchain state from S3 - attempt $count
      (( count++ ))
    else
      finished=1
    fi
  done
  chown -R steemd:steemd /mnt/ramdisk/blockchain
else
  while [[ $count -le 5 ]] && [[ $finished == 0 ]]
  do
    rm -rf $HOME/blockchain/*
    if [[ "$STEEMD_NODE_MODE" == "broadcast" ]]; then
      aws s3 cp s3://$S3_BUCKET/broadcast-$VERSION-latest.tar.lz4 - | lz4 -d | tar x
    elif [[ "$STEEMD_NODE_MODE" == "ahnode" ]]; then
      aws s3 cp s3://$S3_BUCKET/ahnode-$VERSION-latest.tar.lz4 - | lz4 -d | tar x
    else
      aws s3 cp s3://$S3_BUCKET/blockchain-$VERSION-latest.tar.lz4 - | lz4 -d | tar x
    fi
    if [[ $? -ne 0 ]]; then
      sleep 1
      echo notifyalert steemd: unable to pull blockchain state from S3 - attempt $count
      (( count++ ))
    else
      finished=1
    fi
  done
fi
if [[ $finished == 0 ]]; then
  if [[ ! "$SYNC_TO_S3" ]]; then
    echo notifyalert steemd: unable to pull blockchain state from S3 - exiting
    exit 1
  else
    echo notifysteemdsync steemdsync: shared memory file for $VERSION not found, creating a new one by replaying the blockchain
    if [[ "$USE_RAMDISK" ]]; then
      mkdir -p /mnt/ramdisk/blockchain
      chown -R steemd:steemd /mnt/ramdisk/blockchain
    else
      mkdir blockchain
    fi
    aws s3 cp s3://$S3_BUCKET/block_log-latest blockchain/block_log
    if [[ $? -ne 0 ]]; then
      echo notifysteemdsync steemdsync: unable to pull latest block_log from S3, will sync from scratch.
    else
      ARGS+=" --replay-blockchain --force-validate"
    fi
    touch /tmp/isnewsync
  fi
fi

# for appbase tags plugin loading
ARGS+=" --tags-skip-startup-update"

cd $HOME

if [[ "$SYNC_TO_S3" ]]; then
  touch /tmp/issyncnode
  chown www-data:www-data /tmp/issyncnode
fi

chown -R steemd:steemd $HOME/*

# let's get going
cp /etc/nginx/steemd-proxy.conf.template /etc/nginx/steemd-proxy.conf
echo server 127.0.0.1:8091\; >> /etc/nginx/steemd-proxy.conf
echo } >> /etc/nginx/steemd-proxy.conf
rm /etc/nginx/sites-enabled/default
cp /etc/nginx/steemd-proxy.conf /etc/nginx/sites-enabled/default
/etc/init.d/fcgiwrap restart
service nginx restart
exec chpst -usteemd \
    $STEEMD \
        --webserver-ws-endpoint=127.0.0.1:8091 \
        --webserver-http-endpoint=127.0.0.1:8091 \
        --p2p-endpoint=0.0.0.0:2001 \
        --data-dir=$HOME \
        $ARGS \
        $STEEMD_EXTRA_OPTS \
        2>&1&
SAVED_PID=`pgrep -f p2p-endpoint`
echo $SAVED_PID > /var/run/steemd.pid
mkdir -p /etc/service/steemd
if [[ ! "$SYNC_TO_S3" ]]; then
  cp /etc/steemd/runit/steemd-paas-monitor.run /etc/service/steemd/run
else
  cp /etc/steemd/runit/steemd-snapshot-uploader.run /etc/service/steemd/run
fi
chmod +x /etc/service/steemd/run
runsv /etc/service/steemd
