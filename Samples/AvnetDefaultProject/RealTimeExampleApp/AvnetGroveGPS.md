# Real time application information for AvnetGroveGps

The Avnet Grove GPS AzureRTOS real time application reads UART data from a Grove GPS by Seeed and passes GPS telemetry data to the high level application over the inter-core communications path.

# The appliation supports the following Avnet inter-core implementation messages . . .

* IC_HEARTBEAT 
  * The application echos back the IC_HEARTBEAT response
* IC_READ_SENSOR
  * The application fills in the IC_COMMAND_BLOCK_GROVE_GPS structure with the raw data from the Grove GPS device
* IC_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
  * The application receives and parses NEMA GPS data, parses the NEMA data and returns properly formatted JSON
  * {"numSats":9,"fixQuality":2,"Tracking":{"lat":36.034810,"lon":-71.246187,"alt":-0.60}}
* IC_SET_SAMPLE_RATE
  * The application will read the sample rate and if non-zero, will automatically send sensor telemetry at the period specified by the command.  If set to zero, no automatic telemetry messages will be sent. 

# Sideloading the application binary
This application binary can be side loaded onto your device with the following commands . . .

* `azsphere device enable-development`
* `azsphere device sideload deploy --image-package ./AvnetGroveGPS.imagepackage`

# Configuring the Avnet Default High Level application to use this example
To configure the AvnetDefaultProject high level application to use this binary ...

* Add the function definition to m4_support.h

`void groveGPSRawDataHandler(void*);`

* Include the interface definition in the m4_support.c 4mArray[] definition

      // The AvnetGroveGPS app captures data from a Grove GPS V1.2 UART device
      {
       .m4Name="AvnetGroveGPS",
       .m4RtComponentID="592b46b7-5552-4c58-9163-9185f46b96aa",
       .m4InitHandler=genericM4Init,
       .m4Handler=genericM4Handler,
       .m4rawDataHandler=groveGPSRawDataHandler,
       .m4CleanupHandler=genericM4Cleanup,
       .m4TelemetryHandler=genericM4RequestTelemetry,
       .m4InterfaceVersion=V0
      }
     
* Update the high level app_manifest.json file with the real time application's ComponentID

`"AllowedApplicationConnections": [ "592b46b7-5552-4c58-9163-9185f46b96aa" ],`

* Update the high level launch.vs.json  file with the real time application's ComponentID

`"partnerComponents": [ "592b46b7-5552-4c58-9163-9185f46b96aa" ]`

* Update the high level .vscode\launch.json  file with the real time application's ComponentID

`"partnerComponents": [ "592b46b7-5552-4c58-9163-9185f46b96aa" ]`

Include the raw data handler in your high level application in m4_support.c

    /// <summary>
    ///  groveGPSRawDataHandler()
    ///
    /// This handler is called when the high level application receives a raw data read response from the
    /// AvnetGroveGPS real time application.  The handler pulls the GPS data from the response message, checks
    /// to see if the data is different from the last changed data, and if so sends up a device twin update with 
    /// the location data.
    ///
    /// </summary>
    void groveGPSRawDataHandler(void* msg){

    // Track the lat/long so we only send device twin updates with different data
        static double lastLat;
        static double lastLon;

        // Define the expected data structure.  Note this struct came from the AvnetGroveGPS real time application code
        typedef struct
        {
           INTER_CORE_CMD cmd;
           uint32_t sensorSampleRate;
           double lat;
           double lon;
           int fix_qual;
           int numsats;
           float alt;
	 } IC_COMMAND_BLOCK_GROVE_GPS;

        // Cast the message so we can index into the data to pull the GPS data out of it
        IC_COMMAND_BLOCK_GROVE_GPS *messageData = (IC_COMMAND_BLOCK_GROVE_GPS*) msg;
        Log_Debug("RX Raw Data: fix_qual: %d, numstats: %d, lat: %lf, lon: %lf, alt: %.2f\n",
                      messageData->fix_qual, messageData->numsats, messageData->lat, messageData->lon, messageData->alt);
        
    #ifdef OLED_SD1306
        // Update the global GPS variables
        lat = messageData->lat;
        lon = messageData->lon;
        fix_qual = messageData->fix_qual;
        numsats = messageData->numsats;
        alt = messageData->alt;
    #endif 

    #ifdef IOT_HUB_APPLICATION    
        //Check to see if the lat or lon have changed.  If so, update the last* values and send
        // the new data to the IoTHub as device twin update
        if((lastLat != messageData->lat) && (lastLon != messageData->lon)){
    
            // Define the JSON structure
            static const char gpsDataJsonString[] = "{\"DeviceLocation\":{\"lat\": %.5f,\"lon\": %.5f,\"alt\": %.2f}}";

            size_t twinBufferSize = sizeof(gpsDataJsonString)+48;
            char *pjsonBuffer = (char *)malloc(twinBufferSize);
            if (pjsonBuffer == NULL) {
                Log_Debug("ERROR: not enough memory to report GPS location data.");
    	    }

            // Build out the JSON and send it as a device twin update
	    snprintf(pjsonBuffer, twinBufferSize, gpsDataJsonString, messageData->lat, messageData->lon, messageData->alt );
	    Log_Debug("[MCU] Updating device twin: %s\n", pjsonBuffer);
            TwinReportState(pjsonBuffer);
	    free(pjsonBuffer);
        }
    #endif         
    }

## Application Manifest
Note that high level apps and real time apps may not declare the same resources in their app_manifest.json files.  This real time application uses the following Azure Sphere resources, and is built to connect to the AvnetDefaultProject/HighLevelExampleApp application with ComponentID: 06e116bd-e3af-4bfe-a0cb-f1530b8b91ab.

    {
        "SchemaVersion": 1,
        "Name": "AvnetGroveGPSV1",
        "ComponentId": "592b46b7-5552-4c58-9163-9185f46b96aa",
        "EntryPoint": "/bin/app",
        "CmdArgs": [],
        "Capabilities": {
            "Uart": [ "ISU0" ],
            "AllowedApplicationConnections": [ "06e116bd-e3af-4bfe-a0cb-f1530b8b91ab" ]
        },
       "ApplicationType": "RealTimeCapable"
    }

## Hardware resources claimed by this application
Note that using/declaring the ADC controller in the app_manifest.json file will also lock the following MT3620 resources to the real time application.  See the [I/O Peripherals table](https://docs.microsoft.com/en-us/azure-sphere/hardware/mt3620-product-status#io-peripherals) for details on how the MT3620 maps hardware pins to blocks.

Mapped to the ISU0 Hardware Block
* ISU0 I2C functionality
* ISU0 SPI functionality
* ISU0 UART functionality
* GPIO26 - GPIO30

## Serial Debug
By default the application opens the M4 debug port and sends debug data over that connection

    UART Settings: 115200, N, 8, 1
    VT-102 Terminal Emulation
