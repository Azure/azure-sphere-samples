#include "arducam.h"
#include "exit_codes.h"

// Define details needed to access the Azure Storage Account
const char *storageURL = "<your storage account>";
const char *pathFileName = "</your blob storage path";
char fileName[64];
const char *SASToken = "<your SAS key";

// Define the image buffer data structure
struct image_buffer {
    uint8_t *p_data;
    uint32_t size;
};

ExitCode arduCamInit(int csGPIO, int spiISU, int i2cISU)
{
    ExitCode returnVal = ExitCode_Success;

    // init hardware and probe camera
    returnVal = arducam_ll_init(csGPIO, spiISU, i2cISU);
    if ( returnVal != ExitCode_Success) {
        return returnVal;
    }
    arducam_reset();
    
    if (arducam_test() == 0) {
#ifdef USE_OV2640
        Log_Debug("ArduCAM 2640 mini 2MP Plus is found\r\n");
#else
        Log_Debug("ArduCAM 5624 mini 5MP Plus is found\r\n");
#endif

    } else {
#ifdef USE_OV2640
        Log_Debug("ArduCAM 2640 mini 2MP Plus NOT found\r\n");
#else
        Log_Debug("ArduCAM 5624 mini 5MP Plus NOT found\r\n");
#endif
        return ExitCode_Arducam_Not_Found;
    }

    // config Camera

#ifdef CFG_MODE_JPEG
    arducam_set_format(JPEG);
#elif defined(CFG_MODE_BITMAP)
    arducam_set_format(BMP);
#endif
    arducam_InitCAM();
#ifdef CFG_MODE_JPEG

#ifdef USE_OV2640
    arducam_OV2640_set_JPEG_size(OV2640_1600x1200);
#else // USE_OV5642

/*
    Resolution Options
    #define OV5642_320x240 0   // 320x240
    #define OV5642_640x480 1   // 640x480
    #define OV5642_1024x768 2  // 1024x768
    #define OV5642_1280x960 3  // 1280x960  // runs out of memory sometimes
    #define OV5642_1600x1200 4 // 1600x1200 // runs out of memory
    #define OV5642_2048x1536 5 // 2048x1536 // runs out of memory
    #define OV5642_2592x1944 6 // 2592x1944 // runs out of memory

*/
    arducam_OV5642_set_JPEG_size(OV5642_1024x768);

#endif
#endif
    delay_ms(1000);
    arducam_clear_fifo_flag();
    arducam_flush_fifo();
    return returnVal;
}

// Generate a random GUID and return it in outputGUID
void generateGUID(char *outputGUID)
{
    srand((unsigned int)clock());
    #define GUID_SIZE 40
    char GUID[GUID_SIZE];
    int t = 0;
    char *szTemp = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";
    char *szHex = "0123456789abcdef-";
    int nLen = (int)strlen(szTemp);

    for (t = 0; t < nLen + 1; t++) {
        int r = rand() % 16;
        char c = ' ';

        switch (szTemp[t]) {
        case 'x': {
            c = szHex[r];
        } break;
        case 'y': {
            c = szHex[(r & 0x03) | 0x08];
        } break;
        case '-': {
            c = '-';
        } break;
        case '4': {
            c = '4';
        } break;
        }
        GUID[t] = (t < nLen) ? c : 0x00;
    }

    // Move the GUID into the output string
    strncpy(outputGUID, GUID, GUID_SIZE);
}

static void LogCurlError(const char *message, int curlErrCode)
{
    Log_Debug(message);
    Log_Debug(" (curl err=%d, '%s')\n", curlErrCode, curl_easy_strerror(curlErrCode));
}

static size_t read_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    struct image_buffer *p_image_buffer = (struct image_buffer *)userdata;

    size_t total_available_size = size * nitems;
    size_t copy_size = 0;

    if (p_image_buffer->size > total_available_size) {
        copy_size = total_available_size;
        p_image_buffer->size -= total_available_size;
    } else {
        copy_size = p_image_buffer->size;
        p_image_buffer->size = 0;
    }

    for (size_t i = 0; i < copy_size; i++) {
        buffer[i] = *p_image_buffer->p_data++;
    }

    return copy_size;
}

uint32_t CaptureImage(void)
{
    // Trigger a capture and wait for data ready in DRAM
    arducam_start_capture();
    while (!arducam_check_fifo_done())
        ;

    return arducam_read_fifo_length();
}    


