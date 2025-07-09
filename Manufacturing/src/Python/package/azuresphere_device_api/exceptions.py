
"""This module contains the set of Azure Sphere Device API exceptions."""
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.


class AzureSphereDeviceApiException(Exception):
    """Base exception for all errors raised by Azure Sphere Device APIs."""

    pass  # pylint: disable=unnecessary-pass


class ValidationError(AzureSphereDeviceApiException):
    """Input parameter validation failures."""
    pass  # pylint: disable=unnecessary-pass


class UnknownDeviceError(AzureSphereDeviceApiException):
    """Unknown error returned by Device Rest APIs"""
    pass  # pylint: disable=unnecessary-pass


class DeviceError(AzureSphereDeviceApiException):
    """A known error returned by Device Rest APIs"""
    pass  # pylint: disable=unnecessary-pass


class InvalidJsonError(AzureSphereDeviceApiException):
    """Invalid JSON returned by Device Rest APIs"""
    pass  # pylint: disable=unnecessary-pass
