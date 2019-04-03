# Sample: GPIO

This application does the following:

- Provides access to one of the LEDs on the MT3620 development board using GPIO
- Uses a button to change the blink rate of the LED

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [GPIO](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages button A and LED 1 on the device |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the Visual Studio Device Output window during debugging |

## Prerequisites

The sample requires the following hardware:

- Azure Sphere MT3620 development board

## To build and run the sample

1. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 19.02 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Download and install the [latest SDK](https://aka.ms/AzureSphereSDKDownload) as needed.
1. Connect your Azure Sphere device to your PC by USB.
1. Enable [application development](https://docs.microsoft.com/azure-sphere/quickstarts/qs-blink-application#prepare-your-device-for-development-and-debugging), if you have not already done so:

   `azsphere device prep-debug`
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples/) repo and find the GPIO sample.
1. In Visual Studio, open GPIO.sln and press F5 to build and load the application onto the device for debugging. LED1 on the MT3620 begins blinking red. Push button A to cycle the blink interval between three possible values.

### Troubleshooting the Azure Sphere app

If an error similar to the following appears in the Visual Studio Build output when you build the Azure Sphere app, you probably have an outdated version of the Azure Sphere SDK:

   `mt3620_rdb.h:9:10: fatal error: soc/mt3620_i2cs.h: No such file or directory`
