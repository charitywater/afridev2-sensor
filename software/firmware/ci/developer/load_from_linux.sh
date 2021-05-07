#!/bin/bash

# ==============================================================================================#=
# FILE:
#     developer/load_from_linux.sh
#
# This script loads the MSP from a linux machine (assuming programmer is connected to device you're on)
#
# Copyright 2020 Twisthink. All Rights Reserved.
# This is unpublished proprietary source code of Twisthink.
# The contents of this file may not be disclosed to third parties, copied, or duplicated
# in any form, in whole or in part, without the prior written permission of Twisthink.
# ==============================================================================================#=

LOAD_IMAGE="registry.gitlab.com/twisthink/charity-water/afd2/afridev_2/ci-image"
BRANCH="develop"

echo "Setting up script shell CDPATH..."
export CDPATH=".:../:../../:../../../"
cd afridev_2

# check if app txt exists
if [ -f application/build/AfridevV2_App_Boot_MSP430.txt ]; then
    echo "Executable found!"
    cp -f application/build/AfridevV2_App_Boot_MSP430.txt /tmp/
else
    echo "Cannot find txt file: <application/build/AfridevV2_App_Boot_MSP430.txt>"
    echo "Suggest building software first using: ./build.sh all"
    exit 1
fi

# check if script exists
if [ -f ci/helpers/load.sh ]; then
    echo ""
    echo "Load script found!"
    cp -f ci/helpers/load.sh /tmp/
else
    echo "Cannot find script file: <ci/helpers/load.sh>"
    exit 1
fi


# ------------------------------------------------------------------------------+-
# Setup Docker CI image
# ------------------------------------------------------------------------------+-
echo "Remove any left open docker images..."
docker kill dev || true
docker rm dev

echo "Removing old docker images..."
docker system prune -f

echo "Pull down docker image..."
docker pull $LOAD_IMAGE:$BRANCH

# ------------------------------------------------------------------------------+-
# Run the Docker image with
# our local files mounted as a volume in the container.
# ------------------------------------------------------------------------------+-
echo "Running docker image..."
docker run --privileged -itd --name dev -v /tmp/:/tmp $LOAD_IMAGE:$BRANCH /bin/bash

echo "Moving files to MSP Flasher folder..."
docker exec dev bash -c "mv /tmp/load.sh /root/ti/MSPFlasher_1.3.20/"
docker exec dev bash -c "mv /tmp/AfridevV2_App_Boot_MSP430.txt /root/ti/MSPFlasher_1.3.20/"

echo "Loading MSP using docker..."
docker exec dev bash -c "cd /root/ti/MSPFlasher_1.3.20/ && ./load.sh AfridevV2_App_Boot_MSP430.txt"

echo "Cleanup docker image..."
docker kill dev || true
docker rm dev






