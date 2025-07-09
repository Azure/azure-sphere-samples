# Reference solution: Name of sample

<Introductory sentence that describes what the reference solution does. > This reference solution shows how you can deploy an update to an external MCU device using Azure Sphere. The solution deploys firmware to the Nordic nRF52 Development Kit over UART from the Azure Sphere MT3620 board. 

<next paragraph describes why this is useful> You might require such a solution if your product incorporates other MCUs with your Azure Sphere device, and those other MCUs may require updates. Assuming the other MCUs permit updates to be loaded over the connection you establish with the Azure Sphere device, for example over UART, you can use the Azure Sphere device to securely deliver those updates.

<If you need alerts, don't use the ms-docs formatting; it doesn't work here. Use these instead. You should probably only need the first two or three.

**IMPORTANT!** followed by text on same line
**Note:** followed by text on same line
**Tip:**  followed by text on same line
**CAUTION!** followed by text on same line (implies potential negative consequences)
**WARNING!** followed by text on same line (implies certain danger)
>
 
<As a general rule: delete "/en-us" from links to the published docs. If it's omitted, readers will get their localized versions (if any) and it will default to en-us if there isn't one.>

## Overview

Describe what the reference solution entails, so that readers have a clue what they're signing up for.

- List of major steps to be performed in the solution; should map to the H2 sections that follow the overview

### What's in the solution

The reference solution contains the following: <Description of each component. List the folder that contains it if it's not obvious--but if you list one folder, list them all. Ue a table if you prefer>

- Sample Azure Sphere application that updates the nRF52 firmware
- Sample firmware file for the nRF52
- Modified version of the nRF52 bootloader to use as a sample for your own code 

### Prerequisites
Set up your device and development environment as described in the Azure Sphere documentation. [or words to that effect]
 
List any versions, hardware, software, etc. required to build this solution, if different from the minimums described in the docs. 

    - Windows (if different from standard Minbar)
    - Visual Studio version (if different from standard Minbar)
    - Azure Sphere SDK version xxx
    - Hardware? (list any specific hardware requirements)
    - Any other software, special tools, etc.?

Be sure to list the supported boards (i.e. hardware form factors) for specific samples along with the hardware changes that should be made (e.g connecting pins and resistors on a dev board).

## Step 1 (Use a real title, like "Connect the Azure Sphere device to the Nordic nRF52")

Procedures

## Next step...

## Step 3...

...

## Build your own solution

What should the customer do to adapt this for their own use?
Any Gotchas, troubleshooting tips? etc.
 

## License
For details on license, see LICENSE.txt in this directory.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).