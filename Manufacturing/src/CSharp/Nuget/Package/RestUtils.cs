/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma warning disable CS8632

namespace Microsoft.Azure.Sphere.DeviceAPI
{
    using System.Security.Cryptography.X509Certificates;
    using System.Net.Security;
    using System.Net;
    using System;
    using System.IO;
    using System.Reflection;
    using System.Linq;
    using System.Net.Http;
    using System.Text.Json;
    using System.Text.Json.Serialization;
    using System.Text;
    using System.Threading.Tasks;
    using System.ComponentModel;

    /// <summary>
    /// Sets up communication to device, holds common methods for interacting with it.
    /// </summary>
    public static class RestUtils
    {
        // Read an embedded resource, return byte array or new byte[0] if the resource isn't found
        static byte[] GetEmbeddedResource(string name)
        {
            Assembly asm = Assembly.GetExecutingAssembly();
            string[] resourceNames = asm.GetManifestResourceNames();

            var result = resourceNames.Where(c => c.Contains(name)).ToArray();
            if (result.Length == 0)
                return Array.Empty<byte>();

            Stream stream = asm.GetManifestResourceStream(result[0]);
            if (stream != null)
            {
                byte[] ba = new byte[stream.Length];
                stream.Read(ba, 0, ba.Length);
                return ba;
            }

            return Array.Empty<byte>();
        }

        /// <summary>
        /// Creates client with certificate.
        /// </summary>
        private static HttpClient SetupClient(string url)
        {
            //Replace this with the path to your deviceRESTApiCertificate.pem file.
            //The current implementation looks for the pem file in the current folder.
            DirectoryInfo? parentDirectory = Directory.GetParent(Environment.CurrentDirectory);

            // Check if directory exists
            if (parentDirectory == null)
            {
                return null;
            }

            string pemDirectory = parentDirectory.FullName;

            var httpClientHandler = new HttpClientHandler()
            {
                ServerCertificateCustomValidationCallback = (sender, cert, chain, sslPolicyErrors) =>
                {
                    if (sslPolicyErrors.HasFlag(SslPolicyErrors.RemoteCertificateNotAvailable))
                    {
                        return false;
                    }

                    X509Certificate2 ca = new X509Certificate2(GetEmbeddedResource("deviceRestApiCertificate.pem"));
                    X509Chain chain2 = new();
                    chain2.ChainPolicy.TrustMode = X509ChainTrustMode.CustomRootTrust;
                    chain2.ChainPolicy.CustomTrustStore.Add(ca);
                    chain2.ChainPolicy.RevocationMode = X509RevocationMode.NoCheck;
                    if (cert == null)
                    {
                        return false;
                    }
                    bool isVerified = chain2.Build(cert);
                    if (chain2.ChainStatus.Length > 0)
                        return false;

                    return isVerified;
                }
            };

            HttpClient httpClient = new(httpClientHandler) { BaseAddress = new Uri(url) };
            return httpClient;
        }

        /// <summary>
        /// Awaits a HttpRequest, catching timeout exceptions.
        /// </summary>
        private static HttpResponseMessage MakeRequest(Task<HttpResponseMessage> request)
        {
            try
            {
                return request.GetAwaiter().GetResult();
            } catch (HttpRequestException e)
            {
                // Catch timeout exception, bubble up all other exceptions.
                if (e.Message.Contains("A connection attempt failed because the connected party did not properly respond after a period of time, or established connection failed because connected host has failed to respond."))
                    throw new DeviceError(string.Format("Device connection timed out for {0}. Please ensure your device is connected and has development mode enabled", Devices.GetActiveDeviceIpAddress()));
                else
                    throw new HttpRequestException(e.Message);
            }
        }

        /// <summary>
        /// Makes a "GET" request with the given API endpoint.
        /// </summary>
        /// <param name="api"> The specific part of the endpoint that comes after the device address.</param>
        /// <param name="alternativeUrl">Override the Device IP Address, used to talk to the Device Communication Service</param>
        /// <param name="headers">If the headers or the content should be returned from the response.</param>
        /// <returns>json result from successful call, ErrorHandling.CheckResponse will throw an exception on error.</returns>
        public static string GetRequest(string api, string alternativeUrl = null, bool headers = false)
        {
            string url;
            if (!string.IsNullOrEmpty(alternativeUrl))
            {
                Uri uri = new(new Uri(alternativeUrl), api);
                url = uri.ToString();
            }
            else
            {
                url = GetDeviceUrl(Devices.GetActiveDeviceIpAddress(), api);
            }
            HttpClient client = SetupClient(url);

            if (client == null)
            {
                throw new AzureSphereException("Get Request: Client setup failed!");
            }
            return ErrorHandling.CheckResponse(MakeRequest(client.GetAsync(url)), headers);
        }

        /// <summary>
        /// Makes a "DELETE" request with the given API endpoint.
        /// </summary>
        /// <param name="api"> The specific part of the endpoint that comes after the device address.</param>
        /// <returns>json result from successful call, ErrorHandling.CheckResponse will throw an exception on error.</returns>
        public static string DeleteRequest(string api)
        {
            string url = GetDeviceUrl(Devices.GetActiveDeviceIpAddress(), api);
            HttpClient client = SetupClient(url);

            if (client == null)
            {
                throw new AzureSphereException("Delete Request: Client setup failed!");
            }

            return ErrorHandling.CheckResponse(MakeRequest(client.DeleteAsync(url)));
        }

