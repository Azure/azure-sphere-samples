# Microsoft.Azure.Sphere.DeviceAPI

Microsoft Azure Sphere Device REST APIs enables users to interact with an Azure Sphere device using REST APIs. This is useful during [manufacturing operations](https://learn.microsoft.com/en-us/azure-sphere/hardware/manufacturing-guide).

## Sample application

```c#
using Microsoft.Azure.Sphere.DeviceAPI;
internal class Program
{
    private static void Main(string[] args)
    {
        string result = Devices.GetAttachedDevices();
        Console.WriteLine($"Devices result: {result}");
        string networks = Wifi.GetAllConfiguredWifiNetworks();
        Console.WriteLine($"Wifi networks: {networks}");
    }
}
```

## Install via CLI

1. Ensure you have the .NET Runtime and SDK installed for your platform.

1. Create a folder to contain your C# project

1. `cd` to your newly created .NET project folder

1. Create a new .NET console application: `dotnet new console`

1. Install the package from nuget: `dotnet add package Microsoft.Azure.Sphere.DeviceAPI`

## Install via Visual Studio

1. Install Visual Studio

1. Follow the guide to [add a nuget package to a Visual Studio project](https://learn.microsoft.com/en-us/nuget/quickstart/install-and-use-a-package-in-visual-studio).


## API documentation

View the package API documentation [on Github](https://github.com/Azure/azure-sphere-samples/blob/main/Manufacturing/src/README.md).