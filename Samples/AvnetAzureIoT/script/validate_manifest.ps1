# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

Write-Output "Validating project app_manifest.json"

$scriptPath =Split-Path $PSCommandPath -Parent 
$manifestFile = Join-Path $scriptPath "..\app_manifest.json" -Resolve -ErrorAction SilentlyContinue
if ($manifestFile -eq $null) {
    Write-Output "Error: Cannot find the app_manifest.json"
    return 1
}

$ret=0
$deviceAuthPlaceholder="00000000-0000-0000-0000-000000000000"
$dpsEndpoint = "global.azure-devices-provisioning.net"

$json = Get-Content $manifestFile -Raw
$jsonObj = ConvertFrom-Json -InputObject $json

$cmdArgs = $jsonobj.CmdArgs
if ($cmdArgs -eq $null -or $cmdArgs.Count -eq 0) {
    Write-Output "Error: The app_manifest.json 'CmdArgs' needs to contain the ConnectionType, Azure IoT Hub hostname and Device ID for Direct Connection, or ConnectionType and Scope ID for DPS connection."
    $ret=1
}

$allowedConnections=$jsonobj.Capabilities.AllowedConnections
if($allowedConnections -eq $null)
{
    $connectionCount = 0
} else {
    $connectionCount = $allowedConnections.Count
}

# Check for Azure DPS endpoint address only if ConnectionType is set to DPS.
if ($cmdArgs -contains "--ConnectionType DPS") {
    if ($connectionCount -gt 0 -and $allowedConnections.Contains($dpsEndpoint)) {
        $connectionCount--
    } else {
        Write-Output "Error: The app_manifest.json 'AllowedConnections' needs to contain the Azure DPS endpoint address (global.azure-devices-provisioning.net)"
        $ret=1
    }
}

if ($connectionCount -eq 0) {
    Write-Output "Error: The app_manifest.json 'AllowedConnections' needs to contain your IoT Hub/Central application endpoint address(es)"
    $ret=1
}

$deviceAuth=$jsonobj.Capabilities.DeviceAuthentication
if ($deviceAuth -eq $null -or $deviceAuth -eq $deviceAuthPlaceholder) {
    Write-Output "Error: The app_manifest.json 'DeviceAuthentication' needs to contain your Azure Sphere Tenant Id. This can be obtained using the Azure Sphere Developer Command Prompt 'azsphere tenant show-selected'".
    $ret=1
}

if ($ret -eq 0) {
    Write-Output "app_manifest.json IoT Hub/Central parameters exist."
}

exit $ret
