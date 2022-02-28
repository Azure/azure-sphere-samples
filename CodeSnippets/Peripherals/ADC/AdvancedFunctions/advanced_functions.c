/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
   
// Code Snippet: Read ADC channel using Advanced Functions (Linux ioctls)

// This code snippet demonstrates how to read a value from the ADC 
// Potentiometer controller using ioctl system calls, and displays the 
// value in volts.

// To read a value from an ADC channel, the application manifest
// (https://docs.microsoft.com/azure-sphere/app-development/app-manifest) 
// must enable the peripheral. To enable this capability, copy the
// lines in the Capabilities section of AdvancedFunctions/app_manifest.json 
// into your application manifest file.

#include <string.h>

#include <applibs/adc.h>
#include <applibs/log.h>

static int OpenAdc(ADC_ControllerId adcControllerId);
static int GetSampleBitCount(int adcControllerFd, ADC_ChannelId channelId,
                             struct iio_ioctl_chan_spec_buffer *channelSpecBuffer);
static int GetPropertyIndex(const struct iio_ioctl_chan_spec *channelSpec,
                            const char *propertyName);
static int GetExtInfo(int adcControllerFd, ADC_ChannelId channelId,
                      unsigned int extendedPropertyIndex, char *data, size_t length);
static int SetReferenceVoltage(int adcControllerFd, ADC_ChannelId channelId,
                               struct iio_ioctl_chan_spec_buffer *channelSpecBuffer,
                               float referenceVoltage);
static int PollAdc(int adcControllerFd, ADC_ChannelId channelId, uint32_t *outSampleValue);
static int SetExtInfo(int adcControllerFd, ADC_ChannelId channelId,
                      unsigned int extendedPropertyIndex, const char *data, size_t length);
static int GetChannelSpecification(int adcControllerFd, ADC_ChannelId channelId,
                                   struct iio_ioctl_chan_spec_buffer *channelSpecBuffer);
static int ReadAdcChannel(void);

// The maximum voltage.
static const float sampleMaxVoltage = 2.5f;
// ADC device path.
static const char adcPath[] = "/dev/adc";
// Channel specification.
static struct iio_ioctl_chan_spec_buffer channelSpecBuffer;

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
    adcControllerFd = OpenAdc(SAMPLE_POTENTIOMETER_ADC_CONTROLLER);
    if (adcControllerFd == -1) {
        Log_Debug("ERROR: OpenAdc failed.\n");
        goto cleanup;
    }

    // Get the specification for the channel.
    int result = GetChannelSpecification(adcControllerFd, SAMPLE_POTENTIOMETER_ADC_CHANNEL,
                                         &channelSpecBuffer);
    if (result == -1) {
        Log_Debug("ERROR: GetChannelSpecification failed.\n");
        goto cleanup;
    }

    // Get the number of bits in the sample.
    sampleBitCount =
        GetSampleBitCount(adcControllerFd, SAMPLE_POTENTIOMETER_ADC_CHANNEL, &channelSpecBuffer);
    if (sampleBitCount == -1) {
        Log_Debug("ERROR: GetSampleBitCount failed.\n");
        goto cleanup;
    }

    // Set the reference voltage.
    result = SetReferenceVoltage(adcControllerFd, SAMPLE_POTENTIOMETER_ADC_CHANNEL,
                                 &channelSpecBuffer, sampleMaxVoltage);
    if (result == -1) {
        Log_Debug("ERROR: SetReferenceVoltage failed.\n");
        goto cleanup;
    }

    // Poll the ADC to read the voltage.
    uint32_t value;
    result = PollAdc(adcControllerFd, SAMPLE_POTENTIOMETER_ADC_CHANNEL, &value);
    if (result == -1) {
        Log_Debug("ERROR: PollAdc failed.\n");
        goto cleanup;
    }

    // Calculate voltage.
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

/// <summary>
/// Helper function to open the ADC.
/// </summary>
/// <param name="adcControllerId">Id of the ADC controller.</param>
/// <returns>Value of fd if succesful, -1 on error</returns>
static int OpenAdc(ADC_ControllerId adcControllerId)
{
    // Format the ADC path.
    char path[32];
    int len = snprintf(path, sizeof(path), "%s%u", adcPath, adcControllerId);
    if (len < 0 || len >= sizeof(path)) {
        Log_Debug("ERROR: Cannot format ADC path to buffer.\n");
        return -1;
    }

    int fd = open(path, O_RDWR | O_CLOEXEC, 0);
    if (fd == -1) {
        if (errno == ENOENT) {
            Log_Debug("ERROR: open failed with error: %s (%d)\n", strerror(errno), errno);
        }
        return -1;
    }

    return fd;
}

