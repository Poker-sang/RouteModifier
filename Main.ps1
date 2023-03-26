. $PSScriptRoot/Functions.ps1
. $PSScriptRoot/NetAdapterInfo.ps1

$info = Get-PhysicalNics

Remove-AdapterRoute

Remove-AdapterRoute ::/0

Set-AdapterRoute $info "Realtek PCIe GbE Family Controller" 172.18.6.57/32

Set-AdapterRoute $info "Intel(R) Wi-Fi 6 AX200 160MHz" 0.0.0.0/0