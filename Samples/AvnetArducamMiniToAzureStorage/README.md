## Note: This example originated from https://github.com/xiongyu0523/azure-sphere-arducam-mini-2mp-plus.  The project was updated to use CMake and modified to support the 5MP camera.

# ArduCAM mini 2MP/5MP Plus SPI camera 

This sample demonstrates capturing a JPEG picture using [ArduCAM mini 2MP PLUS]() SPI camera on Azure Sphere OS and upload the picture to Azure Blob service using libcurl to initiate [Put Blob](https://docs.microsoft.com/en-us/rest/api/storageservices/put-blob) REST API with [SAS authorization](https://docs.microsoft.com/en-us/rest/api/storageservices/delegate-access-with-shared-access-signature). 

## Configure Azure Blob and generate service SAS

1. To run this demo you need have a [storage account](https://docs.microsoft.com/en-us/azure/storage/common/storage-quickstart-create-account?tabs=azure-portal) and create a [blob container](https://docs.microsoft.com/en-us/azure/storage/blobs/storage-quickstart-blobs-portal)
2. Download [Azure Storage Explorer](https://azure.microsoft.com/en-us/features/storage-explorer/) and login with your Azure AD
3. In Storage Explorer, navigate to your container icon and right click to **Get Shared Access Signature**, create a Ad-hoc Service SAS with at least *Create* and *Write* permission. Copy **Query String**

## To build and run the sample

### Prep your device

1. Ensure that your Azure Sphere device is connected to your PC, and your PC is connected to the internet.
2. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 19.09 or above. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Download and install the [latest SDK](https://aka.ms/AzureSphereSDKDownload) as needed.
3. Open Azure Sphere Developer Command Prompt and issue the following command:

   ```
   azsphere dev edv
   ```
4. The demo is tested on RDB, Starter Kit Rev1 and Starter Kit Rev2 boards.  Connect camera as shown below. Detailed connections are:
   
    |  ArduCAM mini 2MP Plus | RDB  | Avnet SK Rev1/Rev2 |
    |  ----  | ----  | ---- | 
    | SCL  | H2-7 |  Click 1: Header 2: Pin 5 |
    | SDA  | H2-1 |  Click 1: Header 2: Pin 6 |
    | VCC  | H3-3 |  Click 1: Header 1: Pin 7 |
    | GND  | H3-2 |  Click 1: Header 1: Pin 8 |
    | SCK  | H4-7 |  Click 1: Header 1: Pin 4 | 
    | MISO  | H4-5 |  Click 1: Header 1: Pin 5 | 
    | MOSI  | H4-11 | Click 1: Header 1: Pin 6 | 
    | CS   | H1-10 |  Click 1: Header 2: Pin 1 | 
  
### Build and deploy the application

1. Start Visual Studio.
2. Open the CMakeLists.txt file
3. In main.c file, replace storageURL string with your **stroage account name** SASToken string with the **Query String** your created, and **pathFileName** with your blob container name.
4. In app_manifest.json file, fill your own **stroage account name** in AllowedConnections capability. 
5. Press **F5** to build and debug the application
6. In **Device Output** window, you will observe below logs:
   
   ```
   Remote debugging from host 192.168.35.1, port 55935
   Exmaple to capture a JPEG image from ArduCAM mini 2MP Plus and send to Azure Blob
   ArduCAM mini 2MP Plus is found

   len = 8200

   *   Trying 52.239.223.132:443...
   * Connected to <your-storage-account>.blob.core.windows.net (52.239.223.132) port 443 (#0)
   * ALPN, offering http/1.1
   * ALPN, server did not agree to a protocol
   * SSL connection using TLSv1.2 / ECDHE-RSA-AES256-GCM-SHA384
   > PUT /auduino-pictures/img/b21ff595-2dba-469a-a37f-beb68757b12d.jpg<YourSharedAccessSignature> HTTP/1.1
   Host: <your-storage-account>.blob.core.windows.net
   Accept: */*
   x-ms-blob-type:BlockBlob
   Content-Length: 8046
   Expect: 100-continue

   * Mark bundle as not supporting multiuse
   < HTTP/1.1 100 Continue
   * We are completely uploaded and fine
   * Mark bundle as not supporting multiuse
   < HTTP/1.1 201 Created
   < Content-Length: 0
   < Content-MD5: gb+b3lQnVLIIG/YPmA3Sgw==
   < Last-Modified: Tue, 17 Nov 2020 18:33:18 GMT
   < ETag: "0x8D88B27414D9FC0"
   < Server: Windows-Azure-Blob/1.0 Microsoft-HTTPAPI/2.0
   < x-ms-request-id: 60898629-601e-003c-0910-bd5470000000
   < x-ms-version: 2019-02-02
   < x-ms-content-crc64: 4goS2PiDO4c=
   < x-ms-request-server-encrypted: true
   < Date: Tue, 17 Nov 2020 18:33:18 GMT
   < 
   * Connection #0 to host <your-storage-account>.blob.core.windows.net left intact
   App Exit

   Child exited with status 0

   unt>.blob.core.windows.net left intact
   ```

7. Go to Azure Storage Explorer and double click your container, a (Random GUID).jpg* will be listed. Double click the file to open and inspect the image.
