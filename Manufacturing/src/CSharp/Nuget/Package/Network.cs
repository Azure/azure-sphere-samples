/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
namespace Microsoft.Azure.Sphere.DeviceAPI
{
    using System.Collections.Generic;
    using System;

    /// <summary>
    /// Device REST APIs to provide information about the status of network interfaces on the attached device.
    /// </summary>
    public static class Network
    {
        /// <summary>Makes a "POST" request to configure the network proxy on the attached device.</summary>
        /// <param name="enabled">Enable network proxy on the attached device.</param>
        /// <param name="address">The network address of the proxy.</param>
        /// <param name="port">The port on the network address to be used.</param>
        /// <param name="noProxyAddresses">Array of space-separated network addresses the device should avoid for proxy connection.</param>
        /// <param name="authenticationType">If the proxy requires a user name and password, set this to basic, otherwise anonymous.</param>
        /// <param name="username">This is an optional parameter for the basic authentication version. The username used for proxy authentication.</param>
        /// <param name="password">This is an optional parameter for the basic authentication version. The password used for proxy authentication.</param>
        /// <remarks>Accepted authenticationType values: ["anonymous", "basic"]</remarks>
        /// <returns>
        /// The details of the supplied proxy configuration as a string on success. An exception will be thrown on error.
        /// </returns>
        public static string ConfigureProxy(bool enabled, string address, int port, List<string> noProxyAddresses, string authenticationType, string username = "", string password = "")
        {
            if (string.IsNullOrEmpty(address))
            {
                throw new ValidationError("Cannot configure proxy, address was null or empty!");
            }

            string[] validAuthenticationTypes = new string[] { "anonymous", "basic" };

            if (string.IsNullOrEmpty(authenticationType) || !Array.Exists(validAuthenticationTypes, elem => elem.Equals(authenticationType)))
            {
                throw new ValidationError("Cannot configure proxy, authenticationType is invalid!");
            }

            string response;
            // Check if username and password given for proxy 
            if (!string.IsNullOrEmpty(username) && !string.IsNullOrEmpty(password))
            {
                if (authenticationType.Equals("anonymous"))
                {
                    throw new ValidationError("Cannot configure proxy, incorrect configuration of authentication type and proxy credentials!");
                }
                // Basic type network proxy
                var jsonBody = new { enabled, address, port, noProxyAddresses, authenticationType, username, password };
                response = RestUtils.PostRequest("net/proxy", jsonBody);
            }
            else
            {
                if (authenticationType.Equals("basic") || !string.IsNullOrEmpty(username) || !string.IsNullOrEmpty(password))
                {
                    throw new ValidationError("Cannot configure proxy, incorrect configuration of authentication type and proxy credentials!");
                }
                // Anonymous type network proxy 
                var jsonBody = new { enabled, address, port, noProxyAddresses, authenticationType };
                response = RestUtils.PostRequest("net/proxy", jsonBody);
            }
            return response;
        }

        /// <summary>Makes a "DELETE" request to delete the current network proxy for an attached device.</summary>
        /// <returns>An empty response as a string on success. An exception will be thrown on error.</returns>
        public static string DeleteNetworkProxy()
        {
            return RestUtils.DeleteRequest("net/proxy");
        }

        /// <summary>Makes a "GET" request to retrieve information about the current network proxy for an attached device.</summary>
        /// <returns>The details of the current network proxy as a string on success. An exception will be thrown on error.</returns>
        public static string GetNetworkProxy()
        {
            return RestUtils.GetRequest("net/proxy");
        }

        /// <summary>Makes a "GET" request to retrieve all the failed network connection attempts by an attached device.</summary>
        /// <returns>The failed connection attempts as a string on success. An exception will be thrown on error.</returns>
        public static string GetAllFailedNetworkConnections()
        {
            return RestUtils.GetRequest("wifi/diagnostics/networks");
        }

        /// <summary>Makes a "GET" request to retrieve the status of all network interfaces on an attached device.</summary>
        /// <returns>The status of all network interfaces as a string on success. An exception will be thrown on error.</returns>
        public static string GetAllNetworkInterfaces()
        {
            SinceDeviceAPIVersion.ValidateDeviceApiVersion("GetAllNetworkInterfaces");
            return RestUtils.GetRequest("net/interfaces");
        }

        /// <summary>Makes a "GET" request to retrieve a specific failed network connection attempt by an attached device.</summary>
        /// <param name="networkId">The id of the network you would like the failed connection attempt of.</param>
        /// <returns>The failed network connection attempt as a string on success. An exception will be thrown on error.</returns>
        public static string GetFailedNetworkConnection(string networkId)
        {
            return RestUtils.GetRequest($"wifi/diagnostics/networks/{networkId}");
        }

        /// <summary>Makes a "GET" request to retrieve all network firewall rulesets on an attached device.</summary>
        /// <returns>All network firewall rulesets as a string on success. An exception will be thrown on error.</returns>
        public static string GetNetworkFirewallRuleset()
        {
            SinceDeviceAPIVersion.ValidateDeviceApiVersion("GetNetworkFirewallRuleset");
            return RestUtils.GetRequest("net/firewall/rulesets");
        }

        /// <summary>Makes a "GET" request to retrieve the status of the named network interface on an attached device.</summary>
        /// <param name="networkInterfaceName">The name of the network interface you would like to get the status of.</param>
        /// <returns>The status of the named network interface as a string on success. An exception will be thrown on error.</returns>
        public static string GetNetworkInterface(string networkInterfaceName)
        {
            SinceDeviceAPIVersion.ValidateDeviceApiVersion("GetNetworkInterface");
            if (string.IsNullOrEmpty(networkInterfaceName))
            {
                throw new ValidationError("Cannot get network interface, input was null or empty!");
            }

            return RestUtils.GetRequest($"net/interfaces/{networkInterfaceName}");
        }

        /// <summary>Makes a "GET" request to retrieve the network status on an attached device.</summary>
        /// <returns>The status of the current network as a string on success. An exception will be thrown on error.</returns>
        public static string GetNetworkStatus()
        {
            SinceDeviceAPIVersion.ValidateDeviceApiVersion("GetNetworkStatus");
            return RestUtils.GetRequest("net/status");
        }

        /// <summary>Makes a "PATCH" request to add/update a specific network interface's attributes on an attached device.</summary>
        /// <param name="networkInterfaceName">The name of the network interface you would like to get the status of.</param>
        /// <param name="interfaceUp">If you would like the interface to be active or not.</param>
        /// <returns>An empty response as a string on success. An exception will be thrown on error.</returns>
        public static string SetNetworkInterfaces(string networkInterfaceName, bool interfaceUp)
        {
            SinceDeviceAPIVersion.ValidateDeviceApiVersion("SetNetworkInterfaces");
            if (string.IsNullOrEmpty(networkInterfaceName))
            {
                throw new ValidationError("Cannot set network interface, networkInterfaceName was null or empty!");
            }

            return RestUtils.PatchRequest($"net/interfaces/{networkInterfaceName}", new { interfaceUp });
        }
    }
    /// <summary>Helper class, providing options for ConfigureProxy</summary>
    public static class ConfigureProxyAuthenticationTypeOptions
    {
        /// <summary>anonymous option</summary>
        public static string anonymous = "anonymous";
        /// <summary>basic option</summary>
        public static string basic = "basic";
    }
}
