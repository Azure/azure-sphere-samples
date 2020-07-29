# Hardware definitions

Azure Sphere hardware is available from multiple vendors, and each vendor may expose features of the underlying chip in different ways. However, through hardware abstraction, the sample applications in the Azure Sphere samples repository on GitHub are hardware independent. They will run on any Azure Sphere-compatible hardware without changes to the code.

Hardware abstraction is achieved through the use of hardware definition files. The samples are written to run on an abstract "sample appliance". For each supported board or module there is a hardware definition file called sample_appliance.json.
This file maps the "peripherals" used by the sample appliance to the corresponding peripherals on the board or module. The contents of this file are used during the build procedure to update the app_manifest.json file and in compiling and packaging the sample.

To run any of the samples in the [Azure Sphere samples repository](https://github.com/Azure/azure-sphere-samples) on supported Azure Sphere hardware, you specify the directory that contains the sample_appliance.json file for the target hardware.

This README file describes how to [set the target hardware for a sample](#set-the-target-hardware-for-a-sample-application). For additional information, see [Manage hardware dependencies](https://docs.microsoft.com/azure-sphere/app-development/manage-hardware-dependencies) and [Hardware abstraction files](https://review.docs.microsoft.com/azure-sphere/hardware/hardware-abstraction) in the online documentation. 

## Supported Hardware

The samples support the following Azure Sphere hardware:

**Development Kits**

- [MT3620 RDB](mt3620_rdb/)
- [Seeed MT3620 mini-dev board](seeed_mt3620_mdb/), which uses the AI-Link WF-M620-RSC1 module
- [Avnet MT3620 SK](avnet_mt3620_sk/), which uses the Avnet AES-MS-MT3620 module
- [USI MT3620 BT EVB](usi_mt3620_bt_evb/), which uses the USI USI-MT3620-BT-COMBO module

## Set the target hardware for a sample application

To set or change the target hardware for a sample application, use CMake function `azsphere_target_hardware_definition` to specify the directory that contains the hardware definition file for your target hardware.

1. Clone the [Azure Sphere samples repository](https://github.com/Azure/azure-sphere-samples) on GitHub if you have not done so already.

   **Important:** Clone the entire samples repository, instead of downloading an individual sample. The target hardware definition files for all the samples are stored in the HardwareDefinitions directory at the top level of the repository.

1. Open the CMakeLists.txt file, which is located in the sample source directory, and search for `azsphere_target_hardware_definition`.

1. Change the value of the parameter `TARGET_DIRECTORY` to the target hardware definition directory. For example, to run the sample on the Avnet MT3620 Starter Kit: 

   change 
   
   `azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../../../HardwareDefinitions/mt3620_rdb" TARGET_DEFINITION "sample_appliance.json")`

   to 
   
   `azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../../../HardwareDefinitions/avnet_mt3620_sk" TARGET_DEFINITION "sample_appliance.json")`

