// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol
{
    using System;
    using Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol.Contracts;

    internal static class MessageProtocolFactory
    {
        private readonly static byte[] Preamble = { 0x22, 0xB5, 0x58, 0xB9 };

        public static byte[] CreateEventMessage(CategoryIdType categoryId, ushort wifiEventType)
        {
            /* Event format:
             * 
             *  | Offset Bytes |    0     |    1     |    2     |    3     |
             *  |       0      |                  Preamble                 |
             *  |       4      |        Length       | Msg Type | Reserved |
             *  |       8      |      Category ID    |      Event ID       |
             *  
             * Length           : UINT16 (LSB) - the message length excluding the first 6 bytes.
             */

            byte[] message = new byte[12];

            ByteArrayHelper.WriteBytes(Preamble, message);
            ByteArrayHelper.WriteLsbUInt16(6, message, 4); // Length
            message[6] = (byte)MessageType.Event;
            ByteArrayHelper.WriteLsbUInt16((ushort)categoryId, message, 8);
            ByteArrayHelper.WriteLsbUInt16((ushort)wifiEventType, message, 10);

            return message;
        }

        public static byte[] CreateResponseMessage(CategoryIdType categoryId, ushort requestType, uint sequenceId, byte errorCode, ResponseBase response = null)
        {
            // Handle response object if provided...
            byte[] payload = new byte[0];
            if (response != null)
            {
                payload = response.GetPayload();
            }

            /* Response format:
             * 
             *  | Offset Bytes |    0     |    1     |    2     |    3     |
             *  |       0      |                  Preamble                 |
             *  |       4      |        Length       | Msg Type | Reserved |
             *  |       8      |      Category ID    |      Request ID     |
             *  |      12      |   Sequence Number   | Result   | Reserved |
             *  |      16      |               <Response Data>             |
             *  |     ...      |                    ...                    |
             *  
             * Length           : UINT16 (LSB) - the message length excluding the first 6 bytes.
             *  
             * Sequence number  : UINT16 (LSB) - must be the same as the sequence number of the
             *                    request this is answering.
             * 
             * Result           : 0x00 if successful, > 0x00 as an error code if failed
             */

            byte[] message = new byte[16 + payload.Length];

            ByteArrayHelper.WriteBytes(Preamble, message);
            ByteArrayHelper.WriteLsbUInt16((ushort)(10 + payload.Length), message, 4);
            message[6] = (byte)MessageType.Response;
            ByteArrayHelper.WriteLsbUInt16((ushort)categoryId, message, 8);
            ByteArrayHelper.WriteLsbUInt16(requestType, message, 10);
            ByteArrayHelper.WriteLsbUInt16((ushort)sequenceId, message, 12);
            message[14] = errorCode;
            ByteArrayHelper.WriteBytes(payload, message, 16);

            return message;
        }

        public static RequestBase ReadRequestMessagePayload(byte[] message)
        {
            if (message.Length < 16)
            {
                throw new ArgumentOutOfRangeException(nameof(message), $"Message should be at least 16 bytes, not {message.Length}.");
            }

            /* Request format:
             * 
             *  | Offset Bytes |    0     |    1     |    2     |    3     |
             *  |       0      |                  Preamble                 |
             *  |       4      |        Length       | Msg Type | Reserved |
             *  |       8      |      Category ID    |      Request ID     |
             *  |      12      |   Sequence Number   |       Reserved      |
             *  |      16      |               <Request Data>              |
             *  |     ...      |                    ...                    |
             *  
             * Length           : UINT16 (LSB) - the message length excluding the first 6 bytes.
             *  
             * Sequence number  : UINT16 (LSB) - must be used for the response to this request.
             */

            uint length = ByteArrayHelper.ReadLsbUInt16(message, 4);
            CategoryIdType categoryId = (CategoryIdType)ByteArrayHelper.ReadLsbUInt16(message, 8);
            ushort requestType = (ushort)ByteArrayHelper.ReadLsbUInt16(message, 10);
            uint sequenceId = ByteArrayHelper.ReadLsbUInt16(message, 12);

            byte[] payload = null;
            uint payloadLength = length - 10;

            if (payloadLength > 0)
            {
                payload = ByteArrayHelper.ReadBytes(message, 16, payloadLength);
            }

            return ExtractRequestPayload(categoryId, requestType, sequenceId, payload);
        }

        private static RequestBase ExtractRequestPayload(CategoryIdType categoryId, ushort requestType, uint sequenceId, byte[] payload)
        {
            switch (categoryId)
            {
                case CategoryIdType.DeviceControl:
                    DeviceControlRequestId deviceControlRequestId = (DeviceControlRequestId)requestType;
                    switch (deviceControlRequestId)
                    {
                        case DeviceControlRequestId.GetDesiredLedStatus:
                            return new DeviceControlGetDesiredLedStatusRequest(deviceControlRequestId, sequenceId);

                        case DeviceControlRequestId.ReportLedStatus:
                            return new DeviceControlReportLedStatusRequest(deviceControlRequestId, sequenceId, payload);

                        default:
                            throw new InvalidOperationException($"Unknown response payload type: {(WifiRequestId)requestType}");
                    }
                case CategoryIdType.WifiControl:
                    WifiRequestId wifiRequestId = (WifiRequestId)requestType;
                    switch (wifiRequestId)
                    {
                        case WifiRequestId.SetWifiStatus:
                            return new WifiStatusRequest(wifiRequestId, sequenceId, payload);

                        case WifiRequestId.SetWifiScanResultsSummary:
                            return new WifiScanSummaryRequest(wifiRequestId, sequenceId, payload);

                        case WifiRequestId.SetNextWifiScanResult:
                            return new WifiScanResultRequest(wifiRequestId, sequenceId, payload);

                        case WifiRequestId.GetNewWifiDetails:
                            // This request doesn't have a payload
                            return new WifiGetNewDetailsRequest(wifiRequestId, sequenceId);

                        case WifiRequestId.SetWifiOperationResult:
                            return new WifiSetRequest(wifiRequestId, sequenceId, payload);

                        default:
                            throw new InvalidOperationException($"Unknown response payload type: {wifiRequestId}");
                    }
                default:
                    throw new InvalidOperationException($"Unknown Category ID type: {categoryId}");
            }
        }
    }
}
