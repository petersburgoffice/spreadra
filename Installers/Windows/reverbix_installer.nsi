; Reverbix Windows Installer
; NSIS Script for creating Windows installer

!include "MUI2.nsh"
!include "FileFunc.nsh"

; Installer Information
!define PRODUCT_NAME "Reverbix"
!define PRODUCT_VERSION "1.0.0"
!define PRODUCT_PUBLISHER "Reverbix"
!define PRODUCT_WEB_SITE "https://github.com/azverev/reverbix"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\Reverbix"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "..\..\resources\icon.ico"
!define MUI_UNICON "..\..\resources\icon.ico"

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

; Installer details
Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "Reverbix_${PRODUCT_VERSION}_Windows_Installer.exe"
InstallDir "$PROGRAMFILES64\Common Files\VST3"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

; Version Information
VIProductVersion "${PRODUCT_VERSION}.0"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments" "Professional Reverb Plugin"
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "${PRODUCT_PUBLISHER}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "© 2024 ${PRODUCT_PUBLISHER}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "${PRODUCT_NAME} Installer"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${PRODUCT_VERSION}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductVersion" "${PRODUCT_VERSION}"

; Request application privileges for Windows Vista/7/8/10/11
RequestExecutionLevel admin

; Installer Sections
Section "VST3 Plugin (Required)" SEC01
  SectionIn RO
  
  ; Set output path to VST3 directory
  SetOutPath "$INSTDIR"
  
  ; Copy VST3 plugin
  File /r "..\..\build\Reverbix_artefacts\VST3\Reverbix.vst3"
  
  ; Create uninstaller
  WriteUninstaller "$INSTDIR\Reverbix Uninstaller.exe"
  
  ; Registry entries
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\Reverbix.vst3"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\Reverbix Uninstaller.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\Reverbix.vst3"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  
  ; Calculate installed size
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "EstimatedSize" "$0"
SectionEnd

Section "AAX Plugin (Pro Tools)" SEC02
  ; Set output path to AAX directory
  SetOutPath "$PROGRAMFILES64\Common Files\Avid\Audio\Plug-Ins"
  
  ; Copy AAX plugin if it exists
  IfFileExists "..\..\build\Reverbix_artefacts\AAX\Reverbix.aaxplugin" 0 skip_aax
    File /r "..\..\build\Reverbix_artefacts\AAX\Reverbix.aaxplugin"
  skip_aax:
SectionEnd

Section "Standalone Application" SEC03
  ; Set output path to Program Files
  SetOutPath "$PROGRAMFILES64\${PRODUCT_NAME}"
  
  ; Copy standalone application if it exists
  IfFileExists "..\..\build\Reverbix_artefacts\Standalone\Reverbix.exe" 0 skip_standalone
    File "..\..\build\Reverbix_artefacts\Standalone\Reverbix.exe"
    
    ; Create desktop shortcut if standalone exists
    CreateShortCut "$DESKTOP\${PRODUCT_NAME}.lnk" "$PROGRAMFILES64\${PRODUCT_NAME}\Reverbix.exe"
  skip_standalone:
SectionEnd

Section "Desktop Shortcut" SEC04
  ; Only create if standalone app was installed
  IfFileExists "$PROGRAMFILES64\${PRODUCT_NAME}\Reverbix.exe" 0 skip_desktop
    CreateShortCut "$DESKTOP\${PRODUCT_NAME}.lnk" "$PROGRAMFILES64\${PRODUCT_NAME}\Reverbix.exe"
  skip_desktop:
SectionEnd

Section "Start Menu Shortcuts" SEC05
  CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
  
  ; Create shortcuts for installed components
  IfFileExists "$PROGRAMFILES64\${PRODUCT_NAME}\Reverbix.exe" 0 +2
    CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$PROGRAMFILES64\${PRODUCT_NAME}\Reverbix.exe"
  
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk" "$INSTDIR\Reverbix Uninstaller.exe"
SectionEnd

; Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC01} "The main VST3 plugin file. This is required for the plugin to work in your DAW."
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC02} "AAX plugin for Pro Tools (if available)."
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC03} "Standalone application that can run independently of a DAW."
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC04} "Creates a shortcut on your desktop for the standalone application."
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC05} "Creates shortcuts in the Start Menu."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; Installer Functions
Function .onInit
  ; Check if already installed
  ReadRegStr $R0 ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString"
  StrCmp $R0 "" done
  
  MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
  "${PRODUCT_NAME} is already installed. $\n$\nClick `OK` to remove the \
  previous version or `Cancel` to cancel this upgrade." \
  IDOK uninst
  Abort
  
uninst:
  ClearErrors
  ExecWait '$R0 _?=$INSTDIR'
  
  IfErrors no_remove_uninstaller done
    Delete $R0
    RMDir $INSTDIR
  
no_remove_uninstaller:
  
done:
FunctionEnd

; Uninstaller Section
Section Uninstall
  ; Remove plugin files
  RMDir /r "$INSTDIR\Reverbix.vst3"
  RMDir /r "$PROGRAMFILES64\Common Files\Avid\Audio\Plug-Ins\Reverbix.aaxplugin"
  RMDir /r "$PROGRAMFILES64\${PRODUCT_NAME}"
  Delete "$INSTDIR\Reverbix Uninstaller.exe"
  
  ; Remove shortcuts
  Delete "$DESKTOP\${PRODUCT_NAME}.lnk"
  RMDir /r "$SMPROGRAMS\${PRODUCT_NAME}"
  
  ; Remove registry keys
  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  
  ; Remove installation directory if empty
  RMDir "$INSTDIR"
  
  SetAutoClose true
SectionEnd

; Uninstaller Functions
Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort
FunctionEnd 