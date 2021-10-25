# Add support to the sample for connecting via Azure IoT Edge

## Step 1. Start with the Azure IoT README topic

Follow the instructions in the [READMEStartWithIoTHub.md](./READMEStartWithIoTHub.md) file before you perform any of the steps described here.

## Step 2. Set up the IoT Edge device as a transparent gateway

Follow the steps in [Set up Azure IoT Edge for Azure Sphere](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-edge) to set up your IoT Edge device as a transparent gateway.

## Step 3. Update the sample application to connect via your IoT Edge device

By default, the application's top-level CMakeLists.txt pulls in code to connect directly to an IoT Hub. You'll need to modify this CMakeLists.txt to pull in the code to connect via IoT Edge instead, add the root CA certificate of the IoT Edge device. You'll also need to change some information in the app_manifest.json file:

1. Put the IoT Edge device's root CA certificate in the certs/ directory of the application. If you used the scripts provided in the IoT Edge git repository to create test certificates, then the root CA certificate is called *azure-iot-test-only.root.ca.cert.pem*.

1. Find the CMakeLists.txt file in your sample directory and open it.

1. Change the add_subdirectory CMake function call so that it no longer adds the code in the "IoTHub" directory and instead adds the code in the "IoTEdge" subdirectory. The call should now look like this:

   `add_subdirectory(IoTEdge)`

1. Include the root CA certificate by updating the CMake function call that creates the application image package.

   `azsphere_target_add_image_package(${PROJECT_NAME} RESOURCE_FILES "certs/<iotedgedevice_cert_name>")`

1. Save the modified CMakeLists.txt file.

1. Find the app_manifest.json file in your sample directory and open it.

1. Update the *CmdArgs* field of the app_manifest.json file.

   - Copy the DNS name of your IoT Edge device and paste it into the *CmdArgs* field of the app_manifest.json file as an argument to the Hostname option, as shown below:

      `"--Hostname", "<iotedgedevice_hostname>"`

      If using the virtual machine as your IoT Edge device, log in to the Azure portal. You will find the DNS name on the summary screen at the top right of your virtual machine.

   - Provide the root CA certificate of the IoT Edge device to the Azure Sphere device.
      - Put this certificate in the certs/ directory of the application.
      - Copy the name of the certificate and paste it into the *CmdArgs* field of the app_manifest.json file as an argument to the IoTEdgeRootCAPath option, as shown below:

         `"--IoTEdgeRootCAPath", "certs/<iotedgedevice_cert_name>"`

      If you used the scripts provided in the IoT Edge git repository to create test certificates, then the root CA certificate is called *azure-iot-test-only.root.ca.cert.pem*.

   - Each command line option must be surrounded by double quotes. Separate command line options with a comma.

   - Your *CmdArgs* field should now look like this:

        `"CmdArgs": [ "--Hostname", "<iotedgedevice_hostname>", "--IoTEdgeRootCAPath", "certs/<iotedgedevice_cert_name>" ]`

1. Update the *AllowedConnections* field of the app_manifest.json file.

   - Copy the DNS name of your IoT Edge device used in the *CmdArgs* section and append it to the *AllowedConnections* field of the app_manifest.json file:

     `"AllowedConnections": [ "<iotedgedevice_hostname>" ]`
1. Update the *DeviceAuthentication* field of the app_manifest.json file.

   - At the command prompt, run the following command to get the Tenant ID. Use the GUID, not the friendly name, and paste it into the *DeviceAuthentication* field of the app_manifest.json file:

      `azsphere tenant show-selected`

   - Your *DeviceAuthentication* field should now look like this:

      `"DeviceAuthentication": "<GUID>"`

1. Save the modified app_manifest.json file.

## Step 4. Re-build and re-run the sample

Repeat [the steps to build and run the sample](./READMEStartWithHub.md#Build-and-run-the-sample).

## Troubleshooting

For common errors and corrective action, see [Troubleshooting](./AzureIoTTroubleshooting.md).
