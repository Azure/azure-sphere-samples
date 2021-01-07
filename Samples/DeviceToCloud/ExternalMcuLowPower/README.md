---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere – Device to Cloud
urlFragment: DeviceToCloud
extendedZipContent:
- path: .clang-format
  target: .clang-format
- path: BUILD_INSTRUCTIONS.md
  target: BUILD_INSTRUCTIONS.md
- path: Samples/SECURITY.md
  target: SECURITY.md
- path: Samples/troubleshooting.md
  target: troubleshooting.md
description: "Demonstrates how you might use an Azure Sphere device as an interface between an external MCU and an IoT application platform."
---

# Sample: Device to Cloud - external MCU, low power

This reference solution demonstrates how you might use an Azure Sphere device as an interface between an external MCU, not connected to the internet, and an IoT application platform such as IoT Central.

This solution is implemented to optimize for low power by incorporating features such as the Azure Sphere device [Power Down state](https://docs.microsoft.com/azure-sphere/app-development/power-down) when it's not collecting data from the MCU or communicating with IoT Central.

## Overview

This solution models a soda machine that regularly sends usage data to IoT Central and receives new flavor recipes from IoT Central. An external MCU is used to implement the soda machine model. An Azure Sphere MT3620 provides the communication interface between the MCU and IoT Central.

When the user initiates an action, the external MCU wakes, increments its usage count, stores the new value, and returns to low power mode. If it detects a low inventory level it informs the Azure Sphere MT3620.

The Azure Sphere MT3620, connected to the MCU via UART, periodically collects the data from the MCU and sends it to IoT Central. The Azure Sphere MT3620 also receives, and passes on to the MCU, configuration data from IoT Central.

The [Azure Sphere Hardware Designs repository](https://dev.azure.com/msazuresphere/4x4/_git/cust-Hardware?path=%2FGitHub-Local--azure-sphere-hardware-designs) on GitHub contains a [hardware reference design](https://github.com/Azure/azure-sphere-hardware-designs/tree/master/P-MT3620EXMSTLP-1-0) that demonstrates how to integrate an MT3620 module and an external MCU onto a single Printed Circuit Board (PCB) with a low-power design.

See [Build and deploy the External MCU, Low Power reference solution](BuildMcuToCloud.md) to learn how to build, deploy and run this reference solution with a breadboard-based hardware design.

  **Note:** You can also build and deploy this solution using the hardware reference design. However, code changes will be required. For more information, see the [README.md file](https://github.com/Azure/azure-sphere-hardware-designs/tree/master/P-MT3620EXMSTLP-1-0/README.md) for the **Device To Cloud - External MCU, Low Power** reference solution. 

### What's in the solution

The reference solution contains the following:

- The soda machine application running on an external MCU.

  An [STM NUCLEO-L031K6](https://www.st.com/en/evaluation-tools/nucleo-l031k6.html) is used as the low power external MCU but the solution can easily be adapted to run with other external MCUs.
  
- The high-level application running on an Azure Sphere MT3620.  

- The soda machine IoT Central application running in the cloud.

     Soda machine managers can use views in IoT Central to see which machines are low on stock and need refills and which machines are used more. They can also use IoT Central to send new flavors to soda machines.

## License
For details on license, see LICENSE.txt in this directory.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).



