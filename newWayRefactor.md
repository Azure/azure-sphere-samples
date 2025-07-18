# Azure Sphere CMake Project Setup & Build Workflow

## Overview
This document outlines the proven workflow for setting up and building Azure Sphere CMake projects in Visual Studio 2022, specifically tested with the HelloWorld_HighLevelApp sample.

## Prerequisites
- Visual Studio 2022 with Azure Sphere development tools
- Azure Sphere SDK 25.04 or later
- CMake 3.20+ (Current workspace: CMake 3.31.6-msvc6 with Ninja generator)
- Azure Sphere device connected and enabled for development

## Successful Build Workflow

### Step 1: Open Project Directory
1. Launch Visual Studio 2022
2. Select __File > Open > Folder__
3. Navigate to and open the specific app directory (e.g., `Samples\HelloWorld\HelloWorld_HighLevelApp\`)
   - **Important**: Open the individual app folder, not the parent samples directory

### Step 2: Handle CMake Configuration Issues
When CMake configuration fails (common with Azure Sphere projects):

1. **Delete CMake Cache and Reconfigure**:
   - Go to __Project > Delete Cache and Reconfigure__
   - OR use __Project > CMake Cache > Delete Cache for <ProjectName>__
   - Wait for reconfiguration to complete

### Step 3: Clean Build Environment
1. **Clean All**:
   - Go to __Build > Clean All__
   - Wait for clean operation to complete

### Step 4: Rebuild Project
1. **Rebuild All**:
   - Go to __Build > Rebuild All__
   - Monitor __Output__ window for build progress
   - Ensure build completes successfully without errors

### Step 5: Deploy and Test
1. **Deploy to Device**:
   - Press __F5__ to build, deploy, and debug
   - OR use __Debug > Start Debugging__
   - Verify application runs on Azure Sphere device

## Common Issues and Solutions

### CMake Configuration Failures
- **Symptom**: CMake fails to configure on initial project open
- **Solution**: Always perform "Delete Cache and Reconfigure" first

### Hardware Definition Path Issues
- **Symptom**: Cannot find hardware definition files
- **Solution**: Verify `TARGET_HARDWARE` setting in CMakeLists.txt matches available hardware definitions

### Build Cache Issues
- **Symptom**: Build errors persist after code changes
- **Solution**: Follow the complete Clean → Rebuild workflow

## Project Configuration Notes

### Current Configuration (HelloWorld_HighLevelApp)
- **SDK Version**: 25.04
- **API Set**: 17
- **Target Hardware**: avnet_mt3620_sk
- **CMake Version**: 3.31.6-msvc6
- **Generator**: Ninja

### Hardware Definition Paths
The project searches for hardware definitions in:
1. `HardwareDefinitions/${TARGET_HARDWARE}/`
2. `../../../HardwareDefinitions/${TARGET_HARDWARE}/`

## Verification Checklist

- [ ] Project directory opened correctly in Visual Studio
- [ ] CMake cache deleted and reconfigured successfully
- [ ] Clean All completed without errors
- [ ] Rebuild All completed successfully
- [ ] Application deploys to Azure Sphere device
- [ ] Application runs and produces expected output (LED blinking for HelloWorld)

## Notes
- This workflow has been tested and verified with the HelloWorld_HighLevelApp sample
- The same process should work for other Azure Sphere CMake projects
- Always ensure Azure Sphere device is connected and development-enabled before deployment

## Quick Reference Commands
| Action | Menu Path | Shortcut |
|--------|-----------|----------|
| Open Folder | __File > Open > Folder__ | |
| Delete CMake Cache | __Project > Delete Cache and Reconfigure__ | |
| Clean All | __Build > Clean All__ | |
| Rebuild All | __Build > Rebuild All__ | |
| Start Debugging | __Debug > Start Debugging__ | __F5__ |