#!/bin/sh

find ../ -name "*.d" -type f -delete
find ../ -name "*.obj" -type f -delete
find ../ -name "TI*" -type f -delete
find ../ -name "*.d" -type f -delete
find ../ -name "*.pp" -type f -delete
find ../ -name "*.rl" -type f -delete
rm AfridevV2_MSP430_Manuf.map
rm AfridevV2_MSP430_Manuf.out
rm AfridevV2_MSP430_Manuf.txt
rm AfridevV2_MSP430_Manuf_rom.txt
rm AfridevV2_MSP430_Manuf_msg.txt
rm AfridevV2_MSP430_Manuf_linkInfo.xml
