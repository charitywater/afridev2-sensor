#!/bin/bash

# Check for zero arguments
if [ $# -eq 0 ]; then
    echo "Usage: ./build.sh <app/manuf/boot/all>"
    exit 1
fi

# Check for >1 arguments
if [ $# -ne 1 ]; then
    echo "Usage: ./build.sh <app/manuf/boot/all>"
    exit 1
fi

if [ $1 != "app" ] && [ $1 != "manuf" ] && [ $1 != "boot" ] && [ $1 != "all" ]; then
    echo "Usage: ./build.sh <app/manuf/boot/all>"
    exit 1
fi 


# From Linux Development host, build the applications using Docker MSP image.
BUILD_IMAGE="registry.gitlab.com/twisthink/charity-water/afd2/afridev_2/msp-image"
BRANCH="develop"

# Remove old docker images before anything else
echo
echo docker prune...
docker system prune -f

export CDPATH=".:../:../../:../../../"
cd afridev_2

echo
echo Pulling down docker image...
docker pull $BUILD_IMAGE:$BRANCH

echo
echo Starting BUILD docker image and mapping src code into image...
# docker run -it --name build -v /home/twisthink/afridev_2/:/afridev_2 registry.gitlab.com/twisthink/charity-water/afd2/afridev_2/msp-image:master /bin/bash
docker run -it -d --name build -v ${PWD}:/afridev_2 $BUILD_IMAGE:$BRANCH /bin/bash

echo
echo Executing commands to build SSM software...

if [ $1 == "app" ] || [ $1 == "all" ]; then 
    docker exec build bash -c "cd /afridev_2/application/build && ./build.sh"
    docker exec build bash -c "cd /afridev_2/application/build && ../../ci/helpers/afridevV2RomToMsg.py AfridevV2_MSP430_rom.txt AfridevV2_MSP430_msg.txt"
fi

if [ $1 == "boot" ] || [ $1 == "all" ]; then
    docker exec build bash -c "cd /afridev_2/bootloader/build && ./build.sh"
fi 

if [ $1 == "all" ]; then
    docker exec build bash -c "cd /afridev_2/ci/helpers && ./create_combined_file.py ../../application/build/AfridevV2_MSP430.txt ../../bootloader/build/AfridevV2_MSP430_Boot.txt"
    docker exec build bash -c "mv /afridev_2/ci/helpers/AfridevV2_App_Boot_MSP430.txt /afridev_2/application/build/"
fi 



# Stop development docker if still running
echo
echo Stopping and removing docker images...
docker kill build || true
docker rm build