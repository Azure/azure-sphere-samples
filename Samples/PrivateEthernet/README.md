# Sample: Private Ethernet

This sample C application demonstrates how you can [connect Azure Sphere to a private Ethernet network](https://docs.microsoft.com/azure-sphere/network/connect-private-network).

The app runs a basic TCP server to allow easy verification of connectivity from your PC. This basic server accepts only one connection at a time, and does not authenticate or encrypt connections: you would replace it with your own production logic.

The sample uses the following Azure Sphere libraries:

|Library   |Purpose  |
|---------|---------|
|log     |  Displays messages in the Visual Studio Device Output window during debugging  |
|networking    | Gets and sets network interface configuration |

## Prerequisites

 This sample requires the following hardware:

- Azure Sphere MT3620 board
- [Olimex ENC28J60-H development board](https://www.olimex.com/Products/Modules/Ethernet/ENC28J60-H/)
- Jumper wires to connect the boards to each other
- Ethernet support on your computer; either through an internal adapter or external adapter, such as a USB to Ethernet adapter

Make the following connections between the ENC28J60-H and MT3620 dev boards using these jumper wires:

- ENC28J60-H 3V3: 10 to MT3620 3V3: Header 3 (upper right) Pin 3
- ENC28J60-H GND: 9 to MT3620 GND: Header 2 (lower left) Pin 2
- ENC28J60-H CS: 7 to MT3620 CSA0: Header 2 (lower left) Pin 1
- ENC28J60-H SCK: 1 to MT3620 SCLK0: Header 2 (lower left) Pin 3
- ENC28J60-H MOSI: 2 to MT3620 MOSI0: Header 2 (lower left) Pin 5
- ENC28J60-H MISO: 3 to MT3620 MISO0 RTS: Header 2 (lower left) Pin 7
- ENC28J60-H INT: 5 to MT3620 GPIO5: Header 2 (lower left) Pin 4

Refer to the following graphic for details.

![Connection diagram for ENC28J60-H and MT3620](./media/ENC28J60Hconnection.jpg)

## To build and run the sample

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://docs.microsoft.com/azure-sphere/install/install).
1. Even if you've performed this set up previously, ensure you have Azure Sphere SDK version 18.11.1 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Download and install the [latest SDK](https://aka.ms/AzureSphereSDKDownload) as needed.
1. Connect your Azure Sphere device to your PC by USB.
1. Enable [application development](https://docs.microsoft.com/azure-sphere/quickstarts/qs-blink-application#prepare-your-device-for-development-and-debugging), if you have not already done so:

   `azsphere device prep-debug`
1. Package and deploy the [board configuration image](https://docs.microsoft.com/azure-sphere/network/connect-private-network) for the Microchip ENC286J60 Ethernet chip:

   `azsphere image package-board-config --preset lan-enc28j60-isu0-int5 --output enc28j60-isu0-int5.imagepackage`
   
   `azsphere device sideload deploy --imagepackage enc28j60-isu0-int5.imagepackage`
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repo and find the PrivateEthernet sample.
1. In Visual Studio, open PrivateEthernet.sln and press F5 to compile and build the solution and load it onto the device for debugging.

## Configure the Ethernet connection on your computer

1. Open Control Panel and then click **Network and Sharing Center** > **Change adapter settings**.  
1. Right-click on your Ethernet adapter and select **Properties**.
1. In the **Ethernet Properties** window, disable all items except for **Internet Protocol Version 4 (TCP/IPv4)**.
1. Select **Internet Protocol Version 4 (TCP/IPv4)**, and then click the **Properties** button to launch the **Internet Protocol Version 4 (TCP/IPv4) Properties** window.
1. Set the follow IP address fields:
    - **IP address** to 192.168.100.11
    - **Subnet mask** to 255.255.255.0
    - **Default gateway** to blank
1. Click **OK** to close the **IPv4 properties window**, then close the **Ethernet Properties** window.
1. Attach an Ethernet cable from the ENC286J60-H to the Ethernet connection on your computer.

**Note:** If your computer is managed by policies that prevent it from being connected to multiple network interfaces at once, you may need to disable other network interfaces while using this sample.

**Note:** The samples uses the IP address range 192.168.100.xxx. If you have another network adapter using the same range, then you will need to either modify the sample or disable the other network adapter temporarily.

## Test TCP over the Ethernet connection

On your computer, use a terminal application to open a raw TCP connection to the MT3620 at 192.168.100.10 port 11000.  You can open this connection with a third-party terminal application such as PuTTY, or with the built-in Telnet client for Windows.

To use the built-in Telnet client for Windows:

1. Open Control Panel and click **Programs and Features** > **Turn Windows features on or off** to launch the **Windows Features** window.
1. Ensure **Telnet Client** is selected and click **OK**.
1. Open a command prompt and type **telnet 192.168.100.10 11000**.

The characters that you type will render in the debug console in Visual Studio–either immediately or when you enter a newline–showing they have been received by the example TCP server on the MT3620.  Furthermore, when you enter a newline, the MT3620 will send a string back to the terminal, which says:

   ```sh
   Received "<last-received-line>"
   ```

## Troubleshooting

- If you run the sample without the ENC28J60 attached (or improperly attached), then the sample app will exit immediately. The debug output will show an error such as "ERROR: Networking_SetStaticIp: 5 (I/O error)" just before it exits.  If you subsequently attach or fix the connection to the ENC28J60, then you must also reset the MT3620.
- If you run the sample without the board configuration being loaded onto the device, then the sample app will exit immediately. The debug output will show an error such as "ERROR: Networking_SetStaticIp: 2 (No such file or directory)" just before it exits.

## Removing the Ethernet board configuration

If you no longer require Ethernet, for example because you wish to use your board for a different project, you must manually remove the Ethernet board configuration image:

1. Find the installed image with type 'Board config', and note its component ID:

   `azsphere device image list-installed`
1. Delete this image: 

   `azsphere device sideload delete --componentid <component ID>`
1. Press the reset button on the MT3620 dev board.

**Note:** This sample uses ISU0 (I2C/SPI/UART port 0) on the MT3620, which is also used by other samples. Other samples can be adapted to use a different ISU port. For now, it’s not possible to adapt this Private Ethernet sample to use another ISU port.

## Over-the-air deployment
If you want to deploy this sample over-the-air, ensure to [deploy both the board configuartion and the application.](https://docs.microsoft.com/azure-sphere/network/connect-private-network)

## License
For details on license, see LICENSE.txt in this directory.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).