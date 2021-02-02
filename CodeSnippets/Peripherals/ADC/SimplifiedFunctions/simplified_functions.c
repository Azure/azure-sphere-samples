// Code Snippet: Read ADC channel using Azure Sphere simplified functions

// This code snippet demonstrates how to read a value from the ADC 
// Potentiometer controller using Azure Sphere simplified functions and 
// displays the value in volts.

// To read a value from an ADC channel, the application manifest
// (https://docs.microsoft.com/azure-sphere/app-development/app-manifest) 
// must enable the peripheral. Copy the lines in the Capabilities section 
// of SimplifiedFunctions/app_manifest.json into your application manifest file.

#include <applibs/adc.h>
#include <applibs/log.h>

// The maximum voltage.
static const float sampleMaxVoltage = 2.5f;

/// <summary>
/// Reads the value from the ADC channel and displays the value in volts.
/// </summary>
/// <returns>0 on success, else -1.</returns>
static int ReadAdcChannel(void)
{
    int adcControllerFd = -1;
    int sampleBitCount = -1;
    int retVal = -1;

    // Open the ADC.
    adcControllerFd = ADC_Open(SAMPLE_POTENTIOMETER_ADC_CONTROLLER);
    if (adcControllerFd == -1) {
        Log_Debug("ERROR: ADC_Open failed with error: %s (%d)\n", strerror(errno), errno);
        goto cleanup;
    }

    // Get the number of bits in the sample.
    sampleBitCount = ADC_GetSampleBitCount(adcControllerFd, SAMPLE_POTENTIOMETER_ADC_CHANNEL);
    if (sampleBitCount == -1) {
        Log_Debug("ERROR: ADC_GetSampleBitCount failed with error : %s (%d)\n", strerror(errno),
                  errno);
        goto cleanup;
    }

    // Set the reference voltage.
    int result = ADC_SetReferenceVoltage(adcControllerFd, SAMPLE_POTENTIOMETER_ADC_CHANNEL,
                                         sampleMaxVoltage);
    if (result == -1) {
        Log_Debug("ERROR: ADC_SetReferenceVoltage failed with error : %s (%d)\n", strerror(errno),
                  errno);
        goto cleanup;
    }

    // Poll the ADC to read the voltage.
    uint32_t value;
    result = ADC_Poll(adcControllerFd, SAMPLE_POTENTIOMETER_ADC_CHANNEL, &value);
    if (result == -1) {
        Log_Debug("ERROR: ADC_Poll failed with error: %s (%d)\n", strerror(errno), errno);
        goto cleanup;
    }

    // Calculate voltage
    float maxSample = (float)((1 << sampleBitCount) - 1);
    float voltage = ((float)value * sampleMaxVoltage) / maxSample;
    Log_Debug("The out sample value is %.3f V.\n", voltage);
    retVal = 0;

cleanup:
    // Close the file descriptor.
    if (adcControllerFd != -1) {
        int result = close(adcControllerFd);
        if (result != 0) {
            Log_Debug("ERROR: Could not close ADC fd: %s (%d).\n", strerror(errno), errno);
        }
    }

    return retVal;
}