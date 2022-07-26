# Azure Sphere Device REST API yaml files

This folder contains two yaml files.

## device_rest_api.yaml

The device_rest_api.yaml file contains the OpenAPI specification for an Azure Sphere Device REST APIs. The specification contains definitions for the following API categories: Application management, Certificate management, Device state/configuration, Error reports and diagnostic logging, Image management, Network configuration, and Wi-Fi configuration.

## windows_device_communication_service.yaml

The windows_device_communication_service.yaml file contains the OpenAPI specification for the Azure Sphere Windows Device Communication Service, which exposes REST APIs to Enumerate devices managed by the Device Communication Service.

## Converting the Yaml files to code.

You can generate code from the Yaml files using [autorest](https://github.com/Azure/autorest/blob/main/docs/generate/readme.md) - supported languages include: Python, C#, Java, Typescript, Go, Powershell.