/// <summary>
/// Helper function to gets the specification of the channel and stores the specification
/// in channelSpecBuffer parameter.
/// </summary>
/// <param name="adcControllerFd">The ADC file descriptor.</param>
/// <param name="channelId">The channel id of the ADC.</param>
/// <param name="channelSpecBuffer">The channel spec buffer.</param>
/// <returns>0 if succesful, -1 on error</returns>
static int GetChannelSpecification(int adcControllerFd, ADC_ChannelId channelId,
                                   struct iio_ioctl_chan_spec_buffer *channelSpecBuffer)
{
    struct iio_ioctl_chan_spec_buffer_size channelSpecBufferSize = {
        .size = sizeof(channelSpecBufferSize), .index = (int)channelId, .total_size = 0};

    int ret = ioctl(adcControllerFd, (int)IIO_GET_CHANNEL_SPEC_BUFFER_TOTAL_SIZE_IOCTL,
                    &channelSpecBufferSize);
    if (ret < 0) {
        Log_Debug(
            "ERROR: ioctl call failed with error \"%s (%d)\" for request "
            "IIO_GET_CHANNEL_SPEC_BUFFER_TOTAL_SIZE_IOCTL.\n",
            strerror(errno), errno);
        return -1;
    }

    channelSpecBuffer->size = sizeof(*channelSpecBuffer);
    channelSpecBuffer->total_size = channelSpecBufferSize.total_size;
    channelSpecBuffer->index = (int)channelId;
    channelSpecBuffer->channel = NULL;

    ret = ioctl(adcControllerFd, (int)IIO_GET_CHANNEL_SPEC_BUFFER_IOCTL, channelSpecBuffer);
    if (ret < 0) {
        Log_Debug(
            "ERROR: ioctl call failed with error \"%s (%d)\" for request "
            "IIO_GET_CHANNEL_SPEC_BUFFER_IOCTL.\n",
            strerror(errno), errno);
        return -1;
    }

    return 0;
}

/// <summary>
/// Helper function to get the index of the specified property name.
/// </summary>
/// <param name="channelSpec">Pointer to channel specification struct</param>
/// <param name="propertyName">Name of the property</param>
/// <returns>Index of the property if succesful, -1 on error</returns>
static int GetPropertyIndex(const struct iio_ioctl_chan_spec *channelSpec, const char *propertyName)
{
    if (!channelSpec) {
        Log_Debug("ERROR: Channel specification input parameter is NULL.\n");
        return -1;
    }

    int propertyIndex = 0;
    const struct iio_ioctl_chan_spec_ext_info *extendedChannelSpecInfo = channelSpec->ext_info;
    while (extendedChannelSpecInfo) {
        if (extendedChannelSpecInfo->name &&
            strcmp(propertyName, extendedChannelSpecInfo->name) == 0) {
            return propertyIndex;
        }
        ++propertyIndex;
        extendedChannelSpecInfo = extendedChannelSpecInfo->next;
    }

    Log_Debug("ERROR: Failed to retrieve property index for property name %s.\n", propertyName);
    return -1;
}

/// <summary>
/// Helper function to get the extended channel information.
/// </summary>
/// <param name="adcControllerFd">The ADC file descriptor.</param>
/// <param name="propertyName">The channel id of the ADC.</param>
/// <param name="extendedPropertyIndex">The index of the property.</param>
/// <param name="data">Buffer to hold the extended information.</param>
/// <param name="length">Length of the buffer.</param>
/// <returns>0 if succesful, -1 on error</returns>
static int GetExtInfo(int adcControllerFd, ADC_ChannelId channelId,
                      unsigned int extendedPropertyIndex, char *data, size_t length)
{
    struct iio_ioctl_read_chan_ext_info readExtendedChannelInfo = {
        .size = sizeof(readExtendedChannelInfo),
        .channel_index = channelId,
        .info_index = extendedPropertyIndex,
        .buffer = data,
        .length = length};

    int ret =
        ioctl(adcControllerFd, (int)IIO_READ_CHANNEL_EXT_INFO_IOCTL, &readExtendedChannelInfo);
    if (ret < 0) {
        Log_Debug(
            "ERROR: ioctl call failed with error \"%s (%d)\" for request "
            "IIO_READ_CHANNEL_EXT_INFO_IOCTL.\n",
            strerror(errno), errno);
        return -1;
    }

    return 0;
}

