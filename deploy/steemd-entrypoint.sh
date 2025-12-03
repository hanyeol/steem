#!/bin/bash

echo /tmp/core | tee /proc/sys/kernel/core_pattern
ulimit -c unlimited

# if we're not using PaaS mode then start steemd traditionally with sv to control it
if [[ ! "$USE_PAAS" ]]; then
  mkdir -p /etc/service/steemd
  cp /etc/steemd/runit/steemd.run /etc/service/steemd/run
  chmod +x /etc/service/steemd/run
  runsv /etc/service/steemd
elif [[ "$IS_TESTNET" ]]; then
  /usr/local/bin/steemd-test-bootstrap.sh
else
  /usr/local/bin/steemd-paas-bootstrap.sh
fi
