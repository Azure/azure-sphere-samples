# Sample for Microsoft Azure Sphere Device REST APIs for C#

This sample shows how to use Microsoft Device Azure Sphere Device REST APIs for C#. The sample gets and lists all the attached Azure Sphere devices.

The sample uses the following Microsoft Azure Sphere Device REST APIs.

| REST API | Purpose |
|---------|---------|
| [`GetAttachedDevices`](../../README.md#get-attached-devices) | Get the list of attached devices.  |
| [`GetDeviceSecurityState`](../../README.md#get-device-security-state) | Get the device security state. |
| [`SetActiveDeviceIpAddress`](../../README.md#set-active-device-ip-address) | Set the device IP address that the REST APIs should use. |


## Contents

| File/folder                      | Description              |
|----------------------------------|--------------------------|
| DeviceAPISample.csproj           | C# project file.         |
| Program.cs                       | C# source code file.     |

## Prerequisites

This sample requires the following hardware:

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits).

## Setup

1. Ensure that your Azure Sphere device(s) are connected to your computer.
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 22.11 or above. At the command prompt, run `azsphere show-version` to check. If required, upgrade the Azure Sphere SDK for [Windows](https://learn.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://learn.microsoft.com/azure-sphere/install/install-sdk-linux).
1. Install the Microsoft Azure Sphere Device REST APIs for C# NuGet package (the package name is `Microsoft.Azure.Sphere.DeviceAPI`):
    - For Visual Studio: Use the [Visual Studio Package Manager](https://learn.microsoft.com/en-us/nuget/consume-packages/install-use-packages-visual-studio)
    - For Visual Studio Code, use the [dotnet add package](https://learn.microsoft.com/en-us/nuget/consume-packages/install-use-packages-dotnet-cli) command.

## Run the sample

To run this sample:
- For Visual Studio: Use `Debug | Start Debugging` or keyboard `F5`
- For Visual Studio Code:
  - In the terminal/console, use `dotnet restore`, `dotnet build`, `dotnet run`
  - In Visual Studio Code: use `Run | Start Debugging`

### Observe the output

You will see output similar to below:
```
Azure Sphere Device API Sample.
Gets a list of attached devices, displays the IP address and Device ID

192.168.35.2, b5da085c544efa7c01b18586aa7dab2b831f11696681e52d34cbb8c5a596376c2587860c3a05c53a392e46c57ba8826f07c3e2024c6bfb2dfc85399ee904c993

```
