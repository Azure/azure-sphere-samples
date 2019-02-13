// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Sphere.Samples.WifiSetupAndDeviceControlViaBle.MessageProtocol
{
    using System;

    internal static class ByteArrayHelper
    {

        public static void WriteBytes(byte[] source, byte[] destination, uint offset = 0)
        {
            if (source.Length > 0)
            {
                Buffer.BlockCopy(source, 0, destination, (int)offset, source.Length);
            }
        }

        public static byte[] ReadBytes(byte[] source, uint offset, uint count)
        {
            byte[] result = new byte[count];
            Buffer.BlockCopy(source, (int)offset, result, 0, (int)count);

            return result;
        }

        public static string ReadDelimitedHex(byte[] source, uint offset, uint count, char? delimiter = null)
        {
            byte[] data = ReadBytes(source, offset, count);

            string result = BitConverter.ToString(data);

            if (delimiter.HasValue)
            {
                result = result.Replace('-', delimiter.Value);
            }

            return result;
        }

        public static short ReadSignedByte(byte[] source, uint offset)
        {
            return (sbyte)source[offset];
        }

        public static ushort ReadLsbUInt16(byte[] source, uint offset)
        {
            return (ushort)ReadLsbNumber(source, offset, 2);
        }

        public static uint ReadLsbUInt32(byte[] source, uint offset)
        {
            return (uint)ReadLsbNumber(source, offset, 4);
        }

        public static void WriteLsbUInt16(ushort value, byte[] destination, uint offset)
        {
            WriteLsbNumber(value, destination, offset, 2);
        }

        private static ulong ReadLsbNumber(byte[] source, uint offset, uint count)
        {
            uint value = 0;

            for (int i = 0; i < count; i++)
            {
                value += (uint)(source[offset + i] << (i * 8));
            }

            return value;
        }

        private static void WriteLsbNumber(ulong value, byte[] destination, uint offset, uint count)
        {
            ulong tempValue = value;

            for (int i = 0; i < count; i++)
            {
                destination[offset + i] = (byte)(tempValue & 0xFF);
                tempValue = (tempValue >> 8);
            }

            if (tempValue != 0)
            {
                throw new InvalidOperationException($"The value '{value}' cannot fit in {count} bytes.");
            }
        }
    }
}
