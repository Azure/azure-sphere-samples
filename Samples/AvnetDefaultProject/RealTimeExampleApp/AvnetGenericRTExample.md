# Real time application information for AvnetGenericRTExample

# The appliation supports the following Avnet inter-core implementation messages . . .

* IC_HEARTBEAT 
  * The application echos back the IC_HEARTBEAT response
* IC_READ_SENSOR
  * The application returns simulated data in the  rawData8bit and rawDatafloat response data fields
* IC_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
  * The application generates random data returns properly formatted JSON
  * {"sampleRtKeyString":"AvnetKnowsIoT", "sampleRtKeyInt":84, "sampleRtKeyFloat":16.354}
* IC_SET_SAMPLE_RATE
  * The application will read the sample rate and if non-zero, will automatically send sensor telemetry at the period specified by the command.  If set to zero, no automatic telemetry messages will be sent. 

# Sideloading the appliction binary

This application binary can be side loaded onto your device with the following commands . . .

* `azsphere device enable-development`
* `azsphere device sideload deploy --image-package ./AvnetGenericRTExample.imagepackage`

# Configuring the Avnet Default High Level applicatin to use this example

To configure the high level application to use this binary ...

Include the interface definition in the m4_support.c 4mArray[] definition

    {
        .m4Name="AvnetGenericRTApp",
        .m4RtComponentID="9f19b84b-d83c-442b-b8b8-ce095a3b9b33",
        .m4InitHandler=genericM4Init,
        .m4Handler=genericM4Handler,
        .m4CleanupHandler=genericM4Cleanup,
        .m4TelemetryHandler=genericM4RequestTelemetry,
        .m4InterfaceVersion=V0
    }
   
* Update the app_manifest.json file with the real time application's ComponentID

`"AllowedApplicationConnections": [ "9f19b84b-d83c-442b-b8b8-ce095a3b9b33" ],`

* Update the launch.vs.json  file with the real time application's ComponentID

`"partnerComponents": [ "9f19b84b-d83c-442b-b8b8-ce095a3b9b33" ]`

* Update the .vscode\launch.json  file with the real time application's ComponentID

`"partnerComponents": [ "9f19b84b-d83c-442b-b8b8-ce095a3b9b33" ]`

* Include the raw data handler in your high level application in m4_support.c

    /// <summary>
    ///  referenceRawDataHandler()
    ///
    /// This handler is called when the high level application receives a raw data read response from the
    /// AvnetGenericRT real time application.
    ///
    ///  This handler is included as a refeence for your own custom raw data handler.
    ///
    /// </summary>
    void referenceRawDataHandler(void* msg){

        // Define the expected data structure.  Note this struct came from the AvnetGroveGPS real time application code
        typedef struct
        {
            INTER_CORE_CMD cmd;
            uint32_t sensorSampleRate;
            uint8_t rawData8bit;
            float rawDataFloat; 
        } IC_COMMAND_BLOCK_GENERIC_RT_APP;

        IC_COMMAND_BLOCK_GENERIC_RT_APP *messageData = (IC_COMMAND_BLOCK_GENERIC_RT_APP*) msg;
        Log_Debug("RX Raw Data: rawData8bit: %d, rawDataFloat: %.2f\n",
              messageData->rawData8bit, messageData->rawDataFloat);

        // Add message structure and logic to do something with the raw data from the 
        // real time application
}

## Application Manifest

Note that high level apps and real time apps may not declare the same resources in their app_manifest.json files.  This real time application uses the following Azure Sphere resources, and is built to connect to the AvnetDefaultProject/HighLevelExampleApp application with ComponentID: 06e116bd-e3af-4bfe-a0cb-f1530b8b91ab.

    {
        "SchemaVersion": 1,
        "Name": "AvnetGenericRTExample",
        "ComponentId": "9f19b84b-d83c-442b-b8b8-ce095a3b9b33",
        "EntryPoint": "/bin/app",
        "CmdArgs": [],
        "Capabilities": {
            "AllowedApplicationConnections": [ "06e116bd-e3af-4bfe-a0cb-f1530b8b91ab" ]
        },
        "ApplicationType": "RealTimeCapable"
    }


## Hardware resources claimed by this application
None

## Serial Debug
By default the application opens the M4 debug port and sends debug data over that connection
UART Settings: 115200, N, 8, 1
VT200 Terminal Emulation