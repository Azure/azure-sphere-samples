function Test-CmdArgs {
    param(
        [Parameter(Mandatory=$True)] $cmdArgs = $null
    )
    
    $errors = @()

    if (-not $cmdArgs.Contains("--ScopeID")) {
        $errors += "Error: The ID scope for your IoT Central app or DPS instance must be set in the 'CmdArgs' field of your app_manifest.json: `"--ScopeID`", `"<id scope>`""
    }

    return $errors
}

function Test-AllowedConnections {
    param(
        [Parameter(Mandatory=$true)] $allowedConnections = $null
    )

    $dpsEndpoint = "global.azure-devices-provisioning.net"

    if($allowedConnections -eq $null)
    {
        $connectionCount = 0
    } else {
        $connectionCount = $allowedConnections.Count
    }

    $errors = @()

    # Check for Azure DPS endpoint address only if ConnectionType is set to DPS.
    if ($connectionCount -gt 0 -and $allowedConnections.Contains($dpsEndpoint)) {
        $connectionCount--
    } else {
        $errors += "Error: The 'AllowedConnections' field in your app_manifest.json must contain the Azure DPS endpoint address (global.azure-devices-provisioning.net)"
    }

    if ($connectionCount -eq 0) {
        $errors += "Error: The 'AllowedConnections' field in your app_manifest.json must contain your IoT Hub or IoT Central application endpoint address(es)"
    }

    return $errors
}

Export-ModuleMember -Function Test-CmdArgs, Test-AllowedConnections