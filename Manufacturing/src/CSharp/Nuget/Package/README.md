# `Microsoft.Azure.Sphere.DeviceAPI` C# NuGet package

`Microsoft.Azure.Sphere.DeviceAPI` exposes a class/source file for each API category, the C# project files are listed below with a description for each of the classes.

| Filename | Description |
|----------|-------------|
| App.cs | Application management |
| Capabilities.cs | Device capabilities |
| Certificate.cs | Certificate management |
| Certs | Certificate for API to Device verification |
| Device.cs | Device state/configuration  |
| Devices.cs | Get a list of connected devices |
| ErrorHandling.cs | Helper class for error handling |
| Image.cs | Image management |
| Manufacturing.cs | Manufacturing state/configuration |
| Microsoft.Azure.Sphere.DeviceAPI.csproj | C# project file |
| Network.cs | Network Configuration |
| README.md | This file |
| RestUtils.cs |Helper class for REST APIs |
| Sideload.cs | Sideload applications |
| Validation.cs | Value validation helper class |
| Wifi.cs | Wi-Fi configuration |

### Building the NuGet Project from Visual Studio 2019/2022

Building the NuGet project is a two-step process, first open the project in Visual Studio 2019/2022.

Build the project: `Build | Rebuild Solution`
Pack the project: `Build | Pack Microsoft.Azure.Sphere.DeviceAPI`

Once the project is built the `.nupkg` file can be found in `bin\Debug` or `bin\Release`.

### Building the NuGet Project from the command line

The NuGet package can be built using `dotnet` commands.

To build the project: `dotnet restore` and `dotnet build`
to pack the project: `dotnet pack` - more information on the dotnet pack command can be found [here](https://learn.microsoft.com/dotnet/core/tools/dotnet-pack).

