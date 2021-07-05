# Ethernet setup instructions

The Ethernet interface is enabled by default in the operating system, but some preparation is necessary before an application can use it.

The following steps describe how to set up a sample or tutorial to use Ethernet:

1. Add an Ethernet adapter to your hardware. If you are using an MT3620 RDB, see the [wiring instructions](HardwareDefinitions/mt3620_rdb/EthernetWiring.md).
1. Package a board configuration image for the Olimex ENC28J60-H. Follow the instructions in [Create a board configuration image package](https://docs.microsoft.com/azure-sphere/network/connect-ethernet#create-a-board-configuration-image-package).
1. Deploy the board configuration image package in addition to your application image package. Follow the instructions in [Sideload a board configuration image package](https://docs.microsoft.com/azure-sphere/network/connect-ethernet#sideload-a-board-configuration-image-package).

1. Ensure that the global constant **networkInterface** is set to "eth0". Search for the following declaration in source code. It is typically in `main.c` but may be in a different file for some samples.

    `char networkInterface[] = "wlan0";`

    Replace `wlan0` with `eth0` so that you now have the following declaration:

    `char networkInterface[] = "eth0";`
