#!/usr/bin/env python3.6

# V1.0 08/06/20
#   Initial Release

#  Known issues:
#
# 1. The application will crash and not restart if the serial cable is removed while it's running
#    To recover, restart the Pi.  The script will automatically run on boot
#


# This application works with the Guardian100 POC Lab and example application
# Required Hardware
#   Avnet Azure Sphere Guardian 100: https://www.avnet.com/shop/us/products/avnet-engineering-services/aes-ms-mt3620-guard-100-3074457345641694219
#   Raspbery Pi to run this script
#
# Guardian 100 application details
#
# This application interfaces with the G100 application 
# Repo: https://github.com/Avnet/azure-sphere-samples.git
# Branch:  POC
#
# The application implements the following command/responses
#
# 1. RebootCmd: Calls "sudu reboot" to restart the Pi, does not return any data
# 2. PowerdownCmd: Calls "sudo shutdown -P" to power down the Pi, does not return and data
# 3. IpAddressCmd: Reads all active network interfaces and returns interface name and IP address
#    wlan0:192.168.1.12\n
# 4. ReadCPUTempCmd: Reads CPU temperature and returns
#    temp:47.3\n
#

import os
import subprocess
import serial
import atexit
import time
import ifaddr # pip3 install ifaddr

####################################################
#          User Defined Functions                  #
####################################################

###########  sendIpAddressInfo()  ##################
# This function finds all the active pi network adapters then sends the adapter name and IpV4 IP address over the serial
# port i.e., "eth0:192.168.1.2\n"
def sendIpAddressInfo():
    adapters = ifaddr.get_adapters()

    for adapter in adapters:
        try:
            addrData = adapter.nice_name + ":%s\n" % (adapter.ips[0].ip)
        except TypeError:
            print('Type Error Encountered')
        else:
#            print(addrData)
            G100ser.write(addrData.encode())

###########  sendCPUTemperature()  ##################
# This function reads the CPU temperature in C and sends it to the G100
def sendCPUTemperature():

    # Read the temperature
    tempString = subprocess.check_output('/opt/vc/bin/vcgencmd measure_temp', shell=True).decode("utf-8")

    # Replace the '=' with ':'
    tempString = str(tempString.replace('=', ':'))

    # Strip the 'C from the end of the string
    tempString = tempString.replace('\'C', '')

    # Send the data to the G100
    G100ser.write(tempString.encode())
#    print(tempString)

###########  cleanupBeforeExit()  ##################
# This function will be called on exit.
def cleanupBeforeExit():

  print(f'Exiting and cleaning up')
  G100ser.close()

####################################################
#              Main Body Here                      #
####################################################

print ('Starting G100 Downstream (slave) application')

# Define the exit routine
atexit.register(cleanupBeforeExit)

# Declare the serial port object
G100ser = serial.Serial()

# Open the serial port.  Note that we try to open both /dev/ttyACM0 and /dev/ttyACM1
# we have seen the interface use either of these interfaces.
while True:
  try:
    try:
      G100ser = serial.Serial('/dev/ttyACM0', 115200, parity=serial.PARITY_NONE, bytesize=serial.EIGHTBITS, stopbits=serial.STOPBITS_ONE, rtscts=1)
      break
    except serial.serialutil.SerialException:
      G100ser = serial.Serial('/dev/ttyACM1', 115200, parity=serial.PARITY_NONE, bytesize=serial.EIGHTBITS, stopbits=serial.STOPBITS_ONE, rtscts=1)
      break
  except:
    print ("Error opening Serial Port, retrying . . . ")
    time.sleep(.5)

print (f'Opened Serial Port {G100ser.name} to G100')

# Enter a loop to read and process commands from the G100 device
while True:
    # Read data until a end of line is detected, this is a blocking call
    newCmd = G100ser.readline().rstrip()

    # Check for each supported command and execute the requested command/function
    try:
        if(newCmd.decode() == 'RebootCmd'):
            os.system('sudo reboot')
        elif(newCmd.decode() == 'PowerdownCmd'):
            os.system('sudo shutdown -P')
        elif(newCmd.decode() == 'IpAddressCmd'):
            sendIpAddressInfo()
        elif(newCmd.decode() == 'ReadCPUTempCmd'):
            sendCPUTemperature()
        else:  # Unknown command
            print(f'unknown command received: {newCmd} ')
    except:
        print('Something went wrong . . .')