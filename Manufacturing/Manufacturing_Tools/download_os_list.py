#!/usr/bin/env python3

'''
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.

download_os_list.py

This sample script downloads the latest Azure Sphere Operating System OS and Image data.
The file will be stored in the local folder as 'mt3620an.json' and is used as an input for deviceready.py
'''

from genericpath import isdir
import requests
import os
import sys 

print('Download the latest mt3620an.json (OS versions) file')
os_file='mt3620an.json'
output_path = os.path.join(os.path.dirname(os.path.realpath(__file__)),os_file)
if len(sys.argv) == 2:
    if isdir(sys.argv[1]):
        output_path=os.path.join(sys.argv[1],os_file)
    else:
        print(f'"{sys.argv[1]}" is not a valid directory')
        print('Run the script without arguments to download to the script folder')
        print('or provide a valid output directory name')
        sys.quit(1)

response=requests.get('https://prod.releases.sphere.azure.net/versions/mt3620an.json')

with open(output_path, "w") as file:
        file.write(response.text)

print(f'File downloaded to {output_path}')