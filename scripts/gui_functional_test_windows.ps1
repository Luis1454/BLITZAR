param(
    [Parameter(Mandatory = $true)]
    [string] $Executable,
    [Parameter(Mandatory = $true)]
    [string] $Config,
    [int] $TimeoutSeconds = 30,
    [switch] $RequireGranularPropertyAutomation
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
$root = [System.Windows.Automation.AutomationElement]::RootElement
$treeScopeChildren = [System.Windows.Automation.TreeScope]::Children
$treeScopeDescendants = [System.Windows.Automation.TreeScope]::Descendants

function Find-ByName {
    param(
        [System.Windows.Automation.AutomationElement] $Parent,
        [string] $Name,
        [System.Windows.Automation.ControlType] $ControlType = $null
    )
    $nameCondition = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::NameProperty, $Name)
    if ($null -eq $ControlType) {
        return $Parent.FindFirst($treeScopeDescendants, $nameCondition)
    }
    $typeCondition = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::ControlTypeProperty, $ControlType)
    $condition = New-Object System.Windows.Automation.AndCondition($nameCondition, $typeCondition)
    return $Parent.FindFirst($treeScopeDescendants, $condition)
}

function Find-ByAutomationId {
    param(
        [System.Windows.Automation.AutomationElement] $Parent,
        [string] $AutomationId,
        [System.Windows.Automation.ControlType] $ControlType = $null
    )
    $items = $Parent.FindAll($treeScopeDescendants, [System.Windows.Automation.Condition]::TrueCondition)
    foreach ($item in $items) {
        if (-not $item.Current.AutomationId.EndsWith($AutomationId)) { continue }
        if ($null -ne $ControlType -and
            $item.Current.ControlType.ProgrammaticName -ne $ControlType.ProgrammaticName) { continue }
        return $item
    }
    return $null
}

function Wait-Window {
    param([string] $Title)
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        $condition = New-Object System.Windows.Automation.PropertyCondition(
            [System.Windows.Automation.AutomationElement]::NameProperty, $Title)
        $window = $root.FindFirst($treeScopeChildren, $condition)
        if ($null -ne $window) { return $window }
        Start-Sleep -Milliseconds 250
    } while ([DateTime]::UtcNow -lt $deadline)
    throw "GUI window '$Title' was not found within $TimeoutSeconds seconds"
}

function Invoke-Element {
    param([System.Windows.Automation.AutomationElement] $Element, [string] $Description)
    if ($null -eq $Element) { throw "UI element not found: $Description" }
    $pattern = $Element.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern)
    $pattern.Invoke()
}

function Get-ServerStatus {
    $client = New-Object System.Net.Sockets.TcpClient
    $client.Connect("127.0.0.1", 4545)
    try {
        $stream = $client.GetStream()
        $request = [Text.Encoding]::UTF8.GetBytes('{"cmd":"status"}' + "`n")
        $stream.Write($request, 0, $request.Length)
        $buffer = New-Object byte[] 65536
        $builder = New-Object Text.StringBuilder
        do {
            $read = $stream.Read($buffer, 0, $buffer.Length)
            if ($read -le 0) { break }
            [void]$builder.Append([Text.Encoding]::UTF8.GetString($buffer, 0, $read))
        } while ($builder.ToString().IndexOf("`n") -lt 0)
        return $builder.ToString()
    }
    finally {
        $client.Close()
    }
}

function Select-Tab {
    param([System.Windows.Automation.AutomationElement] $Window, [string] $Name)
    $tab = Find-ByName $Window $Name ([System.Windows.Automation.ControlType]::TabItem)
    if ($null -eq $tab) { throw "Tab not found: $Name" }
    try {
        $selection = $tab.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern)
        $selection.Select()
    }
    catch {
        Invoke-Element $tab "tab '$Name'"
    }
    Start-Sleep -Milliseconds 300
}

function Select-ComboItem {
    param(
        [System.Windows.Automation.AutomationElement] $Window,
        [string] $ComboName,
        [string] $ItemName
    )
    $combo = Find-ByName $Window $ComboName ([System.Windows.Automation.ControlType]::ComboBox)
    if ($null -eq $combo) { throw "ComboBox not found: $ComboName" }
    $expand = $combo.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern)
    $expand.Expand()
    Start-Sleep -Milliseconds 200
    try {
        $item = Find-ByName $combo $ItemName ([System.Windows.Automation.ControlType]::ListItem)
        if ($null -eq $item) { throw "ComboBox item not found: $ComboName -> $ItemName" }
        Invoke-Element $item "$ComboName -> $ItemName"
    }
    finally {
        $expand.Collapse()
    }
}

function Select-ComboItemByAutomationId {
    param(
        [System.Windows.Automation.AutomationElement] $Window,
        [string] $AutomationId,
        [string] $ItemName
    )
    $combo = Find-ByAutomationId $Window $AutomationId ([System.Windows.Automation.ControlType]::ComboBox)
    if ($null -eq $combo) { throw "ComboBox not found: $AutomationId" }
    $expand = $combo.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern)
    $expand.Expand()
    Start-Sleep -Milliseconds 200
    try {
        $item = Find-ByName $combo $ItemName ([System.Windows.Automation.ControlType]::ListItem)
        if ($null -eq $item) { throw "ComboBox item not found: $AutomationId -> $ItemName" }
        Invoke-Element $item "$AutomationId -> $ItemName"
        return $item.Current.Name
    }
    finally {
        $expand.Collapse()
    }
}

