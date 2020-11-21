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

# Include Modern UI
# -------------------------------
  !include "${NSISDIR}\Contrib\Modern UI 2\MUI2.nsh"

# Start
# -------------------------------

  !define MUI_PRODUCT "Small World Deluxe"
  !define MUI_FILE "smallworld"
  !define MUI_VERSION "0.0.1-pre-alpha"
  !define MUI_BRANDINGTEXT "GekkoFyre Networks"
  !define GK_ROOT_PATH ".\..\..\.."
  
  CRCCheck On

# General
# -------------------------------

  # Name and file
  Name "${MUI_PRODUCT} v${MUI_VERSION}"
  OutFile "setup-smallworld-${MUI_VERSION}.exe"
  Unicode True
  
  ShowInstDetails "nevershow"
  ShowUninstDetails "nevershow"
  SetCompressor "/FINAL lzma"
 
  # Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\${MUI_BRANDINGTEXT}\${MUI_PRODUCT}" ""
  
  # Request application privileges for Windows Vista
  RequestExecutionLevel user
 
  !define MUI_ICON "icon.ico"
  !define MUI_UNICON "icon.ico"
  !define MUI_SPECIALBITMAP "${GK_ROOT_PATH}\src\contrib\images\vector\gekkofyre-networks\rionquosue\logo_blank_border_text_square_rionquosue.bmp"

# Folder selection page
# -------------------------------

  # Default installation folder
  InstallDir "$PROGRAMFILES\${MUI_BRANDINGTEXT}\${MUI_PRODUCT}"

# Interface Configuration
# -------------------------------

  !define MUI_WELCOMEPAGE
  !define MUI_LICENSEPAGE
  !define MUI_DIRECTORYPAGE
  !define MUI_HEADERIMAGE
  !define MUI_HEADERIMAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Header\nsis.bmp" # This is optional...
  !define MUI_ABORTWARNING
  !define MUI_UNINSTALLER
  !define MUI_UNCONFIRMPAGE
  !define MUI_FINISHPAGE

# Pages
# -------------------------------

  !insertmacro MUI_PAGE_LICENSE "${GK_ROOT_PATH}\LICENSE"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES

  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

# Languages
# -------------------------------

  !insertmacro MUI_LANGUAGE "English" # The first language is the default language...
  !insertmacro MUI_LANGUAGE "French"
  !insertmacro MUI_LANGUAGE "German"
  !insertmacro MUI_LANGUAGE "Spanish"
  !insertmacro MUI_LANGUAGE "SpanishInternational"
  !insertmacro MUI_LANGUAGE "SimpChinese"
  !insertmacro MUI_LANGUAGE "TradChinese"
  !insertmacro MUI_LANGUAGE "Japanese"
  !insertmacro MUI_LANGUAGE "Korean"
  !insertmacro MUI_LANGUAGE "Italian"
  !insertmacro MUI_LANGUAGE "Dutch"
  !insertmacro MUI_LANGUAGE "Danish"
  !insertmacro MUI_LANGUAGE "Swedish"
  !insertmacro MUI_LANGUAGE "Norwegian"
  !insertmacro MUI_LANGUAGE "NorwegianNynorsk"
  !insertmacro MUI_LANGUAGE "Finnish"
  !insertmacro MUI_LANGUAGE "Greek"
  !insertmacro MUI_LANGUAGE "Russian"
  !insertmacro MUI_LANGUAGE "Portuguese"
  !insertmacro MUI_LANGUAGE "PortugueseBR"
  !insertmacro MUI_LANGUAGE "Polish"
  !insertmacro MUI_LANGUAGE "Ukrainian"
  !insertmacro MUI_LANGUAGE "Czech"
  !insertmacro MUI_LANGUAGE "Slovak"
  !insertmacro MUI_LANGUAGE "Croatian"
  !insertmacro MUI_LANGUAGE "Bulgarian"
  !insertmacro MUI_LANGUAGE "Hungarian"
  !insertmacro MUI_LANGUAGE "Thai"
  !insertmacro MUI_LANGUAGE "Romanian"
  !insertmacro MUI_LANGUAGE "Latvian"
  !insertmacro MUI_LANGUAGE "Macedonian"
  !insertmacro MUI_LANGUAGE "Estonian"
  !insertmacro MUI_LANGUAGE "Turkish"
  !insertmacro MUI_LANGUAGE "Lithuanian"
  !insertmacro MUI_LANGUAGE "Slovenian"
  !insertmacro MUI_LANGUAGE "Serbian"
  !insertmacro MUI_LANGUAGE "SerbianLatin"
  !insertmacro MUI_LANGUAGE "Arabic"
  !insertmacro MUI_LANGUAGE "Farsi"
  !insertmacro MUI_LANGUAGE "Hebrew"
  !insertmacro MUI_LANGUAGE "Indonesian"
  !insertmacro MUI_LANGUAGE "Mongolian"
  !insertmacro MUI_LANGUAGE "Luxembourgish"
  !insertmacro MUI_LANGUAGE "Albanian"
  !insertmacro MUI_LANGUAGE "Breton"
  !insertmacro MUI_LANGUAGE "Belarusian"
  !insertmacro MUI_LANGUAGE "Icelandic"
  !insertmacro MUI_LANGUAGE "Malay"
  !insertmacro MUI_LANGUAGE "Bosnian"
  !insertmacro MUI_LANGUAGE "Kurdish"
  !insertmacro MUI_LANGUAGE "Irish"
  !insertmacro MUI_LANGUAGE "Uzbek"
  !insertmacro MUI_LANGUAGE "Galician"
  !insertmacro MUI_LANGUAGE "Afrikaans"
  !insertmacro MUI_LANGUAGE "Catalan"
  !insertmacro MUI_LANGUAGE "Esperanto"
  !insertmacro MUI_LANGUAGE "Asturian"
  !insertmacro MUI_LANGUAGE "Basque"
  !insertmacro MUI_LANGUAGE "Pashto"
  !insertmacro MUI_LANGUAGE "ScotsGaelic"
  !insertmacro MUI_LANGUAGE "Georgian"
  !insertmacro MUI_LANGUAGE "Vietnamese"
  !insertmacro MUI_LANGUAGE "Welsh"
  !insertmacro MUI_LANGUAGE "Armenian"
  !insertmacro MUI_LANGUAGE "Corsican"
  !insertmacro MUI_LANGUAGE "Tatar"
  !insertmacro MUI_LANGUAGE "Hindi"

