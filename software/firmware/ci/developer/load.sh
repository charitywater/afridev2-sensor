#!/bin/bash

# ==============================================================================================#=
# FILE:
#     ci/developer/load.sh
#
# This script loads the MSP attached to DTA at a given IP address.
#
# Copyright 2019 Twisthink. All Rights Reserved.
# This is unpublished proprietary source code of Twisthink.
# The contents of this file may not be disclosed to third parties, copied, or duplicated
# in any form, in whole or in part, without the prior written permission of Twisthink.
# ==============================================================================================#=

DTA_USR="twisthink"
LOAD_IMAGE="registry.gitlab.com/twisthink/charity-water/afd2/afridev_2/ci-image"
BRANCH="develop"

echo "Setting up script shell CDPATH..."
export CDPATH=".:../:../../:../../../"
cd afridev_2

# Confirm script has one argument
if [ $# -ne 1 ]; then
    echo "Usage: ./load.sh <DTA name>"
    echo "Valid station names:"
    jq '.dta_list | keys[]' -r ci/config.json
    exit 1
fi

# Confirm script has valid 1 argument
jq ".dta_list.\"$1\"" -e ci/config.json &> /dev/null
if [ $? != 0 ]; then
    echo "Invalid argument: "$1
    echo "Valid arguments:"
    jq '.dta_list | keys[]' -r ci/config.json
    exit 1
fi

# Determine DTA IP address
echo ""
echo "Parsing config JSON file..."
DTA_IP=$(cat ci/config.json | jq -r ".dta_list.\"$1\".dta_ip")
echo $DTA_IP

# Verify that DTA is reachable
echo ""
echo "Checking connection to DTA..."
ping -c 1 $DTA_IP
rc=$?; if [[ $rc != 0 ]]; then
    echo "Failed to ping DTA: ${rc}"
    exit $rc;
fi

# check if bin exists
if [ -f application/build/AfridevV2_App_Boot_MSP430.txt ]; then
    echo ""
    echo "Copying txt file to DTA..."
    scp -oStrictHostKeyChecking=no application/build/AfridevV2_App_Boot_MSP430.txt $DTA_USR@$DTA_IP:/tmp/
else
    echo "Cannot find binary file: <application/build/AfridevV2_App_Boot_MSP430.txt>"
    echo "Suggest building software first using: ./build.sh all"
    exit 1
fi

# check if script exists
if [ -f ci/helpers/load.sh ]; then
    echo ""
    echo "Copying load script to DTA..."
    scp -oStrictHostKeyChecking=no ci/helpers/load.sh $DTA_USR@$DTA_IP:/tmp/
else
    echo "Cannot find script file: <ci/helpers/load.sh>"
    exit 1
fi


# ------------------------------------------------------------------------------+-
# Setup Docker CI image on the DTA
# ------------------------------------------------------------------------------+-
echo "Remove any left open docker images..."
ssh -oStrictHostKeyChecking=no $DTA_USR@$DTA_IP "docker kill dev || true"
ssh -oStrictHostKeyChecking=no $DTA_USR@$DTA_IP "docker rm dev"

echo "Removing old docker images..."
ssh -oStrictHostKeyChecking=no $DTA_USR@$DTA_IP "docker system prune -f"

echo "Pull down docker image on DTA..."
ssh -oStrictHostKeyChecking=no $DTA_USR@$DTA_IP "docker pull $LOAD_IMAGE:$BRANCH"

# ------------------------------------------------------------------------------+-
# Run the Docker image on the DTA with
# our DTA local files mounted as a volume in the container.
# ------------------------------------------------------------------------------+-
echo "Running docker image on DTA..."
ssh -oStrictHostKeyChecking=no $DTA_USR@$DTA_IP "docker run --privileged -itd --name dev -v /tmp/:/tmp $LOAD_IMAGE:$BRANCH /bin/bash"

echo "Moving files to MSP Flasher folder..."
ssh -oStrictHostKeyChecking=no $DTA_USR@$DTA_IP "docker exec dev bash -c \"mv /tmp/load.sh /root/ti/MSPFlasher_1.3.20/\""
ssh -oStrictHostKeyChecking=no $DTA_USR@$DTA_IP "docker exec dev bash -c \"mv /tmp/AfridevV2_App_Boot_MSP430.txt /root/ti/MSPFlasher_1.3.20/\""

echo "Loading MSP using docker on DTA..."
ssh -oStrictHostKeyChecking=no $DTA_USR@$DTA_IP "docker exec dev bash -c \"cd /root/ti/MSPFlasher_1.3.20/ && ./load.sh AfridevV2_App_Boot_MSP430.txt\""

echo "Cleanup docker image..."
ssh -oStrictHostKeyChecking=no $DTA_USR@$DTA_IP "docker kill dev || true"
ssh -oStrictHostKeyChecking=no $DTA_USR@$DTA_IP "docker rm dev"






