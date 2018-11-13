﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Sphere.Samples.WifiConfigMessageProtocol.Contracts
{
    public abstract class ResponseBase
    {
        internal abstract byte[] GetPayload();
    }
}
