# Bluetooth LE Sample

This folder contains source code and documentation to help you use a Bluetooth LE connection through a Nordic nRF52 board to set up Wi-Fi connectivity on an Azure Sphere MT3620 development board.

All the samples described here have the following minimum requirements:

- An Azure Sphere MT3620 development board, which is enabled for application development
- A Nordic nRF52 Bluetooth LE board

## The Azure Sphere sample application

Additional Requirements:

- Visual Studio Enterprise, Professional, or Community 2017 version 15.7 or later
- Azure Sphere SDK Preview for Visual Studio

The application:

- Uses a custom, extensible MessageProtocol to communicate with both the nRF52 Application (via UART) and the Windows Application (via UART and then Bluetooth LE)
- Instructs the nRF52 application to set the set up Bluetooth advertising with a custom device name
- Communicates with the Windows application to send it scan results and current network details, and receive new network details
- Allows user to delete all stored Wi-Fi networks by pressing Button B on MT3620

## The Nordic nRF52 sample application

Additional Requirements:

- The Nordic tools installed

The application:

- Performs BLE setup under the control of the Azure Sphere application
- Forwards messages between the Windows application (communicating via BLE) and the Azure Sphere application (communicating via UART)

## The Windows 10 sample application

Additional Requirements:

- Windows 10 Fall Creators edition or newer
- Visual Studio 2017 with the Universal Windows Platform development workload installed
- A MT3620 device with the sample application installed
- A Nordic nRF52 board with the sample service installed, and all correct pins connected to the MT3620 board

The Windows 10 application is a Universal Windows application, that uses a portable class library (PCL)-based DLL to enable connection to an MT3620 device via Bluetooth LE.

The application allows the user to:

- Scan for Bluetooth LE enabled devices
- Connect to a Nordic nRF52 Bluetooth LE board that is running our sample service
- Using the Bluetooth LE connection via the Nordic nRF52 board, connect to the MT3620 board's sample application to:
  - See the Wi-Fi network connectivity status of the MT3620 device
  - List the Wi-Fi networks that the MT3620 device can see
  - Connect to any open or WPA2 secured Wi-Fi networks from the above list on that MT3620 board

## License
For details on license, see LICENSE.txt in each directory.

## Code of Conduct
This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).