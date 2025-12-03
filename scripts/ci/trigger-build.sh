#!/bin/bash
set -e
/bin/bash $WORKSPACE/scripts/ci/build-pending.sh
if /bin/bash $WORKSPACE/scripts/ci/build-release.sh; then
  echo BUILD SUCCESS
else
  echo BUILD FAILURE
  exit 1
fi
