#!/bin/sh

find ../ -name "*.d" -type f -delete
find ../ -name "*.obj" -type f -delete
find ../ -name "TI*" -type f -delete
find ../ -name "*.d" -type f -delete
find ../ -name "*.pp" -type f -delete
find ../ -name "*.rl" -type f -delete
rm AfridevV2_MSP430.map
rm AfridevV2_MSP430.out
rm AfridevV2_MSP430.txt
rm AfridevV2_MSP430_linkInfo.xml
rm AfridevV2_MSP430_msg.txt
rm AfridevV2_MSP430_rom.txt