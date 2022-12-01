/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
namespace Microsoft.Azure.Sphere.DeviceAPI
{
    using System.Diagnostics;
    using System.Text.Json;
    using System;
    using System.Text;
    using System.IO;

    /// <summary>
    /// Device REST APIs to configure the Wi-Fi options on the device.
    /// </summary>
    public static partial class Wifi
    {
        /// <summary>Makes a REST "POST" request to add a Wi-Fi network on an attached device.</summary>
        /// <param name="ssid">The SSID of the new network.</param>
        /// <param name="securityState">If the network is enabled or disabled.</param>
        /// <remarks>Accepted securityState values: ["eaptls", "open", "psk"]</remarks>
        /// <param name="configName">A string value (up to 16 characters) that specifies the name for this network.</param>
        /// <param name="configState">If the network is enabled or disabled.</param>
        /// <remarks>Accepted configState values: ["enabled", "disabled"]</remarks>
        /// <param name="targetScan">Attempt to connect to an SSID even if not advertised.</param>
        /// <param name="psk">The WPA/WPA2 PSK for the new network. Do not set this if connecting to an open network.</param>
        /// <param name="clientIdentity">user@domain [EAP-TLS] ID recognized for authentication by this networks RADIUS server. Required for some EAP-TLS networks.</param>
        /// <param name="clientCertStoreIdentifier">[EAP-TLS] A string value (up to 16 characters) that identifies the client certificate (containing both the public and private key). Required to set up an EAP-TLS network.</param>
        /// <param name="rootCaCertStoreIdentifier">[EAP-TLS] A string value (up to 16 characters) that identifies the networks root CA certificate for EAP-TLS networks where the device authenticates the server.</param>
        /// <returns>The details of the newly configured network as a string on success. An exception will be thrown on error.</returns>
        public static string AddWifiNetwork(string ssid, string securityState = "", string configName = "", string configState = "", bool? targetScan = null, string psk = "", string clientIdentity = "", string clientCertStoreIdentifier = "", string rootCaCertStoreIdentifier = "")
        {
            // Validate required input paramters
            if (string.IsNullOrEmpty(ssid))
            {
                throw new ValidationError("Cannot add wifi network config, invalid ssid!");
            }

            string[] validConfigState = new string[] { "enabled", "disabled" };

            if (string.IsNullOrEmpty(configState) || !Array.Exists(validConfigState, elem => elem.Equals(configState)))
            {
                throw new ValidationError("Cannot add wifi network config, invalid configState!");
            }

            string[] validSecurityState = new string[] { "eaptls", "open", "psk" };

            if (string.IsNullOrEmpty(securityState) || !Array.Exists(validSecurityState, elem => elem.Equals(securityState)))
            {
                throw new ValidationError("Cannot add wifi network config, invalid network type!");
            }

            // Build json of configuration request
            string json = string.Empty;

            if (securityState.Equals("eaptls"))
            {
                json = AddWifiNetworkEaptlsToJson(ssid, securityState, configName, configState, targetScan, clientIdentity, clientCertStoreIdentifier, rootCaCertStoreIdentifier);
            }
            else if (securityState.Equals("open"))
            {
                json = AddWifiNetworkOpenToJson(ssid, securityState, configName, configState, targetScan);
            }
            else if (securityState.Equals("psk"))
            {
                json = AddWifiNetworkPskToJson(ssid, securityState, configName, configState, targetScan, psk);
            }

            if (string.IsNullOrEmpty(json))
            {
                throw new ValidationError("Cannot add Wi-Fi network, network state to json conversion returned null or empty!");
            }

            var deserializedJson = JsonSerializer.Deserialize(json, typeof(object));

            if (deserializedJson is null)
            {
                throw new ValidationError("Cannot add Wi-Fi network, json deserilazation returned null!");
            }

            // Send request
            return RestUtils.PostRequest("wifi/config/networks", deserializedJson);
        }