/// <summary>
///  Helper function to set the extended channel information.
/// </summary>
/// <param name="adcControllerFd">The ADC file descriptor.</param>
/// <param name="propertyName">The channel id of the ADC.</param>
/// <param name="extendedPropertyIndex">The index of the property.</param>
/// <param name="data">Buffer holding the extended information to be set.</param>
/// <param name="length">Length of the buffer.</param>
/// <returns>0 if succesful, -1 on error</returns>
static int SetExtInfo(int adcControllerFd, ADC_ChannelId channelId,
                      unsigned int extendedPropertyIndex, const char *data, size_t length)
{
    if (extendedPropertyIndex < 0) {
        Log_Debug("ERROR: SetExtInfo: Invalid extended property index.\n");
        return -1;
    }

    struct iio_ioctl_write_chan_ext_info writeExtendedChannelInfo = {
        .size = sizeof(writeExtendedChannelInfo),
        .channel_index = channelId,
        .info_index = extendedPropertyIndex,
        .buffer = data,
        .length = length};

    int ret =
        ioctl(adcControllerFd, (int)IIO_WRITE_CHANNEL_EXT_INFO_IOCTL, &writeExtendedChannelInfo);
    if (ret < 0) {
        Log_Debug(
            "ERROR: ioctl call failed with error \"%s (%d)\" for request "
            "IIO_WRITE_CHANNEL_EXT_INFO_IOCTL.\n",
            strerror(errno), errno);
        return -1;
    }

    return 0;
}

/// <summary>
/// Helper function to get the number of bits in the sample.
/// </summary>
/// <param name="adcControllerFd">The ADC file descriptor.</param>
/// <param name="propertyName">The channel id of the ADC.</param>
/// <param name="iio_ioctl_chan_spec_buffer">The channel spec buffer.</param>
/// <returns>Number of bits if succesful, -1 on error</returns>
static int GetSampleBitCount(int adcControllerFd, ADC_ChannelId channelId,
                             struct iio_ioctl_chan_spec_buffer *channelSpecBuffer)
{
    // Buffer to hold decimal representation of 4-byte integer value and null terminator.
    char dataBuffer[12];

    int res = GetExtInfo(adcControllerFd, channelId,
                         (unsigned int)GetPropertyIndex(channelSpecBuffer->channel, "current_bits"),
                         dataBuffer, sizeof(dataBuffer));

    if (res < 0) {
        Log_Debug("ERROR: Failed to retrieve extended channel information for channelId = %u.\n",
                  channelId);
        return -1;
    }

    if (sizeof(dataBuffer) <= strnlen(dataBuffer, sizeof(dataBuffer))) {
        Log_Debug("ERROR: Extended channel information received is invalid for channelId = %d.\n",
                  channelId);
        return -1;
    }

    int numberOfBits = atoi(dataBuffer);

    return numberOfBits;
}

/// <summary>
/// Helper function to set the reference voltage.
/// </summary>
/// <param name="adcControllerFd">The ADC file descriptor.</param>
/// <param name="propertyName">The channel id of the ADC.</param>
/// <param name="channelSpecBuffer">The channel spec buffer.</param>
/// <param name="referenceVoltage">The reference voltage to set.</param>
/// <returns>0 if succesful, -1 on error</returns>
static int SetReferenceVoltage(int adcControllerFd, ADC_ChannelId channelId,
                               struct iio_ioctl_chan_spec_buffer *channelSpecBuffer,
                               float referenceVoltage)
{
    // Buffer to hold single precision floating point and null terminator.
    char dataBuffer[12];

    int length = snprintf(dataBuffer, sizeof(dataBuffer), "%.3f", referenceVoltage);
    if (length < 0 || length >= sizeof(dataBuffer)) {
        Log_Debug("ERROR: Cannot write reference voltage to buffer.\n");
        return -1;
    }

    int res =
        SetExtInfo(adcControllerFd, channelId,
                   (unsigned int)GetPropertyIndex(channelSpecBuffer->channel, "reference_voltage"),
                   dataBuffer, (size_t)(length + 1));
    if (res < 0) {
        Log_Debug("ERROR: Failed to set extended channel information for channelId = %u.\n",
                  channelId);
        return -1;
    }

    return 0;
}

/// <summary>
/// Helper function to poll the ADC.
/// </summary>
/// <param name="adcControllerFd">The ADC file descriptor.</param>
/// <param name="propertyName">The channel id of the ADC.</param>
/// <param name="outSampleValue">Holds the sampled value.</param>
/// <returns>0 if succesful, -1 on error</returns>
static int PollAdc(int adcControllerFd, ADC_ChannelId channelId, uint32_t *outSampleValue)
{
    if (adcControllerFd < 0) {
        Log_Debug("ERROR: Invalid file descriptor.\n");
        return -1;
    }

    struct iio_ioctl_raw_channel_info rawChannelInfo = {
        .size = sizeof(rawChannelInfo), .index = (int)(channelId), .mask = IIO_IOCTL_CHAN_INFO_RAW};

    int result = ioctl(adcControllerFd, (int)IIO_READ_RAW_CHANNEL_INFO_IOCTL, &rawChannelInfo);
    if (result < 0) {
        Log_Debug(
            "ERROR: ioctl call failed with error \"%s (%d)\" for request "
            "IIO_READ_RAW_CHANNEL_INFO_IOCTL.\n",
            strerror(errno), errno);
        return -1;
    }

    *outSampleValue = (uint32_t)(rawChannelInfo.val);
    return 0;
}