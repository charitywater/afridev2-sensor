#!/usr/bin/python3

# Create the combined boot file and Application file

import os.path
import re
import sys

if len(sys.argv) != 3:
    print("Usage: ./create_combined_file.py <application txt file> <bootloader txt file>")
    exit()


app_text_file_name = sys.argv[1]
boot_text_file_name = sys.argv[2]

# Create the combined file for BOOT and the MSP430 normal build
# Create a regex to ignore any line starting with a "q".
reFilterTxtFile = re.compile("^[^q]")
# Open the rom file and get rid of any non-code lines.
txtFileString = ""
f = open(app_text_file_name, 'r')
for line in f:
    if reFilterTxtFile.match(line) != None:
        txtFileString+=line
f.close()
f = open(boot_text_file_name, 'r')
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