#!/bin/sh

find ../ -name "*.d" -type f -delete
find ../ -name "*.obj" -type f -delete
find ../ -name "TI*" -type f -delete
find ../ -name "*.d" -type f -delete
find ../ -name "*.pp" -type f -delete
find ../ -name "*.rl" -type f -delete
rm AfridevV2_MSP430_Boot.map
rm AfridevV2_MSP430_Boot.out
rm AfridevV2_MSP430_Boot.txt
rm AfridevV2_MSP430_Boot_linkInfo.xml
