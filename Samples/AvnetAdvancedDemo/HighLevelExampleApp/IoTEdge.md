# Run the sample with IoT Edge

**IMPORTANT**: Follow the instructions in the [README.md](./README.md) file before you perform any of the steps described here.

This sample connects to an IoT Edge device acting as a transparent gateway.

## Before you begin

Before you run the sample with IoT Edge, you must:

- Create an Azure IoT Hub and an Azure IoT Edge device.
- Configure the IoT Edge device as a transparent gateway.
- Set up your Azure Sphere device in IoT hub and authenticate it.
- Configure the sample application with information about your Azure Sphere tenant and your IoT Edge device.

You can then build the application and, if you want to, use a device twin to support additional features. For more information about device twins, see [Understand and use device twins in IoT hub](https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-device-twins).

## Set up the IoT Edge device as a transparent gateway, Azure IoT Hub and Azure Sphere device in Azure IoT Hub

Follow the steps in [Set up Azure IoT Edge for Azure Sphere](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-edge) to set up your IoT Edge device as a transparent gateway, as well as an Azure IoT Hub, and your Azure Sphere device in Azure IoT Hub. When given the option, follow the Quickstart instructions.

## Configure the sample application to work with your IoT Edge device

### Modify the app manifest file of the sample

To configure the sample to connect to an IoT Edge device, you will need the following information:

- The Tenant ID for your Azure Sphere device.
- The Device ID of your Azure Sphere device.
- The Azure IoT Hub server hostname for your IoT hub.
- The connection type to use when connecting to your IoT Edge device.

Follow these steps to gather the information and configure the application:

1. Find the app_manifest.json file in your sample directory and open it.

1. Update the **CmdArgs** field of the app_manifest.json file:

   - To configure the sample to connect to your IoT Edge device, copy and paste the following line into the **CmdArgs** field of the app_manifest.json file:

      `"--ConnectionType", "IoTEdge"`

   - Copy the DNS name of your IoT Edge device and paste it into the **CmdArgs** field of the app_manifest.json file as an argument to the Hostname option, as shown below:

      `"--Hostname", "<iotedgedevice_hostname>"`

      If using the virtual machine as your IoT Edge device, log in to the Azure portal. You will find the DNS name on the summary screen at the top right of your virtual machine.

   - Provide the root CA certificate of IoT Edge device to the Azure Sphere device.
      - Put the trusted root CA certificate of the IoT Edge device in the certs/ folder.
      - Copy the name of the certificate and paste it into the **CmdArgs** field of the app_manifest.json file as an argument to the IoTEdgeRootCAPath option, as shown below:

         `"--IoTEdgeRootCAPath", "certs/<iotedgedevice_cert_name>"`

      If you used the scripts provided in the IoT Edge git repository to create test certificates, then the root CA certificate is called *azure-iot-test-only.root.ca.cert.pem*

   - Each command line option must be surrounded by double quotes. Separate command line options with a comma.

   - Your **CmdArgs** field should now look like this:

        `"CmdArgs": [ "--ConnectionType", "IoTEdge", "--Hostname", "<iotedgedevice_hostname>", "--IoTEdgeRootCAPath", "certs/<iotedgedevice_cert_name>" ]`

1. Update the **AllowedConnections** field of the app_manifest.json file.

   - Copy the DNS name of your IoT Edge device used in the **CmdArgs** section and append it to the **AllowedConnections** field of the app_manifest.json file.

   - Each connection must be surrounded by double quotes. Separate connections with a comma.

   - Your **AllowedConnections** field should now look like this:

     `"AllowedConnections": [ "<iot_edge_device>" ]`

1. Update the **DeviceAuthentication** field of the app_manifest.json file.

   - At the command prompt, run the following command to get the Tenant ID. Use the GUID, not the friendly name, and paste it into the **DeviceAuthentication** field of the app_manifest.json file:

      `azsphere tenant show-selected`

   - Your **DeviceAuthentication** field should now look like this:

     `"DeviceAuthentication": "<GUID>"`

1. Save the modified app_manifest.json file.

### Modify the CMakeLists.txt file of the sample

To include the root CA certificate of your IoT Edge device in the application package, update the

 `azsphere_target_add_image_package` in CMakeLists.txt as shown below:

   `azsphere_target_add_image_package(${PROJECT_NAME} RESOURCE_FILES "certs/<iotedgedevice_cert_name>")`

## Build and run the sample

1. Ensure that your device is connected to the internet.

1. Follow the steps in [Build a sample application](../../BUILD_INSTRUCTIONS.md).

1. See [Troubleshooting samples](../troubleshooting.md) if you encounter errors.

1. After a short delay, the sample app will display output messages like:

   **INFO: Azure IoT Hub send telemetry: { "Temperature": "33.85" }**

1. Press button A on the MT3620 development board to send a button-press notification to the IoT Hub. The sample app will display an output message indicating a button-press:

    **Sending Azure IoT Hub telemetry: { "ButtonPress": "True" }**

## Edit device twin to change properties

You can now edit the device twin to change properties. For example, follow these steps to turn LED 1 on or off by changing a property in the device twin:

1. Log in to the [Azure Portal](https://portal.azure.com) and select your IoT Hub resource.

1. On the left-side menu under **Explorers**, select **IoT Devices**, and then double-click the Device ID for your device.

1. On the **Device Details** page, select **Device Twin**.

1. In the **properties** field, under **"desired"**, add `"StatusLED": true,` as shown below:

   ```json
      "properties": {
         "desired": {
            "StatusLED": true,
            "$metadata": {
            "$lastUpdated": "2019-01-30T22:18:19.612025Z",
   ```

1. Click **Save** to update the twin and notify the application.
In a few seconds, the LED will light up red.

## Troubleshooting

1. The following message in device output indicates a connection error with the IoT Edge device:

    ```
    Azure IoT connection status: IOTHUB_CLIENT_CONNECTION_NO_NETWORK
    Azure IoT connection status: IOTHUB_CLIENT_CONNECTION_NO_NETWORK
    Azure IoT connection status: IOTHUB_CLIENT_CONNECTION_NO_NETWORK
    Azure IoT connection status: IOTHUB_CLIENT_CONNECTION_NO_NETWORK
    ```

   This error may occur if:

   - Inbound port 8883 has not been opened on the IoT Edge device. For more information, see [Open IoT Edge gateway device ports for communication](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-edge#step-3-open-iot-edge-gateway-device-ports-for-communication).
   - The sample is using the incorrect/invalid IoT Edge device root CA certificate.
   - The **edgeAgent** and **edgeHub** modules are not running on the IoT Edge device.

1. The following message in device output indicates a device provisioning error:

    ```
    Azure IoT connection status: IOTHUB_CLIENT_CONNECTION_NO_NETWORK
    Azure IoT connection status: IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL
    Azure IoT connection status: IOTHUB_CLIENT_CONNECTION_NO_NETWORK
    Azure IoT connection status: IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL
    ```

   This error may occur if:

   - The Azure Sphere device is not manually provisioned. For corrective action, follow the steps to [authenticate using a direct connection](https://docs.microsoft.com/azure-sphere/app-development/setup-iot-hub#authenticate-using-a-direct-connection).