# Reserve Files
# -------------------------------

  # If you are using solid compression, files that are required before
  # the actual installation should be stored first in the data block,
  # because this will make your installer start faster.
  
  !insertmacro MUI_RESERVEFILE_LANGDLL

# Installer Functions
# -------------------------------

  !insertmacro MUI_LANGDLL_DISPLAY

# Modern UI System
# -------------------------------

  !insertmacro MUI_SYSTEM 

# Installer Sections
# -------------------------------

Section "install" Installation info
 
  # Add files
  SetOutPath "$INSTDIR"
  File "${GK_ROOT_PATH}\cmake-build-debug\libgcc_s_seh-1.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\libglib-2.0-0.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\libhamlib-2.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\libusb-1.0.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\libwinpthread-1.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\Qt5Core.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\Qt5Gui.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\Qt5Multimedia.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\Qt5Network.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\Qt5OpenGL.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\Qt5SerialPort.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\Qt5Svg.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\Qt5TextToSpeech.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\Qt5Widgets.dll"
 
  File "${GK_ROOT_PATH}\cmake-build-debug\${MUI_FILE}.exe"
  File "${GK_ROOT_PATH}\cmake-build-debug\libgalaxy.a"
  
  SetOutPath "$INSTDIR\audio"
  File "${GK_ROOT_PATH}\cmake-build-debug\audio\qtaudio_windows.dll"
  
  SetOutPath "$INSTDIR\imageformats"
  File "${GK_ROOT_PATH}\cmake-build-debug\imageformats\qgif.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\imageformats\qicns.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\imageformats\qico.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\imageformats\qjp2.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\imageformats\qjpeg.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\imageformats\qsvg.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\imageformats\qtga.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\imageformats\qtiff.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\imageformats\qwbmp.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\imageformats\qwebp.dll"
  
  SetOutPath "$INSTDIR\mediaservice"
  File "${GK_ROOT_PATH}\cmake-build-debug\mediaservice\dsengine.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\mediaservice\qtmedia_audioengine.dll"
  
  SetOutPath "$INSTDIR\platforms"
  File "${GK_ROOT_PATH}\cmake-build-debug\platforms\qdirect2d.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\platforms\qminimal.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\platforms\qoffscreen.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\platforms\qwebgl.dll"
  File "${GK_ROOT_PATH}\cmake-build-debug\platforms\qwindows.dll"
  
  SetOutPath "$INSTDIR\styles"
  File "${GK_ROOT_PATH}\cmake-build-debug\styles\qwindowsvistastyle.dll"

  # Store installation folder
  WriteRegStr HKCU "Software\${MUI_BRANDINGTEXT}\${MUI_PRODUCT}" "" $INSTDIR

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
  DeleteRegKey /ifempty HKCU "Software\${MUI_BRANDINGTEXT}\${MUI_PRODUCT}"
  DeleteRegKey /ifempty HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_BRANDINGTEXT}\${MUI_PRODUCT}"  
 
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

# Uninstaller Functions
# -------------------------------

Function un.onInit

  !insertmacro MUI_UNGETLANGUAGE
  
FunctionEnd

;eof