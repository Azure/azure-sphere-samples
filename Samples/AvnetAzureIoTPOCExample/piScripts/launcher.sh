#!/bin/bash
# launcher.sh

# navigate to the python directory where our script is located
cd /
cd home/pi/python

# (Work around): For some reason this library is not avaliable at boot time, so just install it before
# calling our script!
sudo pip3 install ifaddr

# run the python script
python3 serialCmdResp.py

# change back to the root directory
cd /