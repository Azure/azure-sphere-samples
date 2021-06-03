# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
param(
    [Parameter(Mandatory=$True)][string] $Manifest,
    [Parameter(Mandatory=$True)][string] $Helper
)

Import-Module -Name $Helper

Write-Output "Validating project $($Manifest)"

$manifestPreset = Test-Path $Manifest

if (-not $manifestPreset) {
    Write-Output "Error: Cannot find the app_manifest.json at $($Manifest)"
    return 1
}

$ret=0
$deviceAuthPlaceholder="00000000-0000-0000-0000-000000000000"

$json = Get-Content $Manifest -Raw
$jsonObj = ConvertFrom-Json -InputObject $json

$cmdArgs = $jsonobj.CmdArgs
Write-Host $cmdArgs
if ($cmdArgs -eq $null -or $cmdArgs.Count -eq 0) {
    Write-Output "Error: The 'CmdArgs' field in your app_manifest.json must be set."
    $ret=1
}

$cmdArgsErrors = Test-CmdArgs $cmdArgs

if ($cmdArgsErrors.Count -gt 0) {
    Write-Output $cmdArgsErrors
    $ret = 1
}

$allowedConnectionsErrors = Test-AllowedConnections $jsonobj.Capabilities.AllowedConnections

if ($allowedConnectionsErrors.Count -gt 0) {
    Write-Output $allowedConnectionsErrors
    $ret = 1
}

$deviceAuth=$jsonobj.Capabilities.DeviceAuthentication
if ($deviceAuth -eq $null -or $deviceAuth -eq $deviceAuthPlaceholder) {
    Write-Output "Error: The 'DeviceAuthentication' field in your app_maifest.json must be set to your Azure Sphere Tenant Id. This can be obtained using the Azure Sphere Developer Command Prompt 'azsphere tenant show-selected'".
    $ret=1
}

if ($ret -eq 0) {
    Write-Output "app_manifest.json parameters exist."
}

exit $ret
