function Test-CmdArgs {
    param(
        $cmdArgs
    )
    
    $errors = @()

    if (-not $cmdArgs.Contains("--Hostname")) {
        $errors += "Error: The hostname of your Azure IoT Hub must be set in the 'CmdArgs' field of your app_manifest.json: `"--Hostname`",`"<hub_hostname>`"" 
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
        $errors += "Error: The 'AllowedConnections' field of your app_manifest.json must contain the address of your Azure IoT Hub"
    }

    return $errors
}

Export-ModuleMember -Function Test-CmdArgs, Test-AllowedConnections