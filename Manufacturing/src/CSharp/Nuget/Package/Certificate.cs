/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
namespace Microsoft.Azure.Sphere.DeviceAPI
{
    using System.IO;
    using System;

    /// <summary>
    /// Device REST APIs to manage certificates on the attached device. 
    /// </summary>
    public static class Certificate
    {
        /// <summary>Makes a "POST" request to add a certificate to an attached device certificate store.</summary>
        /// <param name="certificateID">The ID of the certificate.</param>
        /// <param name="certificatePath">The path of your certificate file.</param>
        /// <param name="certType">The type of certificate to add.</param>
        /// <param name="privateKeyLocation">The location of your private key file.</param>
        /// <param name="password">The password to decrypt the private key.</param>
        /// <remarks>Accepted certType values: ["client", "rootca"]</remarks>
        /// <returns>An empty response a string on success. An exception will be thrown on error.</returns>
        public static string AddCertificate(string certificateID, string certificatePath, string certType, string privateKeyLocation = "", string password = "")
        {
            if (string.IsNullOrEmpty(certificatePath))
            {
                throw new ValidationError("Cannot add certificate, certificate location is null or empty.");
            }

            if (!File.Exists(certificatePath))
            {
                throw new ValidationError("Cannot add certificate, file doesn't exist or cannot open.");
            }

            if (string.IsNullOrEmpty(certificateID))
            {
                throw new ValidationError("Cannot add certificate, certificate ID is null or empty.");
            }

            string[] validCertTypes = new string[] { "client", "rootca" };

            if (string.IsNullOrEmpty(certType) || !Array.Exists(validCertTypes, elem => elem.Equals(certType)))
            {
                throw new ValidationError("Cannot add certificate, certificate type is invalid.");
            }

            string publicCert = File.ReadAllText(certificatePath);
            string response = String.Empty;
            string urlWithComponent = $"certstore/certs/{certificateID}";

            if (certType.Equals("client"))
            {

                if (string.IsNullOrEmpty(privateKeyLocation))
                {
                    throw new ValidationError("Cannot add certificate, private key location is null or empty.");
                }

                if (string.IsNullOrEmpty(password))
                {
                    throw new ValidationError("Cannot add certificate, password is null or empty.");
                }

                string privateKey = File.ReadAllText(privateKeyLocation);
                response = RestUtils.PostRequest(urlWithComponent, new { certType, publicCert, privateKey, password });
            }
            else
            {
                response = RestUtils.PostRequest(urlWithComponent, new { certType, publicCert });
            }
            return response;
        }

        /// <summary>Makes a REST "GET" request to retrieve a list of certificates from an attached device certificate store.</summary>
        /// <returns>The identifiers of the attached certificates as a string on success. An exception will be thrown on error.</returns>
        public static string GetAllCertificates()
        {
            return RestUtils.GetRequest("certstore/certs");
        }

        /// <summary>Makes a REST "GET" request to retrieve information about a specific certificate from an attached device.</summary>
        /// <param name="certificateID"> The ID of the certificate.</param>
        /// <returns>Details of the certificate as a string on success. An exception will be thrown on error.</returns>
        public static string GetCertificate(string certificateID)
        {
            if (string.IsNullOrEmpty(certificateID))
            {
                throw new ValidationError("Cannot Remove Certificate, certificate ID is null or empty.");
            }

            return RestUtils.GetRequest($"certstore/certs/{certificateID}");
        }

        /// <summary>Makes a REST "GET" request to retrieve the available free space for an attached device certificate store.</summary>
        /// <returns>The amount of free space for certificates as a string on success. An exception will be thrown on error.</returns>
        public static string GetCertificateSpace()
        {
            return RestUtils.GetRequest("certstore/space");
        }

        /// <summary>Makes a "DELETE" request to delete a certificate in an attached device certificate store.</summary>
        /// <param name="certificateID">The ID of the certificate.</param>
        /// <returns>An empty response as a string on success. An exception will be thrown on error.</returns>
        public static string RemoveCertificate(string certificateID)
        {
            if (string.IsNullOrEmpty(certificateID))
            {
                throw new ValidationError("Cannot remove certificate, certificate ID is null or empty.");
            }
            return RestUtils.DeleteRequest($"certstore/certs/{certificateID}");
        }
    }

    /// <summary>Helper class, providing options for AddCertificate</summary>
    public static class AddCertificateCertTypeOptions
    {
        /// <summary>client option</summary>
        public static string client = "client";
        /// <summary>rootca option</summary>
        public static string rootca = "rootca";
    }
}
