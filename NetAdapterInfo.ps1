class NetAdapterInfo {
    [guid] $SettingID
    [string] $Description
    [string] $DefaultIPGateway

    NetAdapterInfo (
        [guid] $settingID,
        [string] $description,
        [string] $defaultIPGateway
    ) {
        $this.SettingID = $settingID
        $this.Description = $description
        $this.DefaultIPGateway = $defaultIPGateway
    }
}
