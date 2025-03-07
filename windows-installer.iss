[Setup]
; App
AppName=KeeperFX
AppVersion=1.0
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
; Skip the final "Click to finish setup" screen
DisableFinishedPage=yes

[Files]
Source: "release\win64\keeperfx-launcher-qt.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "release\win64\7za.dll"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\KeeperFX"; Filename: "{app}\keeperfx-launcher-qt.exe"

[Run]
Filename: "{app}\keeperfx-launcher-qt.exe"; Parameters: "--install"; Flags: nowait shellexec
