!include "MUI2.nsh"

!ifndef INPUT_DIR
  !error "INPUT_DIR must be defined"
!endif

!ifndef OUTPUT_FILE
  !error "OUTPUT_FILE must be defined"
!endif

!ifndef PRODUCT_VERSION
  !error "PRODUCT_VERSION must be defined"
!endif

Name "BLITZAR"
OutFile "${OUTPUT_FILE}"
InstallDir "$LocalAppData\Programs\BLITZAR"
RequestExecutionLevel user
Unicode true
ShowInstDetails show
ShowUninstDetails show
SetCompressor /SOLID lzma

!define MUI_ABORTWARNING
!define MUI_ICON "${INPUT_DIR}\blitzar-client.exe"
!define MUI_UNICON "${INPUT_DIR}\blitzar-client.exe"

!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Function .onInit
    SetShellVarContext current
FunctionEnd

Section "BLITZAR" SecMain
    SetOutPath "$INSTDIR"
    File /r "${INPUT_DIR}\*"
    CreateDirectory "$SMPROGRAMS\BLITZAR"
    CreateShortcut "$SMPROGRAMS\BLITZAR\BLITZAR GUI.lnk" "$INSTDIR\blitzar-client.exe" "--config $\"$INSTDIR\simulation.ini$\" --module qt" "$INSTDIR\blitzar-client.exe"
    CreateShortcut "$SMPROGRAMS\BLITZAR\Uninstall BLITZAR.lnk" "$INSTDIR\Uninstall BLITZAR.exe"
    CreateShortcut "$DESKTOP\BLITZAR GUI.lnk" "$INSTDIR\blitzar-client.exe" "--config $\"$INSTDIR\simulation.ini$\" --module qt" "$INSTDIR\blitzar-client.exe"
    WriteUninstaller "$INSTDIR\Uninstall BLITZAR.exe"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\BLITZAR" "DisplayName" "BLITZAR"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\BLITZAR" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\BLITZAR" "Publisher" "BLITZAR"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\BLITZAR" "InstallLocation" "$INSTDIR"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\BLITZAR" "DisplayIcon" "$INSTDIR\blitzar-client.exe"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\BLITZAR" "UninstallString" "$\"$INSTDIR\Uninstall BLITZAR.exe$\""
SectionEnd

Section "Uninstall"
    Delete "$DESKTOP\BLITZAR GUI.lnk"
    Delete "$SMPROGRAMS\BLITZAR\BLITZAR GUI.lnk"
    Delete "$SMPROGRAMS\BLITZAR\Uninstall BLITZAR.lnk"
    RMDir "$SMPROGRAMS\BLITZAR"
    Delete "$INSTDIR\Uninstall BLITZAR.exe"
    RMDir /r "$INSTDIR"
    DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\BLITZAR"
SectionEnd