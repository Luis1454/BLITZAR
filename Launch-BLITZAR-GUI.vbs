Option Explicit

Dim shell, fileSystem, root, executable
Set shell = CreateObject("WScript.Shell")
Set fileSystem = CreateObject("Scripting.FileSystemObject")
root = fileSystem.GetParentFolderName(WScript.ScriptFullName)
executable = root & "\build-gui-check\blitzar-client.exe"

If Not fileSystem.FileExists(executable) Then
    MsgBox "BLITZAR GUI build is missing: " & executable, vbCritical, "BLITZAR"
    WScript.Quit 1
End If

shell.Run Chr(34) & executable & Chr(34), 1, False