        /// <summary>Creates a JSON string given a set of parameters for the PSK network type.</summary>
        /// <remarks>helper</remarks>
        /// <param name="ssid">The SSID of the new network.</param>
        /// <param name="securityState">If the network is enabled or disabled.</param>
        /// <param name="configName">A string value (up to 16 characters) that specifies the name for this network.</param>
        /// <param name="configState">If the network is enabled or disabled.</param>
        /// <param name="targetScan">Attempt to connect to an SSID even if not advertised.</param>
        /// <param name="psk">The WPA/WPA2 PSK for the new network. Do not set this if connecting to an open network.</param>
        /// <returns>The JSON string on success, or an empty string on failure.</returns>
        private static string AddWifiNetworkPskToJson(string ssid, string securityState, string configName, string configState, bool? targetScan, string psk)
        {

            if (string.IsNullOrEmpty(psk))
            {
                throw new ValidationError("Cannot add wifi network config for PSK, psk is null or empty!");
            }

            var options = new JsonWriterOptions
            {
                Indented = false
            };

            using var stream = new MemoryStream();
            using (var writer = new Utf8JsonWriter(stream, options))
            {
                writer.WriteStartObject();
                // Write items that must exist
                writer.WriteString("ssid", ssid);
                writer.WriteString("securityState", securityState);
                writer.WriteString("psk", psk);
                // write items that could exist
                if (!string.IsNullOrEmpty(configName)) { writer.WriteString("configName", configName); }
                if (!string.IsNullOrEmpty(configState)) { writer.WriteString("configState", configState); }
                if (targetScan is not null) { writer.WriteBoolean("targetScan", (bool)targetScan); }

                writer.WriteEndObject();
            }
            return Encoding.UTF8.GetString(stream.ToArray());
        }

        /// <summary>Creates a JSON string given a set of parameters for the Open network type.</summary>
        /// <remarks>helper</remarks>
        /// <param name="ssid">The SSID of the new network.</param>
        /// <param name="securityState">If the network is enabled or disabled.</param>
        /// <param name="configName">A string value (up to 16 characters) that specifies the name for this network.</param>
        /// <param name="configState">If the network is enabled or disabled.</param>
        /// <param name="targetScan">Attempt to connect to an SSID even if not advertised.</param>
        /// <returns>The JSON string on success, or an empty string on failure.</returns>
        private static string AddWifiNetworkOpenToJson(string ssid, string securityState, string configName, string configState, bool? targetScan)
        {
            var options = new JsonWriterOptions
            {
                Indented = false
            };

            using var stream = new MemoryStream();
            using (var writer = new Utf8JsonWriter(stream, options))
            {
                writer.WriteStartObject();
                // Write items that must exist
                writer.WriteString("ssid", ssid);
                writer.WriteString("securityState", securityState);

                // write items that could exist
                if (!string.IsNullOrEmpty(configName)) { writer.WriteString("configName", configName); }
                if (!string.IsNullOrEmpty(configState)) { writer.WriteString("configState", configState); }
                if (targetScan is not null) { writer.WriteBoolean("targetScan", (bool)targetScan); }

                writer.WriteEndObject();
            }
            return Encoding.UTF8.GetString(stream.ToArray());
        }

