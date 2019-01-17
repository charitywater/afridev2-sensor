REM  If an argument is given, then the contents of the manufacturing data should be written to a file
IF %1.==. GOTO No1
C:\ti\MSPFlasher_1.3.18\MSP430Flasher -n MSP430G2X55 -r [%1.txt,0x1000-0x103F]
C:\ti\MSPFlasher_1.3.18\MSP430Flasher -n MSP430G2X55 -w AfridevV2_App_Boot_MSP430.txt -v -z [VCC] -e ERASE_MAIN
GOTO End1
:No1
  ECHO No IMEI entered, Last Step Failed
GOTO End1
:End1

