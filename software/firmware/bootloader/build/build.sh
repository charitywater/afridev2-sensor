#!/bin/bash

#
# Compile MSP code using cl430 compiler
#

COMPILER_PATH="/ti-cgt-msp430_18.12.3.LTS/bin/"
COMPILER="cl430"
HEX="hex430"
OUTPUT_NAME=AfridevV2_MSP430_Boot

# These are options that are used for BOTH building AND linking.  Please be careful adding here.
GENERIC_OPTIONS=(   -vmsp \
                    -O4 \
                    --use_hw_mpy=none \
                    --define=__MSP430G2955__ \
                    -g \
                    --printf_support=minimal \
                    --diag_warning=225 \
                    --diag_wrap=off \
                    --display_error_number )

# Additional options for building only
BUILD_OPTIONS=( --preproc_with_compile )

# Include paths for building
BUILD_INCLUDE_PATHS=(   --include_path="/msp430/include" \
                        --include_path="../src/" \
                        --include_path="/ti-cgt-msp430_18.12.3.LTS/include")

# Additional options for linking only
LINK_OPTIONS=(  -z \
                --stack_size=80 \
                --heap_size=80 \
                --cinit_hold_wdt=on \
                --reread_libs \
                --diag_wrap=off \
                --display_error_number \
                --warn_sections \
                --xml_link_info="$OUTPUT_NAME"_linkInfo.xml \
                --rom_model)

# Include paths for linking
LINK_INCLUDE_PATHS=(  -i"/msp430/include" \
                      -i"/ti-cgt-msp430_18.12.3.LTS/lib" \
                      -i"/ti-cgt-msp430_18.12.3.LTS/include" )

# Libraries that we want to use for linking
LINK_LIBRARIES=( -llibc.a )

# Files that we want to build and then link
# As files are added/changed, this list should be updated
# The *.c at the end of each file is omitted for flexibility in the BASH script
FILES=( "../src/bootRecord" \
        "../src/appRecord" \
        "../src/hal" \
        "../src/flash" \
        "../src/main" \
        "../src/modemPower" \
        "../src/modemMgr" \
        "../src/modemCmd" \
        "../src/time" \
        "../src/utils" \
        "../src/msgOta" )

# ------------------------------------------------------------------------------------------------------
# Shouldn't need to edit below this line.
# This is where the magic happens.
#
#                        _,-'|
#                     ,-'._  |
#           .||,      |####\ |
#          \.`',/     \####| |
#          = ,. =      |###| |
#          / || \    ,-'\#/,'`.
#            ||     ,'   `,,. `.
#            ,|____,' , ,;' \| |
#           (3|\    _/|/'   _| |
#            ||/,-''  | >-'' _,\\
#            ||'      ==\ ,-'  ,'
#            ||       |  V \ ,|
#            ||       |    |` |
#            ||       |    |   \
#            ||       |    \    \
#            ||       |     |    \
#            ||       |      \_,-'
#            ||       |___,,--")_\
#            ||         |_|   ccc/
#            ||        ccc/
#            ||
# ------------------------------------------------------------------------------------------------------

length=${#FILES[@]}

# Build all individual files
for ((i=0;i<$length;i++)); do
    FULL_PATH=${FILES[$i]}
    FOLDER=$(echo $FULL_PATH | sed -r 's:(.*/)(.*$):\1:' ) # Strip down to only folder
    echo Building file: $FULL_PATH.c
    echo Invoking $COMPILER Compiler
    BUILD_COMMAND="$COMPILER_PATH$COMPILER ${GENERIC_OPTIONS[@]} ${BUILD_OPTIONS[@]} ${BUILD_INCLUDE_PATHS[@]} --preproc_dependency=$FULL_PATH.d --obj_directory=$FOLDER $FULL_PATH.c"
    echo $BUILD_COMMAND
    $BUILD_COMMAND
    if [ $? -ne 0 ]
    then
        exit 1
    fi
  echo "Successfully created file"
    echo Finished building: $FULL_PATH.c
    echo
done

# Link files and generate *.out file
echo
echo Building target: $OUTPUT_NAME.out
echo Invoking: $COMPILER Linker
LINK_COMMAND="$COMPILER_PATH$COMPILER ${GENERIC_OPTIONS[@]} ${LINK_OPTIONS[@]} -m$OUTPUT_NAME.map ${LINK_INCLUDE_PATHS[@]} -o $OUTPUT_NAME.out ${FILES[@]/%/.obj} ../CCS_AfridevV2_MSP430_Boot/lnk_msp430g2955.cmd ${LINK_LIBRARIES[@]}"
echo $LINK_COMMAND
$LINK_COMMAND
if [ $? -ne 0 ]
then
    exit 1
fi
echo Finished building target: $OUTPUT_NAME.out


echo
echo Creating TI TXT file
HEX_COMMAND="$COMPILER_PATH$HEX --ti_txt $OUTPUT_NAME.out -o $OUTPUT_NAME.txt -order MS -romwidth 16"
echo $HEX_COMMAND
$HEX_COMMAND
if [ $? -ne 0 ]
then
    exit 1
fi
echo Finished creating TI TXT file.


