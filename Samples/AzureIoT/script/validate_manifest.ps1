# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

Write-Output "Validating project app_manifest.json"

$scriptPath =Split-Path $PSCommandPath -Parent 
$manifestFile = Join-Path $scriptPath "..\app_manifest.json" -Resolve -ErrorAction SilentlyContinue
if ($manifestFile -eq $null) {
    Write-Output "Cannot find the app_manifest.json"
    return 1
}

$ret=0
$deviceAuthPlaceholder="00000000-0000-0000-0000-000000000000"
$dpsEndpoint = "global.azure-devices-provisioning.net"

$json = Get-Content $manifestFile -Raw
$jsonObj = ConvertFrom-Json -InputObject $json

if ($jsonobj.CmdArgs.Count -eq 0) {
    Write-Output "CmdArgs is empty, this needs to contain the ScopeId for your IoT Hub/Central application"
    $ret=1
}

$allowedConnections=$jsonobj.Capabilities.AllowedConnections
if ($allowedConnections.Count -eq 0) {
    Write-Output "The app_manifest.json 'AllowedConnections' needs to contain:"
    Write-Output "1. The Azure DPS Endpoint address (global.azure-devices-provisioning.net)"
    Write-Output "2. Your IoT Hub/Central application Endpoint address"        
    $ret=1
}

if ($allowedConnections.Count -eq 1) {
    if ($dpsEndpoint -eq $allowedConnections[0]) {
        Write-Output "The app_manifest.json 'AllowedConnections' needs to contain:"
        Write-Output "  Your IoT Hub/Central application Endpoint address"
        $ret=1
    } else {
        Write-Output "The app_manifest.json 'AllowedConnections' needs to contain:"
        Write-Output "  The DPS Endpoint address: global.azure-devices-provisioning.net"
    }
}

$deviceAuth=$jsonobj.Capabilities.DeviceAuthentication
if ($deviceAuth -eq $deviceAuthPlaceholder) {
    Write-Output "The app_manifest.json file contains a placeholder 'DeviceAuthentication'"
    Write-Output "this needs to be replaced with your Azure Sphere Tenant Id"
    Write-Output "This can be obtained using the Azure Sphere Developer Command Prompt "
    Write-Output "'azsphere tenant show-selected'"        
    $ret=1
}

if ($ret -eq 0) {
    Write-Output "app_manifest.json IoT Hub/Central parameters exist."
}

exit $ret
