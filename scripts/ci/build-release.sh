#!/bin/bash
set -e
export IMAGE_NAME="hanyeol/steem:$BRANCH_NAME"
if [[ $IMAGE_NAME == "hanyeol/steem:stable" ]] ; then
  IMAGE_NAME="hanyeol/steem:latest"
fi
sudo docker build --build-arg BUILD_STEP=2 -t=$IMAGE_NAME .
sudo docker login --username=$DOCKER_USER --password=$DOCKER_PASS
sudo docker push $IMAGE_NAME
# make docker cleanup after itself and delete all exited containers
sudo docker rm -v $(docker ps -a -q -f status=exited) || true