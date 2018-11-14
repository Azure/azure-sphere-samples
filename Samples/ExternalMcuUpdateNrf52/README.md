# Sample: External MCU update

This folder contains source code and documentation to help you update an external MCU through an Azure Sphere device. The sample uses a custom application on the Azure Sphere device to communicate with a Nordic nRF52 board. The nRF52 board uses the bootloader located in the Nrf52Bootloader folder and is updated with firmware from the Binaries folder.

## Modified Bootloader

The Nrf52Bootloader folder contains a modified version of the secure_bootloader\pca10040_uart_debug example from
the Nordic nRF5 SDK (version 15.2.0 - https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v15.x.x/nRF5_SDK_15.2.0_9412b96.zip)
for use on a Nordic nRF52 Development Kit.

This bootloader has been modified to:
- Use a custom board configuration where the UART pins are remapped and have pullups enabled on the input pins
- Accept signed or unsigned applications
- Accept signed or unsigned bootloaders
- Accept firmware upgrades or downgrades
- Enable DFU mode via pin input as well as button press

## Building the bootloader
You can build the bootloader two ways:
- Using gcc, with the Makefile in the subfolder nRF52_DFU\pca10040\s132\armgcc
- Using the SEGGER Embedded Studio and the project file: nRF52_DFU\pca10040\s132\ses\bootloader_uart_mbr_pca10040_debug.emProject

In both cases you must first edit the file to set the SDK_ROOT variable to point to the root
directory of an install of the Nordic SDK linked above. Replace the words "CHANGE_THIS_TO_YOUR_NORDIC_SDK_PATH"
with the correct path.

Further documentation on using gcc or SEGGER to build can be found in the [Nordic infocenter](http://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.gs%2Fdita%2Fgs%2Fgs.html&cp=1).

# Sample: External MCU update reference solution

Your product may incorporate other MCUs with your Azure Sphere device, and those other MCUs may require updates. Assuming the other MCUs permit updates to be loaded over the connection you establish with the Azure Sphere device, for example over UART, SPI, or I2C, you can use the Azure Sphere device to securely deliver those updates.

This reference solution describes a way that you might deploy an update to an external MCU device using an Azure Sphere device. In this solution, you will deploy firmware from the Azure Sphere MT3620 over UART to the Nordic nRF52 board.

## Preparation

If you have completed the [BLE-based Wi-Fi setup](../WifiConfigViaBle/README.md) reference solution, you should have already completed the preparation steps required for this solution. If you completed the BLE-based Wi-Fi setup solution and your boards are still connected, proceed to **Install bootloader on nRF52**.

This sample solution requires the following hardware:

- Azure Sphere MT3620 board
- Nordic nRF52 BLE development board
- Jumper wires to connect the boards
- Two free USB ports on your computer

## Connect nRF52 to MT3620

Use jumper wires to connect the nRF52 and the Azure Sphere MT3620 boards.

1. NRF52 RX: P0.11 <-> MT3620 UART0 TX: Header 2 (lower-left) Pin 3 
1. NRF52 TX: P0.12 <-> MT3620 UART0 RX: Header 2 (lower-left) Pin 1
1. NRF52 CTS: P0.23 <-> MT3620 UART0 RTS: Header 2 (lower-left) Pin 7
1. NRF52 RTS: P0.22 <-> MT3620 UART0 CTS: Header 2 (lower-left) Pin 5
1. Reset: P0.21 <-> MT3620 GPIO5: Header 2 (lower-left) Pin 4
1. DFU: P0.16 <-> MT3620 GPIO44: Header 2 (lower-left) Pin 14
1. Ground: GND (between P0.2 and P0.25)<-> MT3620 GND: Header 2 (lower-left) Pin 2

![Connection diagram for nRF52 and MT3620](./media/nRF52_MT3620_connection.png)

## Install bootloader on nRF52

1. If you haven't already done so, clone the **External Device Firmware Update** MT3620 sample from GitHub.  
    ```sh
    git clone https://github.com/Azure/azure-sphere-samples.git
    ```
1. Connect the nRF52 developer board to your computer using USB. Once connected, the nRF52 will display a JLINK removable drive in Windows.
1. Find the bootloader and sample binary in the AppSamples\LocalSamples\ExternalMcuUpdateNrf52\Binaries folder, and copy them to the JLINK drive presented by the nRF52.

## Run external device firmware update

1. Open, build, package, run, and deploy (F5) the AppSamples\LocalSamples\ExternalMcuUpdateNrf52\AzureSphereApp\ExternalMcuUpdateNrf52.sln in the sample you cloned from GitHub.
1. As the solution runs, observe the output window for activity messages. You should see the sample firmware install to the nRF52.
1. Restart the ExternalMcuUpdateNrf52 solution. In the output window, verify that the MT3620 app checks the version of the nRF52 firmware, and that it does not reinstall the firmware.

## Deploy different firmware for the nRF52

We provide another prebuilt firmware for this solution: BlinkyV2. Use this firmware to demonstrate that the nRF52 Developer Kit can be updated through normal Azure Sphere over the air updates.

1. In the cloned files from our repository, locate the BlinkyV2 files in the **AzureSphereApp\External Nrf52 Firmware** subfolder.
1. Edit the MT3620 application to use the BlinkyV2 files:
    - Delete the existing Nrf52Dfu resource files from the solution.
    - Add the .dat and .bin files from the BlinkyV2 application as resources for the solution.
    - Right click each resource file and set the **Content** property to *Yes*, to ensure they are included in the MT3620 image package.
    - Update the filename constants in the source to point to the new resources.  
    **Note** the *SoftDevice* BLE stack .bin and .dat are still included as resources.
1. Deploy the firmware to your MT3620.
1. Use the log debug output window to observe as the BlinkyV2 firmware is installed and run on the nRF52. Notice that the SoftDevice BLE stack was not updated because it is already at the correct version.