        /// <summary>
        /// Makes a "POST" request with the given API endpoint and json body.
        /// </summary>
        /// <param name="api"> The specific part of the endpoint that comes after the device address.</param>
        /// <param name="jsonBody"> The json body to be added to the request.</param>
        /// <returns>json result from successful call, ErrorHandling.CheckResponse will throw an exception on error.</returns>
        public static string PostRequest(string api, object jsonBody)
        {
            string url = GetDeviceUrl(Devices.GetActiveDeviceIpAddress(), api);
            HttpClient client = SetupClient(url);

            if (client == null)
            {
                throw new AzureSphereException("Post Request: Client setup failed!");
            }

            string json = JsonSerializer.Serialize(jsonBody);
            var content = new StringContent(json, Encoding.UTF8, "application/json");
            return ErrorHandling.CheckResponse(MakeRequest(client.PostAsync(url, content)));
        }

        /// <summary>
        /// Makes a "POST" request with the given API endpoint.
        /// </summary>
        /// <param name="api"> The specific part of the endpoint that comes after the device address.</param>
        /// <returns>json result from successful call, ErrorHandling.CheckResponse will throw an exception on error.</returns>
        public static string PostRequestNoBody(string api)
        {
            string url = GetDeviceUrl(Devices.GetActiveDeviceIpAddress(), api);
            HttpClient client = SetupClient(url);

            if (client == null)
            {
                throw new AzureSphereException("Post Request (no body): Client setup failed!");
            }

            return ErrorHandling.CheckResponse(MakeRequest(client.PostAsync(url, null)));
        }

        /// <summary>
        /// Makes a "PATCH" request with the given API endpoint and json body.
        /// </summary>
        /// <param name="api"> The specific part of the endpoint that comes after the device address.</param>
        /// <param name="jsonBody"> The json body to be added to the request.</param>
        /// <returns>json result from successful call, ErrorHandling.CheckResponse will throw an exception on error.</returns>
        public static string PatchRequest(string api, object jsonBody)
        {
            string url = GetDeviceUrl(Devices.GetActiveDeviceIpAddress(), api);
            HttpClient client = SetupClient(url);

            if (client == null)
            {
                throw new AzureSphereException("Patch Request: Client setup failed!");
            }

            string json = JsonSerializer.Serialize(jsonBody);
            var content = new StringContent(json, Encoding.UTF8, "application/json");
            return ErrorHandling.CheckResponse(MakeRequest(client.PatchAsync(url, content)));
        }

        /// <summary>
        /// Makes a "PUT" request with the given API endpoint and json body.
        /// </summary>
        /// <param name="api"> The specific part of the endpoint that comes after the device address.</param>
        /// <param name="jsonBody"> The json body to be added to the request.</param>
        /// <returns>json result from successful call, ErrorHandling.CheckResponse will throw an exception on error.</returns>
        public static string PutRequest(string api, object jsonBody)
        {
            string url = GetDeviceUrl(Devices.GetActiveDeviceIpAddress(), api);

            HttpClient client = SetupClient(url);

            if (client == null)
            {
                throw new AzureSphereException("Put Request: Client setup failed!");
            }

            string json = JsonSerializer.Serialize(jsonBody);
            var content = new StringContent(json, Encoding.UTF8, "application/json");

            return ErrorHandling.CheckResponse(MakeRequest(client.PutAsync(url, content)));
        }

        /// <summary>
        /// Helper function generate a URL from a device IP address and path
        /// </summary>
        /// <param name="ipAddress">device IP address</param>
        /// <param name="path">REST API path</param>
        /// <returns>string containing the device REST API path</returns>
        internal static string GetDeviceUrl(string ipAddress, string path)
        {
            Uri url = new(new Uri($"https://{ipAddress}"), path);
            return url.ToString();
        }

        /// <summary>
        /// Makes a "PUT" request with the given API endpoint and image package, returning the response as a RestResponse.
        /// </summary>
        /// <param name="api"> The specific part of the endpoint that comes after the device address.</param>
        /// <param name="streamBody"> The binary octet stream body you would like to send with the request.</param>
        /// <returns>json result from successful call, ErrorHandling.CheckResponse will throw an exception on error.</returns>
        public static string PutWithOctetStreamRequest(string api, byte[] streamBody)
        {
            string url = GetDeviceUrl(Devices.GetActiveDeviceIpAddress(), api);
            HttpClient client = SetupClient(url);

            if (client == null)
            {
                throw new AzureSphereException("Put Request (Octet Stream): Client setup failed!");
            }

            StreamContent content = new(new MemoryStream(streamBody));
            content.Headers.Remove("Content-Type");
            content.Headers.Add("Content-Type", "application/octet-stream");

            return ErrorHandling.CheckResponse(MakeRequest(client.PutAsync(url, content)));
        }
    }
}
