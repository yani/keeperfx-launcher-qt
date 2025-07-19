[Setup]
; App
AppName=KeeperFX
AppVerName=KeeperFX
; Installation directory
; {sd} -> System Drive: The drive Windows is installed on, typically "C:"
DefaultDirName={sd}\Games\KeeperFX
; Start Menu group name
DefaultGroupName=KeeperFX
; Installer build output
OutputDir=release\win64
OutputBaseFilename=keeperfx-web-installer
; Compression
Compression=lzma2
SolidCompression=yes
; Force 64 bit support because the launcher is 64bit
ArchitecturesAllowed=x64os
; Ensure no admin rights are needed
PrivilegesRequired=lowest
; Allow users to change the installation directory
DisableDirPage=no
; Skip the final "Click to finish setup" screen
DisableFinishedPage=yes
; Add extra bytes to the calculated total size (because we download KeeperFX ourselves)
ExtraDiskSpaceRequired=640329910
; Show "Select Setup Language" dialog
ShowLanguageDialog=yes

[Files]
Source: "release\win64\keeperfx-launcher-qt.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "release\win64\*.dll"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\KeeperFX"; Filename: "{app}\keeperfx-launcher-qt.exe"
Name: "{autodesktop}\KeeperFX"; Filename: "{app}\keeperfx-launcher-qt.exe";

; Run the custom installer of the launcher
; We do it like this to provide a 'web installer'
[Run]
Filename: "{app}\keeperfx-launcher-qt.exe"; Parameters: "--install --language={language}"; Flags: nowait shellexec

; Remove complete KeeperFX directory on uninstall
; Reason being that the launcher downloads KeeperFX and the uninstaller would otherwise only uninstall the launcher
[UninstallDelete]
Type: filesandordirs; Name: "{app}"

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "nl"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "it"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"
Name: "es"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "de"; MessagesFile: "compiler:Languages\German.isl"
Name: "pl"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "sv"; MessagesFile: "compiler:Languages\Swedish.isl"
Name: "ja"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "ko"; MessagesFile: "compiler:Languages\Korean.isl"
Name: "cs"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "uk"; MessagesFile: "compiler:Languages\Ukrainian.isl"
Name: "zh_Hans"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
Name: "zh_Hant"; MessagesFile: "compiler:Languages\ChineseTraditional.isl"
