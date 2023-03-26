. $PSScriptRoot/NetAdapterInfo.ps1

function Test-PhysicalNic {
    param (
        [Parameter(
            Mandatory,
            ValueFromPipeline
        )]
        [NetAdapterInfo]
        $Info
    )
  
    $str = Get-ItemProperty `
        -Path "HKLM:\SYSTEM\CurrentControlSet\Control\Network\{4D36E972-E325-11CE-BFC1-08002BE10318}\{$($Info.SettingID)}\Connection" `
        -Name PnPInstanceId
    $str.PnPInstanceId.StartsWith("PCI")
}

function Get-PhysicalNics {
    Get-CimInstance `
        -Class Win32_NetworkAdapterConfiguration `
    | ForEach-Object {
        [NetAdapterInfo]::new(
            $_.SettingID,
            $_.Description,
            $_.DefaultIPGateway
        )
    } `
    | Where-Object { Test-PhysicalNic $_ }
    
}

function Remove-AdapterRoute {
    param (
        [string]
        $DestinationPrefix = "0.0.0.0/0"
    )
    Remove-NetRoute $DestinationPrefix
}

function Set-AdapterRoute {
    param (
        [NetAdapterInfo[]]
        $List,
        [string]
        $AdapterDescription,
        [string]
        $DestinationPrefix = "0.0.0.0/0",
        [UInt16]
        $RouteMetric = 0,
        [bool]
        $PersistentStore = $false
    )
    $Private:NextHop = ($List | Where-Object Description -EQ $AdapterDescription)[0].DefaultIPGateway
    New-NetRoute `
        $DestinationPrefix `
        -InterfaceAlias $AdapterDescription `
        -NextHop $Private:NextHop `
        -PolicyStore ($PersistentStore ? "PersistentStore" : "ActiveStore")
} 

function Set-AdapterRoute {
    param (
        [NetAdapterInfo[]]
        $List,
        [string]
        $InterfaceDescription,
        [string]
        $DestinationPrefix = "0.0.0.0/0",
        [string]
        $IsIpv4 = $true,
        [UInt16]
        $RouteMetric = 0,
        [bool]
        $PersistentStore = $false
    )
    $InterfaceAlias = (Get-NetAdapter | Where-Object Name -EQ $InterfaceDescription)[0].Name
    $gateways = ($List | Where-Object Description -EQ $InterfaceAlias)[0].DefaultIPGateway
    if ($gateways -EQ "") {
        return
    }
    foreach ($gateway in $gateways.Split(' ')) {
        if (($IsIpv4 -and ([System.Net.IPAddress]::Parse($gateway).AddressFamily -EQ [System.Net.Sockets.AddressFamily]::InterNetwork)) -or `
            (!$IsIpv4 -and ([System.Net.IPAddress]::Parse($gateway).AddressFamily  -EQ [System.Net.Sockets.AddressFamily]::InterNetworkV6))) {
            New-NetRoute `
                $DestinationPrefix `
                -InterfaceAlias $InterfaceAlias `
                -NextHop $gateway `
                -PolicyStore ($PersistentStore ? "PersistentStore" : "ActiveStore") `
                -RouteMetric $RouteMetric
            return
        }
    }
} 