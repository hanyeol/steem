#!/bin/bash
set -e
if /bin/bash $WORKSPACE/scripts/ci/build-tests.sh; then
  echo BUILD SUCCESS
else
  echo BUILD FAILURE
  exit 1
fi