        /// <summary>Creates a JSON string given a set of parameters for the Eaptls network type.</summary>
        /// <remarks>helper</remarks>
        /// <param name="ssid">The SSID of the new network.</param>
        /// <param name="securityState">If the network is enabled or disabled.</param>
        /// <param name="configName">A string value (up to 16 characters) that specifies the name for this network.</param>
        /// <param name="configState">If the network is enabled or disabled.</param>
        /// <param name="targetScan">Attempt to connect to an SSID even if not advertised.</param>
        /// <param name="clientIdentity">user@domain [EAP-TLS] ID recognized for authentication by this networks RADIUS server. Required for some EAP-TLS networks.</param>
        /// <param name="clientCertStoreIdentifier">[EAP-TLS] A string value (up to 16 characters) that identifies the client certificate (containing both the public and private key). Required to set up an EAP-TLS network.</param>
        /// <param name="rootCaCertStoreIdentifier">[EAP-TLS] A string value (up to 16 characters) that identifies the networks root CA certificate for EAP-TLS networks where the device authenticates the server.</param>
        /// <returns>The JSON string on success, or an empty string on failure.</returns>
        private static string AddWifiNetworkEaptlsToJson(string ssid, string securityState, string configName, string configState, bool? targetScan, string clientIdentity, string clientCertStoreIdentifier, string rootCaCertStoreIdentifier)
        {
            if (string.IsNullOrEmpty(clientCertStoreIdentifier))
            {
                throw new ValidationError("Cannot add wifi network config for eaptls, clientCertStoreIdentifier is null or empty!");
            }

            var options = new JsonWriterOptions
            {
                Indented = false
            };

            using var stream = new MemoryStream();
            using (var writer = new Utf8JsonWriter(stream, options))
            {
                writer.WriteStartObject();
                // Write items that must exist
                writer.WriteString("ssid", ssid);
                writer.WriteString("securityState", securityState);
                writer.WriteString("clientCertStoreIdentifier", clientCertStoreIdentifier);

                // write items that could exist
                if (!string.IsNullOrEmpty(configName)) { writer.WriteString("configName", configName); }
                if (!string.IsNullOrEmpty(configState)) { writer.WriteString("configState", configState); }
                if (targetScan is not null) { writer.WriteBoolean("targetScan", (bool)targetScan); }
                if (!string.IsNullOrEmpty(clientIdentity)) { writer.WriteString("clientIdentity", clientIdentity); }
                if (!string.IsNullOrEmpty(rootCaCertStoreIdentifier)) { writer.WriteString("rootCaCertStoreIdentifier", rootCaCertStoreIdentifier); }

                writer.WriteEndObject();
            }
            return Encoding.UTF8.GetString(stream.ToArray());
        }

        /// <summary>Makes a "PATCH" request to modify a specific Wi-Fi network configuration from an attached device.</summary>
        /// <param name="networkID">The ID of the desired network.</param>
        /// <param name="configState">The state of the configured network.</param>
        /// <remarks>Accepted configState values: ["unknown", "enabled", "disabled", "temp-disabled"]</remarks>
        /// <param name="psk">The WPA/WPA2 PSK for the network.</param>
        /// <returns>The updated Wi-Fi network configuration on success. An exception will be thrown on error.</returns>
        public static string ChangeConfiguredWifiNetwork(int networkID, string configState, string psk)
        {
            string[] validConfigState = new string[] { "unknown", "enabled", "disabled", "temp-disabled" };

            if (string.IsNullOrEmpty(configState) || !Array.Exists(validConfigState, elem => elem.Equals(configState)))
            {
                throw new ValidationError("Cannot change Wi-Fi network config, configState is invalid!");
            }

            if (string.IsNullOrEmpty(psk))
            {
                throw new ValidationError("Cannot change Wi-Fi network config, psk is invalid!");
            }

            string response = String.Empty;
            try
            {
                response = RestUtils.PatchRequest($"wifi/config/networks/{networkID}", new { configState, psk });
            }
            catch (System.Net.Http.HttpRequestException)
            {
                throw new ValidationError("Cannot change wifi network config, psk is invalid!");
            }

            return response;
        }

        /// <summary>Makes a REST "PATCH" request that modifies the underlying Wifi hardware behaviour.</summary>
        /// <param name="reloadConfig">Reload all configuration data.</param>
        /// <param name="enablePowerSavings">Enable or disable power savings in the underlying Wifi hardware module</param>
        /// <returns>An empty response as a string on success. An exception will be thrown on error.</returns>
        public static string ChangeWiFiInterfaceState(bool reloadConfig, bool enablePowerSavings)
        {
            if (enablePowerSavings)
            {
                SinceDeviceAPIVersion.ValidateDeviceApiVersion("SetWiFiInterfacePowerSavings", "4.6.0");
            }

            return RestUtils.PatchRequest("wifi/interface", new { reloadConfig, enablePowerSavings });
        }

