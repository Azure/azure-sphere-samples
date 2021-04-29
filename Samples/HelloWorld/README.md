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

The following samples are included:

- [HelloWorld_HighLevelApp](HelloWorld_HighLevelApp/) &mdash; A basic high-level application.
- [HelloWorld_RTApp_MT3620_BareMetal](HelloWorld_RTApp_MT3620_BareMetal/) &mdash; A basic real-time capable application (RTApp) for the MT3620 running on bare-metal.

## Contents

| File/folder                         | Description |
|-------------------------------------|-------------|
| `README.md`                         | This README file. |
| `HelloWorld_HighLevelApp`           | Folder containing the configuration files, source code files, hardware definitions, and other files needed for the high-level application. |
| `HelloWorld_RTApp_MT3620_BareMetal` | Folder containing the configuration files, source code files, and other files needed for the real-time capable application (RTApp). |

## Prerequisites

- **High-level application**: See the [high-level app prerequisites](HelloWorld_HighLevelApp/README.md#prerequisites).
- **Real-time capable application**: See the [RTApp prerequisites](HelloWorld_RTApp_MT3620_BareMetal/README.md#prerequisites).

## Setup

- **High-level application**: See the [high-level app setup](HelloWorld_HighLevelApp/README.md#setup).
- **Real-time capable application**: See the [RTApp setup](HelloWorld_RTApp_MT3620_BareMetal/README.md#setup).

## Build and run the sample

- **High-level application**: See the [high-level app build instructions](HelloWorld_HighLevelApp/README.md#build-and-run-the-sample).
- **Real-time capable application**: See the [RTApp build instructions](HelloWorld_RTApp_MT3620_BareMetal/README.md#build-and-run-the-sample).

## Next steps

- For an overview of Azure Sphere, see [What is Azure Sphere](https://docs.microsoft.com/azure-sphere/product-overview/what-is-azure-sphere).
- To learn more about Azure Sphere application development, see [Overview of Azure Sphere applications](https://docs.microsoft.com/azure-sphere/app-development/applications-overview).
