# Reference solution: Device To Cloud - External MCU, Low Power

This reference solution demonstrates how you might connect an external MCU to an Azure Sphere device and send data to IoT Central. This solution is implemented to optimize for low power by incorporating features such as the Azure Sphere device [Power Down state](https://docs.microsoft.com/azure-sphere/app-development/power-down) when it's not collecting data from the MCU or communicating with IoT Central.

## Overview

This solution models a soda machine that regularly sends usage data to IoT Central and receives new flavor recipes from IoT Central. An external MCU is used to implement the soda machine model. An Azure Sphere device provides the communication interface between the MCU and IoT Central.

When the user initiates an action, the external MCU wakes, increments its usage count, stores the new value, and returns to low power mode. If it detects a low inventory level it informs the Azure Sphere device.

The Azure Sphere device, connected to the MCU via UART, periodically collects the data from the MCU and sends it to IoT Central. The Azure Sphere device also receives,
and passes on to the MCU, configuration data from IoT Central.

See [Build and deploy the External MCU, Low Power reference solution](BuildMcuToCloud.md) to learn how to build, deploy and run this reference solution.

### What's in the solution

The reference solution contains the following:

- The soda machine application running on an external MCU board.

  An [STM NUCLEO-L031K6](https://www.st.com/en/evaluation-tools/nucleo-l031k6.html) development board is used as the low power external MCU but the solution can easily be adapted to run with other external MCUs.
  
- The high-level application running on an Azure Sphere MT3620 board.  
   
- The soda machine IoT Central application running in the cloud.    

     Soda machine managers can use views in IoT Central to see which machines are low on stock and need refills and which machines are used more. They can also use IoT Central to send new flavors to soda machines.   

## License
For details on license, see LICENSE.txt in this directory.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).