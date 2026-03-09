!define APPNAME "Darvin IDE"
!define VERSION "1.0.0"
!define ICON "darvincpp.ico"

; --- HIGH COMPRESSION TRICK ---
; Crush 700MB down to ~90MB using Solid LZMA
SetCompressor /SOLID lzma
SetCompressorDictSize 64

; --- ICON CONFIG ---
Icon "${ICON}"
UninstallIcon "${ICON}"

OutFile "Darvin_Setup.exe"
InstallDir "$PROGRAMFILES\Darvin"
RequestExecutionLevel admin

Page directory
Page instfiles

Section "Install"
    SetOutPath "$INSTDIR"
    ; Package the clean, verified runtime bundle (relative to this script)
    File /r "Darvin_Production_Test\*"
    File "${ICON}"

    ; Create Shortcuts with the app icon
    CreateDirectory "$SMPROGRAMS\Darvin"
    CreateShortcut "$SMPROGRAMS\Darvin\Darvin.lnk" "$INSTDIR\darvin.exe" "" "$INSTDIR\${ICON}"
    CreateShortcut "$DESKTOP\Darvin.lnk" "$INSTDIR\darvin.exe" "" "$INSTDIR\${ICON}"

    ; Create Uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    CreateShortcut "$SMPROGRAMS\Darvin\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Uninstall"
    Delete "$DESKTOP\Darvin.lnk"
    RMDir /r "$SMPROGRAMS\Darvin"
    RMDir /r "$INSTDIR"
SectionEnd