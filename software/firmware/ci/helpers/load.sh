#!/bin/sh

# ==============================================================================================#=
# FILE:
#     helpers/load_msp.sh
#
# Load the SSM main application using MSPFlasher utility.
# Expects MSPFlasher utility to be installed in Linux.
# Run this script from the MSPFlasher directory.
# Usage: ./load_ssm <path/to/binary.txt>
#
# Copyright 2019 Twisthink. All Rights Reserved.
# This is unpublished proprietary source code of Twisthink.
# The contents of this file may not be disclosed to third parties, copied, or duplicated
# in any form, in whole or in part, without the prior written permission of Twisthink.
# ==============================================================================================#=

if [ $# -ne 1 ]; then
    echo "Usage: ./load_msp <path/to/binary.txt>"
    exit 1
fi

if [ -f $1 ]; then
    echo "Attempting to load MSP430..."
    echo "Please wait..."
    ./MSP430Flasher -n MSP430G2X55 -w $1 -v -z [VCC]
else
    echo "MSP430 Flasher Failed!"
    exit 1
fi
