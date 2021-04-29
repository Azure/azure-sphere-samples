# Ethernet setup instructions

The following steps describe how to set up a sample or tutorial to use Ethernet:

1. Add an Ethernet adapter to your hardware. If you are using an MT3620 RDB, see the [wiring instructions](HardwareDefinitions/mt3620_rdb/EthernetWiring.md).
1. Package a board configuration image for the Olimex ENC28J60-H. Follow the instructions in [Create a board configuration image package](https://docs.microsoft.com/azure-sphere/network/connect-ethernet#create-a-board-configuration-image-package).
1. Deploy the board configuration image package in addition to your application image package. Follow the instructions in [Sideload a board configuration image package](https://docs.microsoft.com/azure-sphere/network/connect-ethernet#sideload-a-board-configuration-image-package).
1. Add the following line to the **Capabilities** section of the `app_manifest.json` file:

    `"NetworkConfig" : true`

1. Ensure that the global constant **networkInterface** is set to "eth0". Search for the following declaration in source code. It is typically in `main.c` but may be in a different file for some samples.

    `char networkInterface[] = "wlan0";`

    And replace `wlan0` with `eth0`:

    `char networkInterface[] = "eth0";`

1. In `main.c`, add a call to **Networking_SetInterfaceState** before any other networking calls:

    ```c
     int err = Networking_SetInterfaceState(networkInterface, true);
     if (err == -1) {
         Log_Debug("Error setting interface state %d\n",errno);
         return -1;
     }
    ```
