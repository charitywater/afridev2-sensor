#
# This utility is used to create the AfridevV2 Application upgrade message 
# using the AfridevV2 MSP430 output build file AfridevV2_MSP430.txt.
#
# It also combines the Application output text file and Boot output
# text file into a single file that can be used by the FET programmer
# to burn the flash.
#
# This utility assumes its running from a directory parallel to the 
# MSP430 Application Project and MSP430 Boot Project.
#
#
#

import subprocess
import os.path
import shutil
import re

cwd = os.getcwd()
# print (cwd)

# Identify the Application and Boot files
InputAppOutFileName = cwd + '/' + '../AfridevV2_MSP430_Manuf/Debug/AfridevV2_MSP430_Manuf.out'
InputBootOutFileName = cwd + '/' + '../AfridevV2_MSP430_Boot/CCS_AfridevV2_MSP430_Boot/Debug/AfridevV2_MSP430_Boot.out'
InputAppTxtFileName = cwd + '/' + '../AfridevV2_MSP430_Manuf/Debug/AfridevV2_MSP430_Manuf.txt'
InputBootTxtFileName = cwd + '/' + '../AfridevV2_MSP430_Boot/CCS_AfridevV2_MSP430_Boot/Debug/AfridevV2_MSP430_Boot.txt'

# print (InputAppOutFileName)

# ==============================================================
# Run the hex430.exe file to build the ROM output file first.
# The ROM file is used to create the Upgrade Firmware Message.
#

# Make sure the MSP430 application out file is present
fileFound = os.path.exists(InputAppOutFileName)
if fileFound == False:
    print ('Could not locate application out file')
    exit()
else:
    print ('Application out file found')
    shutil.copy (InputAppOutFileName, cwd);

# Run the hex430 to create the rom file from the MSP app out file
# C:\ti\ccsv5\tools\compiler\msp430_4.2.1\bin\hex430.exe afridevV2_app_to_rom.cmd
p = subprocess.Popen([ 'hex430.exe', 'afridevV2_app_to_rom.cmd' ], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
for line in p.stdout.readlines():
    print (line)
retval = p.wait()

# Create the AfridevV2 Upgrade Message File
p = subprocess.Popen([ 'python', 'afridevV2RomToMsg.py', 'AfridevV2_MSP430_rom.txt', 'AfridevV2_MSP430_msg.txt' ], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
for line in p.stdout.readlines():
    print (line)
retval = p.wait()

fileFound = os.path.exists('AfridevV2_MSP430_msg.txt')
if fileFound == False:
    print ('AfridevV2_MSP430_msg.txt file not created.')
    exit()
else:
    print ("File AfridevV2_MSP430_msg.txt created.")

# ==============================================================
# -Combine the application file and the boot file
#  into a single txt file.
# -The combination is created using the two build output text
#  files
#   + AfridevV2_MSP430/Debug/AfridevV2_MSP430.txt'
#   + AfridevV2_Boot_MSP430/Debug/AfridevV2_Boot_MSP430.txt'
# -We need to remove the line with the "q" at the end of the
#  Application output text file when we combine the two files.
#

# Make sure the MSP430 boot out file is present
fileFound = os.path.exists(InputBootOutFileName)
if fileFound == False:
    print ('Could not locate boot out file')
    exit()
else:
    print ('Boot file found')

# Create the combined boot file and Application file
# destination = open('afridevV2_App_Boot_MSP430.txt','wb')
# shutil.copyfileobj(open(InputAppTxtFileName,'rb'), destination)
# shutil.copyfileobj(open(InputBootTxtFileName,'rb'), destination)
# destination.close()

# Create a regex to ignore any line starting with a "q".
reFilterTxtFile = re.compile("^[^q]")
# Open the rom file and get rid of any non-code lines.
txtFileString = ""
f = open(InputAppTxtFileName, 'r')
for line in f:
    if reFilterTxtFile.match(line) != None:
        txtFileString+=line
f.close()
f = open(InputBootTxtFileName, 'r')
for line in f:
        txtFileString+=line
f.close()
f = open('AfridevV2_App_Boot_MSP430.txt', 'w')
f.write (txtFileString)
f.close()

fileFound = os.path.exists('AfridevV2_App_Boot_MSP430.txt')
if fileFound == False:
    print ('AfridevV2_App_Boot_MSP430.txt file not created.')
    exit()
else:
    print ("File AfridevV2_App_Boot_MSP430.txt created.")

# ==============================================================
