---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name: Azure Sphere - Hello World
urlFragment: HelloWorld
extendedZipContent:
- path: HardwareDefinitions
  target: HelloWorld_HighLevelApp/HardwareDefinitions
- path: .clang-format
  target: .clang-format
- path: BUILD_INSTRUCTIONS.md
  target: BUILD_INSTRUCTIONS.md
- path: Samples/SECURITY.md
  target: SECURITY.md
- path: Samples/troubleshooting.md
  target: troubleshooting.md
description: "Demonstrates the basic features of Azure Sphere high-level and real-time capable applications and provides confirmation that the device is working."
---

# Samples: Hello World

The samples in this folder demonstrate the basic features of Azure Sphere high-level and real-time capable applications and provide confirmation that the Azure Sphere hardware, SDK, and other tools are properly installed and configured.

## Samples

 * [HelloWorld_HighLevelApp](HelloWorld_HighLevelApp/) - a basic high-level app.
 * [HelloWorld_RTApp_MT3620_BareMetal](HelloWorld_RTApp_MT3620_BareMetal/) - a basic real-time capable app for the MT3620 running on bare-metal.
