[Setup]
; App
AppName=KeeperFX
AppVerName=KeeperFX
DefaultDirName={sd}\Games\KeeperFX
DefaultGroupName=KeeperFX
; Where to build the installer
OutputDir=release\win64
OutputBaseFilename=keeperfx-web-installer
; Compression
Compression=lzma2
SolidCompression=yes
; Launcher is 64bit
ArchitecturesAllowed=x64os
; Ensure no admin rights are needed
PrivilegesRequired=lowest
; Allow users to change the installation directory
DisableDirPage=no
; Skip the final "Click to finish setup" screen
DisableFinishedPage=yes

[Files]
Source: "release\win64\keeperfx-launcher-qt.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "release\win64\*.dll"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\KeeperFX"; Filename: "{app}\keeperfx-launcher-qt.exe"

; Run the custom installer of the launcher
; We do it like this to provide a 'web installer'
[Run]
Filename: "{app}\keeperfx-launcher-qt.exe"; Parameters: "--install"; Flags: nowait shellexec

; Remove complete KeeperFX directory on uninstall
; Reason being that the launcher downloads KeeperFX and the uninstaller would otherwise only uninstall the launcher
[UninstallDelete]
Type: filesandordirs; Name: "{app}"