        /// <summary>Makes a REST "PATCH" request that modifies the underlying Wifi hardware behaviour.</summary>
        /// <param name="reloadConfig">Reload all configuration data.</param>
        /// <returns>An empty response as a string on success. An exception will be thrown on error.</returns>
        public static string SetWiFiInterfaceReloadConfiguration(bool reloadConfig) {
            return RestUtils.PatchRequest("wifi/interface", new { reloadConfig });
        }

        /// <summary>Makes a REST "PATCH" request that modifies the underlying Wifi hardware behaviour.</summary>
        /// <param name="enablePowerSavings">Enable or disable power savings in the underlying Wifi hardware module</param>
        /// <returns>An empty response as a string on success. An exception will be thrown on error.</returns>
        public static string SetWiFiInterfacePowerSavings(bool enablePowerSavings) {
            SinceDeviceAPIVersion.ValidateDeviceApiVersion("SetWiFiInterfacePowerSavings", "4.6.0");
            return RestUtils.PatchRequest("wifi/interface", new { enablePowerSavings });
        }

        /// <summary>Makes a REST "GET" request to retrieve the current Wi-Fi configurations for an attached device.</summary>
        /// <returns>The current Wi-Fi configurations as a string on success. An exception will be thrown on error.</returns>
        public static string GetAllConfiguredWifiNetworks()
        {
            return RestUtils.GetRequest("wifi/config/networks");
        }

        /// <summary>Makes a "GET" request to retrieve a specific Wi-Fi network configuration for an attached device.</summary>
        /// <param name="networkID">The ID of the desired network.</param>
        /// <returns>The Wi-Fi network configuration as a string on success. An exception will be thrown on error.</returns>
        public static string GetConfiguredWifiNetwork(int networkID)
        {
            return RestUtils.GetRequest($"wifi/config/networks/{networkID}");
        }

        /// <summary>Makes a "GET" request to retrieve the state of the wireless interface on an attached device.</summary>
        /// <returns>The status of the wireless interface as a string on success. An exception will be thrown on error.</returns>
        public static string GetWiFiInterfaceState()
        {
            return RestUtils.GetRequest("wifi/interface");
        }

        /// <summary>Makes a "GET" request to retrieve Wi-Fi networks visible to an attached device.</summary>
        /// <returns>The list of Wi-Fi networks visible to the attached device as a string on success. An exception will be thrown on error.</returns>
        public static string GetWiFiScan()
        {
            return RestUtils.GetRequest("wifi/scan");
        }

        /// <summary>Makes a "DELETE" request to delete a specific Wi-Fi network configuration from an attached device.</summary>
        /// <param name="networkID">The ID of the network.</param>
        /// <returns>An empty response as a string on success. An exception will be thrown on error.</returns>
        public static string DeleteWiFiNetConfig(int networkID)
        {
            SinceDeviceAPIVersion.ValidateDeviceApiVersion("DeleteWiFiNetConfig", "3.0.0");
            return RestUtils.DeleteRequest($"wifi/config/networks/{networkID}");
        }
    }

    /// <summary>Helper class, providing options for AddConfiguredWifiNetwork</summary>
    public static class AddConfiguredWifiNetworkSecurityStateOptions
    {
        /// <summary>eaptls option</summary>
        public static string eaptls = "eaptls";
        /// <summary>open option</summary>
        public static string open = "open";
        /// <summary>psk option</summary>
        public static string psk = "psk";
    }

    /// <summary>Helper class, providing options for AddConfiguredWifiNetwork</summary>
    public static class AddConfiguredWifiNetworkConfigStateOptions
    {
        /// <summary>enabled option</summary>
        public static string enabled = "enabled";
        /// <summary>disabled option</summary>
        public static string disabled = "disabled";
    }

    /// <summary>Helper class, providing options for ChangeConfiguredWifiNetwork</summary>
    public static class ChangeConfiguredWifiNetworkConfigStateOptions
    {
        /// <summary>unknown option</summary>
        public static string unknown = "unknown";
        /// <summary>enabled option</summary>
        public static string enabled = "enabled";
        /// <summary>disabled option</summary>
        public static string disabled = "disabled";
        /// <summary>temp-disabled option</summary>
        public static string temp_disabled = "temp-disabled";
    }
}
