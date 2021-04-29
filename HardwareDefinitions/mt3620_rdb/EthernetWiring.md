# Connecting Ethernet adapters to the MT3620 development board

This section provides instructions for connecting the following Ethernet boards and modules to the Azure Sphere MT3620 board.

- [Connection instructions for the Olimex ENC28J60-H development board](#Connection-instructions-for-the-Olimex-ENC28J60-H-development-board)
- [Connection instructions for the MT3620 Ethernet Shield](#Connection-instructions-for-the-MT3620-Ethernet-Shield)

## Connection instructions for the Olimex ENC28J60-H development board

The [Olimex ENC28J60-H development board](https://www.olimex.com/Products/Modules/Ethernet/ENC28J60-H/) uses ISU port 0 (ISU0) to communicate with the MT3620 board via SPI.

Use jumper wires to make the following connections between the ENC28J60-H board and the MT3620 board.

| ENC28J60-H pin | MT3620 board pin |
| -------------- | ---------------- |
| 3V3: 10        | 3V3: Header 3 (upper right), Pin 3      |
| GND: 9         | GND: Header 2 (lower left), Pin 2       |
| CS: 7          | CSA0: Header 2, Pin 5      |
| SCK: 1         | SCLK0: Header 2, Pin 3     |
| MOSI: 2        | MOSI0: Header 2, Pin 7     |
| MISO: 3        | MISO0 RTS: Header 2, Pin 1 |
| INT: 5         | GPIO5: Header 2, Pin 4     |

Refer to the following graphic for details.

![Connection diagram for ENC28J60-H and MT3620](./Media/ENC28J60Hconnection.jpg)

## Connection instructions for the MT3620 Ethernet Shield

The [MT3620 Ethernet Shield](https://www.seeedstudio.com/MT3620-Ethernet-Shield-v1-0-p-2917.html) uses the same board configuration package, and is internally wired to the same MT3620 development board GPIO pins as the Olimex ENC28J60-H Development Board.
It also uses the same ISU port (ISU0) to communicate with the MT3620 board via SPI. However, you connect the MT3620 Ethernet Shield directly to the interface headers on the MT3620 board.

![MT3620 Ethernet Shield - Seeed Studio](./Media/MT3620EthernetShield.jpg)