void UploadFileToAzureBlob(uint32_t img_len)
{

	uint8_t *p_imgBuffer = malloc(img_len);
    arducam_CS_LOW();
    arducam_set_fifo_burst();
    // BW: Add code here to send chunks of very large images
    arducam_read_fifo_burst(p_imgBuffer, img_len);
    arducam_CS_HIGH();
    arducam_clear_fifo_flag();

#if defined(CFG_MODE_JPEG)
    // OV2640 pad 0x00 bytes at the end of JPG image
    while (p_imgBuffer[img_len - 1] != 0xD9) {
        --img_len;
    }

#elif defined(CFG_MODE_BITMAP)
    // OV2640 pad 8 bytes '0x00' at the end of raw RGB image
    img_len -= 8;
#endif


#if defined(CFG_MODE_BITMAP)

    uint32_t file_size;
    // https://docs.microsoft.com/en-us/previous-versions/dd183376(v=vs.85)

    file_size = BMPIMAGEOFFSET + img_len;
    p_file = calloc(file_size, 1);

    memcpy(&p_file[0], &bmp_header[0], BMPIMAGEOFFSET);

    uint8_t midbuf = 0;
    for (uint32_t i = 0; i < img_len; i += 2) {
        midbuf = p_imgBuffer[i];
        p_imgBuffer[i] = p_imgBuffer[i + 1];
        p_imgBuffer[i + 1] = midbuf;
    }

    memcpy(&p_file[BMPIMAGEOFFSET], p_imgBuffer, img_len);

    free(p_imgBuffer);

#endif

    static struct image_buffer userdata;
    userdata.p_data = p_imgBuffer;
    userdata.size = img_len;

    CURL *curlHandle = NULL;
    CURLcode res = CURLE_OK;
    struct curl_slist *list = NULL;
    char *rootca = NULL;

    if ((res = curl_global_init(CURL_GLOBAL_ALL)) != CURLE_OK) {
        LogCurlError("curl_global_init", res);
        goto exitLabel;
    }

    // Generate a new GUID to use as the filename
    generateGUID(fileName);

    // Construct the url that includes the base url + file path  + file name + SAS Token
    char *sasurl = calloc(strlen(storageURL) + strlen(pathFileName) + sizeof(fileName) +
                              strlen(SASToken) + sizeof('\0'),
                          sizeof(char));
    (void)strcat(
        strcat(strcat(strcat(strcat(sasurl, storageURL), pathFileName), fileName), FILE_EXTENSION),
        SASToken);

    // Initialize the curl handle
    if ((curlHandle = curl_easy_init()) == NULL) {
        Log_Debug("curl_easy_init() failed\r\n");
        goto cleanupLabel;
    }

    // Set the URL
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_URL, sasurl)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_URL", res);
        goto cleanupLabel;
    }

    // Set the default value: strict certificate ON
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYPEER, 1L)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_URL", res);
        goto cleanupLabel;
    }

    // Set the blob type header
    list = curl_slist_append(list, "x-ms-blob-type:BlockBlob");
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, list)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_HTTPHEADER", res);
        goto cleanupLabel;
    }

    rootca = Storage_GetAbsolutePathInImagePackage("certs/BaltimoreCyberTrustRoot.pem");
    if (rootca == NULL) {
        Log_Debug("The root ca path could not be resolved: errno=%d (%s)\r\n", errno,
                  strerror(errno));
        goto cleanupLabel;
    }

    // Set the root ca option
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_CAINFO, rootca)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_CAINFO", res);
        goto cleanupLabel;
    }

    // Set the upload option
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_UPLOAD, 1)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_UPLOAD", res);
        goto cleanupLabel;
    }

    // Pass the size of the file
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_INFILESIZE, userdata.size)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_INFILESIZE", res);
        goto cleanupLabel;
    }

    // Set the read callback
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_READFUNCTION, read_callback)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_READFUNCTION", res);
        goto cleanupLabel;
    }

    // Pass a pointer to the data to upload
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_READDATA, &userdata)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_READFUNCTION", res);
        goto cleanupLabel;
    }

    // Set output level to verbose.
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_VERBOSE, 1L)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_VERBOSE", res);
        goto cleanupLabel;
    }

    // Turn on the LED to show we're sending data to Azure Storage
    GPIO_SetValue(senbLedGpioFd, GPIO_Value_Low);


    // Perform the opeartion
    if ((res = curl_easy_perform(curlHandle)) != CURLE_OK) {
        LogCurlError("curl_easy_perform", res);
    }

cleanupLabel: // This point cleans up all resources

    // Clean up sample's cURL resources.
    curl_easy_cleanup(curlHandle);

    // Clean up cURL library's resources.
    curl_global_cleanup();


exitLabel: // This point cleans up all resources except for the curl resources

    // Turn on the LED to show we're sending data to Azure Storage
    GPIO_SetValue(senbLedGpioFd, GPIO_Value_High);


    // Free up memory we allocated
    free(sasurl);
    free(rootca);
    free(p_imgBuffer);
    return;
}
