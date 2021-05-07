#!/usr/bin/python3

# Convert a TI rom file to an AfridevV2 Upgrade Message
# Creates Msgheader
# Calculates CRC16
# Determines Length

import re
import crcmod
import binascii
import sys

print("Converting ROM file to a AFD2 upgrade message...")
print('Number of arguments:', len(sys.argv), 'arguments.')
print('Argument List:', str(sys.argv))

# Constants
if len(sys.argv) == 3:
    InputRomFileName = sys.argv[1]
    OutputFileName = sys.argv[2]
else:
    InputRomFileName = 'AfridevV2_MSP430_rom.txt'
    OutputFileName = None

# create regex to remove any non code lines from the rom file.
# for example, the @addr lines and the "q" line are the end
reFilterRom = re.compile("^[0-9,A-F][0-9,A-F]")

# Open the rom file and get rid of any non-code lines.
romFilteredFileString = ""
f = open(InputRomFileName, 'r')
for line in f:
    if reFilterRom.match(line) != None:
        romFilteredFileString+=line

# create regex to parse ti text rom file and grab the data
re = re.compile("[0-9,A-F][0-9,A-F]")

# open rom file and read into string
# with open('outpour_MSP430_rom.txt') as f:
# 	romCodeSection = f.read()

# Using the regex, Parse data into list - removing all spaces and newlines
# Each entry in the list will one string byte value (i.e. 'FF')
# parsedCodeSectionList = re.findall(romCodeSection)

parsedCodeSectionList = re.findall(romFilteredFileString)

# Determine length of list (i.e. number of bytes)
length = len(parsedCodeSectionList)

# print (parsedCodeSectionList)

# Now we are going to create a string of binary data (i.e. '\xFF').
# This is what is required to perform the CRC, which accepts a string as input.
# This trick is that the values is the string must be the binary data (each byte).
# We use the binascii module ascii to bin hex function to accomplish this.
byteData = bytearray();                # If using python3
for value in parsedCodeSectionList:
	# binVal = binascii.a2b_hex(value) # If using python2
	# byteData += binVal               # If using python2
	byteData.append(int(value,16))     # If using python3

# print(byteData)

# Perform the CRC.
# First setup of CRC16 ANSI, polynomial = 0x8005.  Must add the 1 in front, as it is
# usually implied but required here.
# To match what is done on the MSP430, set reverse to True.
crc16_func = crcmod.mkCrcFun(0x18005, rev=True, initCrc=0x0000, xorOut=0x0000)
crc16 = crc16_func(byteData)
# print("CRC16 result is {0:04X}".format(crc16))

# Put code back into a string with spaces in between values
# We will use this to create the output file.
codeSectionString = ""
for value in parsedCodeSectionList:
	codeSectionString += "{} ".format(value)

# Create the pieces of the upgrade message
# msgNum 2: 1 byte
# msgId   : 2 bytes
# key0-3  : 4 bytes
# Total number of sections: 1 byte
# MsgHeader Total: 8 Bytes
msgNumber = 0x10
msgIdMsb  = 0x01
msgIdLsb  = 0x02
key0 = 0x31
key1 = 0x41
key2 = 0x59
key3 = 0x26
# Hardcode the number os sections for now.
numberOfSections = 1

# Create msg header
msgHeader1       = "{0:02X} {1:02X} {2:02X} ".format (msgNumber, msgIdMsb, msgIdLsb)
msgHeader2       = "{0:02X} {1:02X} {2:02X} {3:02X} ".format (key0, key1, key2, key3)
msgHeader3       = "{0:02X} ".format (numberOfSections)

# Create section header
# Format is:
# Start byte: 0xA5                : 1 byte
# Section Number, starting with 0 : 1 byte
# start flash address of section  : 2 bytes
# length of section               : 2 bytes
# crc16 (ANSI, poly=0x8005)       : 2 bytes
sectionNum       = "{0:02X} {1:02X} ".format(0xA5, 0x00)
sectionAddr16    = "{0:02X} {1:02X} ".format(0x90, 0x00)
sectionLength16  = "{0:02X} {1:02X} ".format(length>>8, length & 0xFF)
sectionCrc16     = "{0:02X} {1:02X} ".format(crc16>>8, crc16 & 0xFF)

upgradeMsg  = msgHeader1 + msgHeader2 + msgHeader3
upgradeMsg += sectionNum + sectionAddr16 + sectionLength16 + sectionCrc16 
upgradeMsg += codeSectionString

if OutputFileName != None:
    # write the total output message to the output file
    f = open(OutputFileName, 'w')
    f.write(upgradeMsg)
    f.close()
else:
    # Print the total output message to the console
    print (upgradeMsg)

