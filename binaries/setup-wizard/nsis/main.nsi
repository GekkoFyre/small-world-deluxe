#
#     __                 _ _   __    __           _     _ 
#    / _\_ __ ___   __ _| | | / / /\ \ \___  _ __| | __| |
#    \ \| '_ ` _ \ / _` | | | \ \/  \/ / _ \| '__| |/ _` |
#    _\ \ | | | | | (_| | | |  \  /\  / (_) | |  | | (_| |
#    \__/_| |_| |_|\__,_|_|_|   \/  \/ \___/|_|  |_|\__,_|
#                                                         
#                  ___     _                              
#                 /   \___| |_   ___  _____               
#                / /\ / _ \ | | | \ \/ / _ \              
#               / /_//  __/ | |_| |>  <  __/              
#              /___,' \___|_|\__,_/_/\_\___|              
#                                                                 
#
#   If you have downloaded the source code for "Small World Deluxe" and are reading this,
#   then thank you from the bottom of our hearts for making use of our hard work, sweat
#   and tears in whatever you are implementing this into!
#
#   Copyright (C) 2020. GekkoFyre.
#
#   Small World Deluxe is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Small World is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Small World Deluxe.  If not, see <http://www.gnu.org/licenses/>.
#
#
#   The latest source code updates can be obtained from [ 1 ] below at your
#   discretion. A web-browser or the 'git' application may be required.
#
#   [ 1 ] - https://code.gekkofyre.io/amateur-radio/small-world-deluxe
#

# Start
# -------------------------------

  !define MUI_PRODUCT "Small World Deluxe"
  !define MUI_FILE "savefile"
  !define MUI_VERSION "0.0.1-pre-alpha"
  !define MUI_BRANDINGTEXT "GekkoFyre Networks"
  CRCCheck On
 
  # We should test if we must use an absolute path 
  !include "${NSISDIR}\Contrib\Modern UI\System.nsh

# General
# -------------------------------

  OutFile "setup-smallworld-0.0.1-pre-alpha.exe"
  ShowInstDetails "nevershow"
  ShowUninstDetails "nevershow"
  SetCompressor "/FINAL lzma"
 
  !define MUI_ICON "icon.ico"
  !define MUI_UNICON "icon.ico"
  !define MUI_SPECIALBITMAP "src/contrib/images/vector/gekkofyre-networks/rionquosue/logo_blank_border_text_square_rionquosue.bmp"

# Folder selection page
# -------------------------------

  InstallDir "$PROGRAMFILES\${MUI_BRANDINGTEXT}\${MUI_PRODUCT}"

# Modern UI Configuration
# -------------------------------

  !define MUI_WELCOMEPAGE
  !define MUI_LICENSEPAGE
  !define MUI_DIRECTORYPAGE
  !define MUI_ABORTWARNING
  !define MUI_UNINSTALLER
  !define MUI_UNCONFIRMPAGE
  !define MUI_FINISHPAGE

# Language
# -------------------------------

  !insertmacro MUI_LANGUAGE "English"

# Modern UI System
# -------------------------------

  !insertmacro MUI_SYSTEM 

# Data
# -------------------------------

  LicenseData "Data.txt"

# Installer Sections
# -------------------------------

Section "install" Installation info
 
  # Add files
  SetOutPath "$INSTDIR"
 
  File "${MUI_FILE}.exe"
  File "${MUI_FILE}.ini"
  File "Data.txt"
  SetOutPath "$INSTDIR\playlists"
  file "playlists\${MUI_FILE}.epp"
  SetOutPath "$INSTDIR\data"
  file "data\*.cst"
  file "data\errorlog.txt"
  # Here follow the files that will be in the playlist
  SetOutPath "$INSTDIR"  
  file /r mpg
  SetOutPath "$INSTDIR"  
  file /r xtras  
 
  # Create desktop shortcut
  CreateShortCut "$DESKTOP\${MUI_PRODUCT}.lnk" "$INSTDIR\${MUI_FILE}.exe" ""
 
  # Create start-menu items
  CreateDirectory "$SMPROGRAMS\${MUI_PRODUCT}"
  CreateShortCut "$SMPROGRAMS\${MUI_PRODUCT}\Uninstall.lnk" "$INSTDIR\Uninstall.exe" "" "$INSTDIR\Uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\${MUI_PRODUCT}\${MUI_PRODUCT}.lnk" "$INSTDIR\${MUI_FILE}.exe" "" "$INSTDIR\${MUI_FILE}.exe" 0
 
  # Write uninstall information to the registry
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_BRANDINGTEXT}\${MUI_PRODUCT}" "DisplayName" "${MUI_PRODUCT} (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_BRANDINGTEXT}\${MUI_PRODUCT}" "UninstallString" "$INSTDIR\Uninstall.exe"
 
  WriteUninstaller "$INSTDIR\Uninstall.exe"
 
SectionEnd

# Uninstaller Section
# -------------------------------

Section "Uninstall"
 
  # Delete Files 
  RMDir /r "$INSTDIR\*.*"    
 
  # Remove the installation directory
  RMDir "$INSTDIR"
 
  # Delete Start Menu Shortcuts
  Delete "$DESKTOP\${MUI_PRODUCT}.lnk"
  Delete "$SMPROGRAMS\${MUI_PRODUCT}\*.*"
  RmDir  "$SMPROGRAMS\${MUI_PRODUCT}"
 
  # Delete Uninstaller And Unistall Registry Entries
  DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${MUI_BRANDINGTEXT}\${MUI_PRODUCT}"
  DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_BRANDINGTEXT}\${MUI_PRODUCT}"  
 
SectionEnd

# MessageBox Section
# -------------------------------

# Function that calls a messagebox when installation finished correctly
Function .onInstSuccess
  MessageBox MB_OK "You have successfully installed ${MUI_PRODUCT} by ${MUI_BRANDINGTEXT}. Use the Desktop or Start Menu shortcut to quickly and easily start the application."
FunctionEnd
 
Function un.onUninstSuccess
  MessageBox MB_OK "You have successfully uninstalled ${MUI_PRODUCT} by ${MUI_BRANDINGTEXT}."
FunctionEnd


;eof