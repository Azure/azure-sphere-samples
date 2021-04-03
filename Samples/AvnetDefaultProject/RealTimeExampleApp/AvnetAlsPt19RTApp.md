# Real time application information for AvnetAlsPt19RTApp

The Avnet ALS-PT19 AzureRTOS real time application reads adc data from the Avnet Starter Kit's on-board ALS-PT19 light sensor and passes telemetry data to the high level application over the inter-core communications path.

# The appliation supports the following Avnet inter-core implementation messages . . .

* IC_HEARTBEAT 
  * The application echos back the IC_HEARTBEAT response
* IC_READ_SENSOR
  * The application returns the adc voltage read from the device in the lightSensorAdcData response data field
* IC_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
  * The application reads the light sensor, converts the raw data to units of LUX and returns properly formatted JSON
  * {"light_intensity": 1234.56} 
* IC_SET_SAMPLE_RATE
* The application will read the sample rate and if non-zero, will automatically send sensor telemetry at the period specified by the command.  If set to zero, no automatic telemetry messages will be sent. 

# Sideloading the application binary
This application binary can be side loaded onto your device with the following commands . . .

* `azsphere device enable-development`
* `azsphere device sideload deploy --image-package ./AvnetAlsPt19TRApp.imagepackage`

# Configuring the Avnet Default High Level application to use this example
To configure the high level application to use this binary ...

* Add the function definition to m4_support.h

`void alsPt19RawDataHandler(void*);`

Include the interface definition in the m4_support.c 4mArray[] definition

    // The Avnet Light Sensor application reads the ALS-PT19 light sensor on the Avnet Starter Kit
    {
          .m4Name="AvnetLightSensor",
          .m4RtComponentID="b2cec904-1c60-411b-8f62-5ffe9684b8ce", 
          .m4InitHandler=genericM4Init,
          .m4rawDataHandler=alsPt19RawDataHandler,
          .m4Handler=genericM4Handler,
          .m4CleanupHandler=genericM4Cleanup,
          .m4TelemetryHandler=genericM4RequestTelemetry,
          .m4InterfaceVersion=V0
    },
   
* Update the app_manifest.json file with the real time application's ComponentID

`"AllowedApplicationConnections": [ "b2cec904-1c60-411b-8f62-5ffe9684b8ce" ],`

* Update the launch.vs.json  file with the real time application's ComponentID

`"partnerComponents": [ "b2cec904-1c60-411b-8f62-5ffe9684b8ce" ]`

* Update the .vscode\launch.json  file with the real time application's ComponentID

`"partnerComponents": [ "b2cec904-1c60-411b-8f62-5ffe9684b8ce" ]`

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
      void alsPt19RawDataHandler(void* msg){

        // Define the expected data structure.  Note this struct came from the AvnetGroveGPS real time application code
        typedef struct
        {
            INTER_CORE_CMD cmd;
            uint32_t sensorSampleRate;
            uint32_t lightSensorAdcData;
        } IC_COMMAND_BLOCK_ALS_PT19;

        IC_COMMAND_BLOCK_ALS_PT19 *messageData = (IC_COMMAND_BLOCK_ALS_PT19*) msg;
        Log_Debug("RX Raw Data: lightSensorAdcData: %d\n", messageData->lightSensorAdcData);

        // Add message structure and logic to do something with the raw data from the 
        // real time application
      }

## Application Manifest
Note that high level apps and real time apps may not declare the same resources in their app_manifest.json files.  This real time application uses the following Azure Sphere resources, and is built to connect to the AvnetDefaultProject/HighLevelExampleApp application with ComponentID: 06e116bd-e3af-4bfe-a0cb-f1530b8b91ab.

    {
        "SchemaVersion": 1,
        "Name": "Avnet-Als-PT19-RTApp",
        "ComponentId": "b2cec904-1c60-411b-8f62-5ffe9684b8ce",
        "EntryPoint": "/bin/app",
        "CmdArgs": [],
        "Capabilities": {
            "Adc": [ "ADC-CONTROLLER-0" ],
            "AllowedApplicationConnections": [ "06e116bd-e3af-4bfe-a0cb-f1530b8b91ab" ]
        },
        "ApplicationType": "RealTimeCapable"
    }

## Hardware resources claimed by this application
Note that using/declaring the ADC controller in the app_manifest.json file will also lock the following MT3620 resources to the real time application.  See the [I/O Peripherals table](https://docs.microsoft.com/en-us/azure-sphere/hardware/mt3620-product-status#io-peripherals) for details on how the MT3620 maps hardware pins to blocks.

Mapped to the ADC-CONTROLOLER-0 Hardware Block
* All ADC functions
* GPIO41 - GPIO48

## Serial Debug
By default the application opens the M4 debug port and sends debug data over that connection

    UART Settings: 115200, N, 8, 1
    VT-102 Terminal Emulation
