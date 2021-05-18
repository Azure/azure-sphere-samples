function Test-CmdArgs {
    param(
        $cmdArgs
    )
    
    $errors = @()

    if (-not $cmdArgs.Contains("--Hostname")) {
        $errors += "Error: The hostname of your Azure IoT Edge device must be set in the 'CmdArgs' field of your app_manifest.json: `"--Hostname`",`"<hub_hostname>`"" 
    }
    if (-not $cmdArgs.Contains("--IoTEdgeRootCAPath")) {
        $errors += "Error: The path to the root CA certificate for your IoT Edge device must be set in the 'CmdArgs' field on your app_manifest.json: `"--IoTEdgeRootCAPath`", `"<path to .pem file>`""
    }

    return $errors
}

function Test-AllowedConnections {
    param(
        $allowedConnections
    )

    $errors = @()

    if (($allowedConnections -eq $null) -or 
        ($allowedConnections.Count -eq 0)) {
        $errors += "Error: The 'AllowedConnections' field of your app_manifest.json must contain your IoT edge device address(es)"
    }

    return $errors
}

Export-ModuleMember -Function Test-CmdArgs, Test-AllowedConnections