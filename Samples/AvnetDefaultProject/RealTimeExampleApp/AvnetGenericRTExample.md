# Real time application information for AvnetGenericRTExample

This application binary can be side loaded onto your device with the following commands . . .

* `azsphere device enable-development`
* `azsphere device sideload deploy --image-package ./AvnetGenericRTExample.imagepackage`

To configure the high level application to use this binary ...

Include the interface definition in the m4_support.c 4mArray[] definition

`{
     .m4Name="AvnetGenericRTApp",
     .m4RtComponentID="9f19b84b-d83c-442b-b8b8-ce095a3b9b33",
     .m4InitHandler=genericM4Init,
     .m4Handler=genericM4Handler,
     .m4CleanupHandler=genericM4Cleanup,
     .m4TelemetryHandler=genericM4RequestTelemetry,
     .m4InterfaceVersion=V0
   }`
   
* Update the app_manifest.json file with the real time application's ComponentID

`"AllowedApplicationConnections": [ "9f19b84b-d83c-442b-b8b8-ce095a3b9b33" ],`

* Update the launch.vs.json  file with the real time application's ComponentID

`"partnerComponents": [ "9f19b84b-d83c-442b-b8b8-ce095a3b9b33" ]`

* Update the .vscode\launch.json  file with the real time application's ComponentID

`"partnerComponents": [ "9f19b84b-d83c-442b-b8b8-ce095a3b9b33" ]`

Note that high level apps and real time apps may not declare the same resources in the app_manifest.json file.  This real time application uses the following Azure Sphere resources, and is build to connect to the AvnetDefaultProject/HighLevelExampleApp application.

`{
  "SchemaVersion": 1,
  "Name": "AvnetGenericRTExample",
  "ComponentId": "9f19b84b-d83c-442b-b8b8-ce095a3b9b33",
  "EntryPoint": "/bin/app",
  "CmdArgs": [],
  "Capabilities": {
    "AllowedApplicationConnections": [ "06e116bd-e3af-4bfe-a0cb-f1530b8b91ab" ]
  },
  "ApplicationType": "RealTimeCapable"
}`
