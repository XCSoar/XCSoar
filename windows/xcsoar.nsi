; XCSoar NSIS Installer Script for WIN64OPENGL
; This script creates a Windows installer for the 64-bit ANGLE (OpenGL ES) build
; Including XCSoar.exe and the required ANGLE DLLs (libEGL.dll and libGLESv2.dll)

; Product name can be overridden from command line with -DPRODUCT_NAME=name
!ifndef PRODUCT_NAME
!define PRODUCT_NAME "XCSoar"
!endif

; Version should be coming via override from command line with -DPRODUCT_VERSION=x.y
!ifndef PRODUCT_VERSION
!define PRODUCT_VERSION "dev"
!endif

; Installer label (e.g., "XCSoar" or "XCSoar Testing")
!ifndef INSTALLER_LABEL
!define INSTALLER_LABEL "${PRODUCT_NAME}"
!endif

; Binary directory can be overridden from command line
!ifndef BIN_DIR
!define BIN_DIR "..\output\WIN64OPENGL\bin"
!endif

!define PRODUCT_PUBLISHER "XCSoar Development Team"
!define PRODUCT_WEB_SITE "https://xcsoar.org"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

; Include Modern UI
!include "MUI2.nsh"

; General settings
Name "${INSTALLER_LABEL} ${PRODUCT_VERSION}"

; Output file can be overridden from command line
!ifndef OUTPUT_FILE
OutFile "..\output\WIN64OPENGL\XCSoar-Installer.exe"
!else
OutFile "${OUTPUT_FILE}"
!endif

InstallDir "$PROGRAMFILES64\XCSoar"
InstallDirRegKey HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation"
RequestExecutionLevel admin

; Interface Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\COPYING"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

; Languages
!insertmacro MUI_LANGUAGE "English"

; Installer Section
Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite on

  ; Install main executable
  File "${BIN_DIR}\XCSoar.exe"

  ; Install ANGLE DLLs
  File "${BIN_DIR}\libEGL.dll"
  File "${BIN_DIR}\libGLESv2.dll"

  ; Create shortcuts
  CreateDirectory "$SMPROGRAMS\XCSoar"
  CreateShortCut "$SMPROGRAMS\XCSoar\XCSoar.lnk" "$INSTDIR\XCSoar.exe"
  CreateShortCut "$DESKTOP\XCSoar.lnk" "$INSTDIR\XCSoar.exe"

  ; Write uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  ; Write registry keys for Add/Remove Programs
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoRepair" 1
SectionEnd

; Uninstaller Section
Section "Uninstall"
  ; Remove files
  Delete "$INSTDIR\XCSoar.exe"
  Delete "$INSTDIR\libEGL.dll"
  Delete "$INSTDIR\libGLESv2.dll"
  Delete "$INSTDIR\Uninstall.exe"

  ; Remove shortcuts
  Delete "$SMPROGRAMS\XCSoar\XCSoar.lnk"
  Delete "$DESKTOP\XCSoar.lnk"
  RMDir "$SMPROGRAMS\XCSoar"

  ; Remove installation directory
  RMDir "$INSTDIR"

  ; Remove registry keys
  DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"

  ; Display completion message
  MessageBox MB_OK "XCSoar has been successfully removed from your computer."
SectionEnd
