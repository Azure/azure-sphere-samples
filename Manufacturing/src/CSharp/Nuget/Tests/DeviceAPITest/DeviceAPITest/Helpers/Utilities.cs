/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

using Newtonsoft.Json;
using Microsoft.Azure.Sphere.DeviceAPI;
using Newtonsoft.Json.Linq;
using System.Diagnostics;

namespace TestDeviceRestAPI.Helpers
{
    /// <summary>
    /// A helper class with commonly used methods and values.
    /// </summary>
    internal class Utilities
    {

        /// <summary>Path to AzureSphereBlinkTest.imagepackage sample application.<summary>
        public static readonly string pathToBlinkImage = GetBlinkPath();

        /// <summary>Path to MutableStorage.imagepackage sample application.<summary>
        public static readonly string pathToMutableStorage = GetMutableStoragePath();

        /// <summary>Component id of the MutableStorage.imagepackage sample application.<summary>
        public static readonly string mutableStorageId = "ae4714aa-03aa-492b-9663-962f966a9cc3";

        /// <summary>Component id of the AzureSphereBlinkTest.imagepackage sample application.<summary>
        public static readonly string blinkComponentId = "2f074b0f-d99d-4692-82d9-ef93a7d6463c";

        /// <summary>A randomly generated component id.<summary>
        public static readonly string randomUUID = "6770fc06-e7db-11ec-8fea-0242ac120002";

        /// <summary>Path to test client certificate pem file.<summary>
        public static readonly string pathToTestClientCert = GetTestClientCert();
        /// <summary>Path to test client private key pem file.<summary>
        public static readonly string pathToTestClientPrivateKey = GetTestClientPrivateKey();
        /// <summary>Path to test root certificate pem file.<summary>
        public static readonly string pathToTestRootCert = GetTestRootCert();

        /// <summary>Gets the path to the mutable storage image package.<summary>
        /// <returns>The path to the image as a string.</returns>
        private static string GetMutableStoragePath()
        {
            return GetFileFromHelpers("Images", "MutableStorage.imagepackage");
        }

        /// <summary>Gets the path to the blink image package.<summary>
        /// <returns>The path to the image as a string.</returns>
        private static string GetBlinkPath()
        {
            return GetFileFromHelpers("Images", "AzureSphereBlinkTest.imagepackage");
        }

        /// <summary>Gets the path to the test client certificate.<summary>
        /// <returns>The path to the certificate as a string.</returns>
        private static string GetTestClientCert()
        {
            return GetFileFromHelpers("Certificates", "TestClientCert.pem");
        }

        /// <summary>Gets the path to the test client private key certificate.<summary>
        /// <returns>The path to the certificate as a string.</returns>
        private static string GetTestClientPrivateKey()
        {
            return GetFileFromHelpers("Certificates", "TestClientPrivateKey.pem");
        }

        /// <summary>Gets the path to the test root certificate.<summary>
        /// <returns>The path to the certificate as a string.</returns>
        private static string GetTestRootCert()
        {
            return GetFileFromHelpers("Certificates", "TestRootCert.pem");
        }

        /// <summary>Gets a file location in a folder.<summary>
        /// <returns>The path to the file as a string.</returns>
        private static string GetFileFromHelpers(string folder, string fileName)
        {
            DirectoryInfo parentDirectory =
                Directory.GetParent(Environment.CurrentDirectory).Parent.Parent;

            // Check if directory exists
            if (parentDirectory == null)
            {
                throw new InvalidOperationException("Project layout is unexpected.");
            }

            string pemDirectory = parentDirectory.FullName;
            return Path.Combine(pemDirectory, $"Helpers\\{folder}\\{fileName}");
        }

        /// <summary>Removes all applications except gdbserver.<summary>
        public static void CleanImages()
        {
            int timeout = 5000;
            int currentTime = 0;
            string installResponse = string.Empty;
            while (currentTime < timeout)
            {
                try
                {
                    installResponse = Sideload.InstallImages();
                    break;
                }
                catch (HttpRequestException)
                {
                    Thread.Sleep(500);
                    currentTime += 500;
                }
            }

            if (string.IsNullOrEmpty(installResponse))
            {
                throw new InvalidOperationException("Cannot clean images, installing images failed.");
            }

            List<Dictionary<string, object>> images = GetImageComponents();

            // Remove all images that are not applications
            images.RemoveAll(elem => (long?)elem["image_type"] != 10);

            DeleteImagesWithoutName(images, "gdbserver");
        }

        /// <summary>Removes the added certificates.<summary>
        public static void CleanCertificates()
        {
            List<string> certs = GetCertIds();

            foreach (string cert in certs)
            {
                Certificate.RemoveCertificate(cert);
            }
        }

        /// <summary>Gets all certificate ids.<summary>
        /// <returns>The ids of all certs as strings in a list.</returns>
        public static List<string> GetCertIds()
        {
            string response = Certificate.GetAllCertificates();
            return JsonConvert.DeserializeObject<Dictionary<string, List<string>>>(response)["identifiers"];
        }

        /// <summary>Delets all applications from a list without the given name.<summary>
        /// <param name="images">The list to delete from.</param>
        /// <param name="name">The name of the image to keep.</param>
        private static void DeleteImagesWithoutName(
    List<Dictionary<string, object>> images, string name)
        {
            images.ForEach(delegate (Dictionary<string, object> image)
            {
                string componentName = (string)image["name"];
                if (componentName == null)
                {
                    throw new InvalidOperationException(
                        "Cannot delete images, component doesnt have a name.");
                }
                if (!componentName.Equals(name))
                {
                    string uid = (string)image["uid"];
                    if (uid == null)
                    {
                        throw new KeyNotFoundException(
                            "Cannot clean images, dictionary returned component with no UID.");
                    }

                    string response = Sideload.DeleteImage(uid);

                    Debug.WriteLine($"Deleted image {uid}");
                }
            });
        }

        /// <summary>Gets all images on a device.<summary>
        /// <returns>The list of images.</returns>
        public static List<Dictionary<string, object>> GetImageComponents()
        {

            string response;
            try
            {
                response = Image.GetImages();
            }
            catch (HttpRequestException)
            {
                return GetImageComponents();
            }



            Dictionary<string, object> json = JsonConvert.DeserializeObject<Dictionary<string, object>>(response);

            if (json == null)
            {
                throw new InvalidOperationException("Cannot get image components, deserialization of json failed.");
            }

            JArray components = (JArray)json["components"];
            return components.ToObject<List<Dictionary<string, object>>>() ?? new List<Dictionary<string, object>>();
        }

        /// <summary>Removes all added wifi networks.<summary>
        public static void CleanWifiNetworks()
        {
            string response;
            try
            {
                response = Wifi.GetAllConfiguredWifiNetworks();
            }
            catch (HttpRequestException)
            {
                Network.SetNetworkInterfaces("wlan0", true);
                response = Wifi.GetAllConfiguredWifiNetworks();
            }

            Dictionary<string, JArray> values =
                JsonConvert.DeserializeObject<Dictionary<string, JArray>>(response);

            foreach (JObject wifiConfig in values["values"])
            {
                Wifi.DeleteWiFiNetConfig((int)wifiConfig["id"]);
            }
        }
    }
}