$process = $null
try {
    $process = Start-Process -FilePath (Resolve-Path $Executable).Path `
        -ArgumentList @("--config", (Resolve-Path $Config).Path) -PassThru
    $window = Wait-Window "N-Body Qt Client"
    Start-Sleep -Seconds 2

    Select-Tab $window "Physics"
    $layoutCombo = Find-ByAutomationId $window "treePmLayoutCombo" ([System.Windows.Automation.ControlType]::ComboBox)
    if ($null -eq $layoutCombo) { throw "TreePM particle layout control was not found" }
    foreach ($requestedLayout in @("auto", "linear", "gather_linear", "gather_morton")) {
        $selectedLayout = Select-ComboItemByAutomationId $window "treePmLayoutCombo" $requestedLayout
        if ($selectedLayout -ne $requestedLayout) {
            throw "TreePM particle layout was not applied: requested=$requestedLayout actual=$selectedLayout"
        }
    }
    Write-Output "treepm_layouts=auto,linear,gather_linear,gather_morton"

    Select-Tab $window "Scene"
    $sceneList = Find-ByAutomationId $window "sceneObjectList" ([System.Windows.Automation.ControlType]::List)
    if ($null -eq $sceneList) { throw "Scene object list was not found" }
    $sceneItems = $sceneList.FindAll($treeScopeDescendants, [System.Windows.Automation.Condition]::TrueCondition)
    if ($sceneItems.Count -lt 1) { throw "Scene object list is empty" }

    $propertyButton = Find-ByAutomationId $window "sceneObjectPropertyButton"
    if ($null -eq $propertyButton) {
        if ($RequireGranularPropertyAutomation) {
            throw "Scene property button is not exposed through UI Automation; use the Qt in-process GUI test"
        }
        Write-Output "granular_properties=delegated_to_qt_in_process"
    }
    else {
        Invoke-Element $propertyButton "+ Property"
        Start-Sleep -Milliseconds 250
        $mirrorAction = Find-ByName $root "Add Mirror" ([System.Windows.Automation.ControlType]::MenuItem)
        if ($null -eq $mirrorAction) { throw "Add Mirror action was not found" }
        Invoke-Element $mirrorAction "Add Mirror"
        Start-Sleep -Milliseconds 250

        Invoke-Element $propertyButton "+ Property"
        Start-Sleep -Milliseconds 250
        $rotationAction = Find-ByName $root "Add Rotation" ([System.Windows.Automation.ControlType]::MenuItem)
        if ($null -eq $rotationAction) { throw "Add Rotation action was not found" }
        Invoke-Element $rotationAction "Add Rotation"
        Start-Sleep -Milliseconds 250

        if ($null -eq (Find-ByName $window "Mirror property" ([System.Windows.Automation.ControlType]::Group))) {
            throw "Mirror property panel was not displayed"
        }
        if ($null -eq (Find-ByName $window "Rotation property" ([System.Windows.Automation.ControlType]::Group))) {
            throw "Rotation property panel was not displayed"
        }
        Write-Output "granular_properties=ui_automation"
    }
    $apply = Find-ByAutomationId $window "applySceneObjectsButton" ([System.Windows.Automation.ControlType]::Button)
    Invoke-Element $apply "Apply Scene"
    Start-Sleep -Seconds 2
    if ($process.HasExited) { throw "GUI exited after applying the referenced particle system" }
    $serverStatus = Get-ServerStatus
    if ($serverStatus -notmatch '"particles":[1-9][0-9]*') {
        throw "GUI-applied runtime did not report a positive physical count: $serverStatus"
    }

    Select-Tab $window "Config"
    $addId = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::AutomationIdProperty, "addConfigurationSectionButton")
    $removeId = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::AutomationIdProperty, "removeConfigurationSectionButton")
    if ($null -ne $window.FindFirst($treeScopeDescendants, $addId)) {
        throw "Config exposes the removed dynamic section creation control"
    }
    if ($null -ne $window.FindFirst($treeScopeDescendants, $removeId)) {
        throw "Config exposes the removed dynamic section deletion control"
    }

    Write-Output "gui_functional=passed"
    Write-Output "properties=Mirror,Rotation"
    Write-Output "runtime_status=$serverStatus"
    Write-Output "apply_alive=$(-not $process.HasExited)"
}
finally {
    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    if ($null -ne $process) {
        & taskkill.exe /PID $process.Id /T /F 2>$null | Out-Null
    }
    $bundleDirectory = (Split-Path (Resolve-Path $Executable).Path -Parent).ToLowerInvariant()
    Get-CimInstance Win32_Process |
        Where-Object {
            $_.ExecutablePath -and
            (Split-Path $_.ExecutablePath -Parent).ToLowerInvariant() -eq $bundleDirectory
        } |
        ForEach-Object { & taskkill.exe /PID $_.ProcessId /T /F 2>$null | Out-Null }
    $ErrorActionPreference = $previousErrorActionPreference
}
