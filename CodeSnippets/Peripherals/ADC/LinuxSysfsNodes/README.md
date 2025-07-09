# Snippet: Linux sysfs nodes

This is a Linux code snippet which demonstrates how to read a value from an MCP3008 ADC chip which is connected to a Raspberry Pi 4 Model B using Linux sysfs nodes and displays the value in volts.

Refer to this snippet to understand the semantic differences in interacting with an analog to digital converter (ADC) peripheral when coding for Azure Sphere versus coding for a generic Linux system. It may be particularly helpful if you have a Linux background but are new to Azure Sphere.

## Prerequisites

The snippet requires the following hardware:

1. Raspberry Pi 4 Model B
   - The Raspberry Pi has no built-in ADC. Hence the MCP3008 ADC chip is connected externally to the Raspberry Pi.
   - The SPI interface must be enabled on the Raspberry Pi. It is usually disabled by default.
   - Append the following line to `/boot/config.txt` to set up the device driver for the MCP3008 ADC chip.
        `dtoverlay=mcp3008:spi0-0-present`

1. MCP3008 ADC chip
   - The MCP3008 has a 10-bit resolution, and so it can report a range of numbers from 0 to 1023 (2 to the power of 10). A reading of 0 means the input is 0V and a reading of 1023 means the input is 3.3V.

## Circuit
The following list shows how to connect the MCP3008 to the Raspberry Pi. It requires 6 GPIO pins on the Raspberry Pi P1 Header.

VDD   3.3V   (P1-01)
VREF  3.3V   (P1-01)
AGND  GROUND (P1-06)
CLK   GPIO11 (P1-23)
DOUT  GPIO9  (P1-21)
DIN   GPIO10 (P1-19)
CS    GPIO8  (P1-24)
DGND  GROUND
