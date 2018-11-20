# Sample: BLE-based Wi-Fi setup - reference solution

This reference solution demonstrates how you might complete [Wi-Fi configuration](https://docs.microsoft.com/en-us/azure-sphere/network/wifi-including-ble) of an Azure Sphere-based device through Bluetooth Low Energy (BLE) using a companion app on a mobile device. This solution utilizes a Nordic nRF52 Development Kit to provide BLE connectivity over UART to the Azure Sphere MT3620 board, and a Windows 10 app to illustrate the companion user experience. 

> [!IMPORTANT]
> This solution currently uses a basic approach to BLE connectivity between the nRF52 and the companion app, which is not suitable for production use. Specifically, any companion device can connect whenever the nRF52 is active, and the connection between the companion app and the nRF52 is not encrypted. Future updates to this solution will demonstrate a more restrictive approach.

## Preparation

This reference solution requires the following:

- Azure Sphere MT3620 board
- Nordic nRF52 BLE development board
- Jumper wires to connect the boards to each other
- Two free USB ports to connect both boards to your computer
- BLE support on your computer, either through internal hardware or external hardware such as a USB BLE dongle
- Windows 10 Fall Creators edition (1709) or newer, which is required for its updated BLE support
- Enabling Developer Mode on Windows, which enables installation of the sample Windows 10 companion app

To set Windows to use Developer Mode:
1. In Settings, click **Update & Security**, and then click **For Developers**.
1. Under **Use developer features** select **Developer mode**.

## Connect Azure Sphere MT3620 to the Nordic nRF52

Make the following connections between the nRF52 and MT3620 dev boards using the jumper wires:

- nRF52 RX: P0.11 to MT3620 UART0 TX: Header 2 (lower left) Pin 3
- nRF52 TX: P0.12 to MT3620 UART0 RX: Header 2 (lower left) Pin 1
- nRF52 CTS: P0.23 to MT3620 UART0 RTS: Header 2 (lower left) Pin 7
- nRF52 RTS: P0.22 to MT3620 UART0 CTS: Header 2 (lower left) Pin 5
- nRF52 Reset: P0.21 to MT3620 GPIO5: Header 2 (lower left) Pin 4
- nRF52 DFU: P0.16 to MT3620 GPIO44: Header 2 (lower left) Pin 14
- nRF52 Ground: GND to MT3620 GND: Header 2 (lower left) Pin 2

Refer to the following graphic for details.

![Connection diagram for nRF52 and MT3620](./media/nRF52_MT3620_connection.png)

## Install the nRF52 app on the nRF52 Development Kit

1. If you haven't already, clone this repo: 
    ```sh
    git clone https://github.com/Azure/azure-sphere-samples.git

1. Connect the nRF52 developer board to your computer using USB. Once connected, the nRF52 displays a JLINK removable drive in Windows.
1. Find the nRF52 binary at WifiConfigViaBle/Binaries/softdevice_WifiConfigViaBleApp.hex.
1. Copy this file to the root of the JLINK removable drive. Once this is complete, the nRF52 restarts automatically and runs the sample application.

## Run the Azure Sphere app

1. Open WifiConfigViaBle/AzureSphereApp/WifiConfigViaBle.sln in Visual Studio.
1. Build and debug the application (F5).
1. Press button A on the MT3620 board. The Azure Sphere app will start the nRF52, wait for notification that the nR52 app is active, and then request that it begin advertising. LED 2 on the MT3620 should light up blue when this is complete.
1. Note the randomly generated device name in the Output window in Visual Studio. You will use this name to identify the BLE connection in a subsequent step. The name is similar to *Azure_Sphere_BLE_123456*. 

**Note:** The Azure Sphere app will stop the nR52 after 5 minutes. If you've not completed the following steps in time, simply press button A on the MT3620 board again to restart it.

## Run the Windows 10 companion app on your PC

This Windows app allows you to use your development PC to simulate a mobile app configuring Wi-Fi on the Azure Sphere device, via the BLE connection to the nRF52, and provides a code reference that can be ported to other platforms.

1. Start a separate instance of Visual Studio.
1. Open WifiConfigViaBle/WindowsApp/WifiConfigViaBle.sln.
1. Build and debug the application (F5). If this is your first time developing Universal Windows Platform (UWP) applications on this computer, you may be required to download the Universal Windows Platform Development workload.

## Configure the Wi-Fi settings

1. Once the app runs, click the **Scan for devices** button at the top to scan for BLE devices.
1. Select your device from the list. It has the name you noted earlier in the "Run the Azure Sphere app" step.
1. Click **Connect**. This shows the current Wi-Fi status on the device.
1. If there is no active Wi-Fi network, click **Add new network...**. If there is already a network, you can delete it by pressing button B on the MT3620 development board.
1. Click **Scan for Wi-Fi networks**. This may take a few seconds to display a complete list of networks that the Azure Sphere device can see. Only open and WPA2 networks are supported.
1. If the network is secured, a prompt appears for a network password. Enter the password and then click **Connect**. If you are connecting to an open network, simply click **Connect**.
1. Observe that the current Wi-Fi status is displayed again, and refreshed every 5 seconds. You should see the Azure Sphere device connect to the new Wi-Fi network successfully. 

If you are running the Azure Sphere and Windows apps in debug mode in Visual Studio, the Output window should show the protocol communications they are sending and receiving. If you aren't seeing the messages, check the wiring between the boards is correct, or reset the boards.

## Design overview

The Azure Sphere application:

- Uses a custom, extensible message-passing protocol to communicate with both the nRF52 Application (via UART) and the Windows Application (via UART and then Bluetooth LE)
- Instructs the nRF52 application to start Bluetooth advertising with a custom device name
- Communicates with the Windows application to send it scan results and current network details, and receive new network details
- Allows user to delete all stored Wi-Fi networks by pressing Button B on MT3620 development board

The Nordic nRF52 application:

- Performs BLE setup under the control of the Azure Sphere application
- Forwards messages between the Windows application (communicating via BLE) and the Azure Sphere application (communicating via UART)

The Windows 10 application:

- Is built the Universal Windows Platform (UWP)
- Uses a portable class library (PCL)-based DLL to enable connection to an MT3620 device via Bluetooth LE

## Build your own solution

To edit and re-deploy the Azure Sphere and Windows apps, use Visual Studio as per the steps above.

To edit and re-deploy the nRF52 app:

1. Download and install [SEGGER Embedded Studio](https://www.segger.com/downloads/embedded-studio). [Download the 32-bit version](https://www.segger.com/downloads/embedded-studio/EmbeddedStudio_ARM_Win_x86) not the 64-bit version. Ensure you are licensed to use it for your purposes. In Oct 2018, we were able to [obtain the license for free because we were developing for NRF52](https://www.segger.com/news/segger-embedded-studio-ide-now-free-for-nordic-sdk-users/).
1. Install (download and extract) the [Nordic NRF5 SDK V15.2](https://www.nordicsemi.com/eng/Products/Bluetooth-low-energy/nRF5-SDK#Downloads)
1. Edit the nRF52 sample app so you can build it against your SDK:
    - Use a text editor to open WifiConfigViaBle\Nrf52App\pca10040\s132\ses\ble_app_uart_pca10040_s132.emProject.
    - Set the SDK_ROOT variable in this file to point to the root directory of your Nordic SDK install. Specifically, replace the words "CHANGE_THIS_TO_YOUR_NORDIC_SDK_PATH" with the correct path, changing any backslashes ("\") to forward slashes ("/") in the path. For example: macros="SDK_ROOT=C:/Users/ExampleUser/source/nRF5_SDK_15.2.0_9412b96;…"
1. Open this .emProject file in the Segger IDE.
1. Build and debug the application (F5).

You can also use Azure Sphere to deploy the nRF52 app itself. See the [reference solution for External MCU update](https://github.com/Azure/azure-sphere-samples/tree/master/Samples/ExternalMcuUpdateNrf52).

## License
For license details, see LICENSE.txt in each directory.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).

