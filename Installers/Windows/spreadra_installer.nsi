; Spreadra Windows Installer
; NSIS Script for creating Windows installer

!include "MUI2.nsh"
!include "FileFunc.nsh"

; Installer Information
!define PRODUCT_NAME "Spreadra"
!define PRODUCT_VERSION "0.9.21"
!define PRODUCT_PUBLISHER "SonicMakers"
!define PRODUCT_WEB_SITE "https://github.com/petersburgoffice/spreadra"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\Spreadra"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; MUI Settings (no icons)
!define MUI_ABORTWARNING

; Welcome page
!insertmacro MUI_PAGE_WELCOME

; License page
!insertmacro MUI_PAGE_LICENSE "license.txt"

; Components page
!insertmacro MUI_PAGE_COMPONENTS

; Directory page
!insertmacro MUI_PAGE_DIRECTORY

; Instfiles page
!insertmacro MUI_PAGE_INSTFILES

; Finish page
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; Installer attributes
Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "Spreadra_${PRODUCT_VERSION}_Windows_Installer.exe"
InstallDir "$PROGRAMFILES64\${PRODUCT_NAME}"
ShowInstDetails show
ShowUnInstDetails show

; Request admin privileges
RequestExecutionLevel admin

; Install Sections
Section "VST3 Plugin" SEC01
    SetOutPath "$PROGRAMFILES64\Common Files\VST3"
    SetOverwrite ifnewer
    
    ; Check if VST3 plugin exists
    IfFileExists "..\..\build\Spreadra_artefacts\VST3\Spreadra.vst3" 0 skip_vst3
    File /r "..\..\build\Spreadra_artefacts\VST3\Spreadra.vst3"
    skip_vst3:
SectionEnd

Section "Core Files" SEC02
    SetOutPath "$INSTDIR"
    SetOverwrite ifnewer
    
    ; Create uninstaller
    WriteUninstaller "$INSTDIR\Spreadra Uninstaller.exe"
    
    ; Write registry keys for Windows Add/Remove Programs
    WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\Spreadra Uninstaller.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd

Section "AAX Plugin (Pro Tools)" SEC03
    SetOutPath "$PROGRAMFILES64\Common Files\Avid\Audio\Plug-Ins"
    SetOverwrite ifnewer
    
    ; Check if AAX plugin exists  
    IfFileExists "..\..\build\Spreadra_artefacts\AAX\Spreadra.aaxplugin" 0 skip_aax
    File /r "..\..\build\Spreadra_artefacts\AAX\Spreadra.aaxplugin"
    skip_aax:
SectionEnd

; Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC01} "VST3 plugin for use in DAWs like Cubase, FL Studio, etc."
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC02} "Core application files and uninstaller"
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC03} "AAX plugin for Pro Tools (requires Pro Tools to be installed)"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; Uninstaller
Section "Uninstall"
    ; Remove registry keys
    DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
    DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
    
    ; Remove plugins
    RMDir /r "$PROGRAMFILES64\Common Files\VST3\Spreadra.vst3"
    RMDir /r "$PROGRAMFILES64\Common Files\Avid\Audio\Plug-Ins\Spreadra.aaxplugin"
    
    ; Remove uninstaller
    Delete "$INSTDIR\Spreadra Uninstaller.exe"
    RMDir "$INSTDIR"
    
    SetAutoClose true
SectionEnd

; Functions
Function .onInit
    ; Check if already installed
    ReadRegStr $R0 ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString"
    StrCmp $R0 "" done
    
    MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
    "${PRODUCT_NAME} is already installed. $\n$\nClick `OK` to remove the \
    previous version or `Cancel` to cancel this upgrade." \
    IDOK uninst
    Abort
    
    ; Run the uninstaller
    uninst:
        ClearErrors
        ExecWait '$R0 _?=$INSTDIR'
        
        IfErrors no_remove_uninstaller done
        no_remove_uninstaller:
    
    done:
FunctionEnd

Function un.onInit
    MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
    Abort
FunctionEnd

Function un.onUninstSuccess
    HideWindow
    MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd
