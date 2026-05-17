#define MyAppName "Quarantine Engine"
#define MyAppPublisher "Trifido"
#define MyAppExeName "Quarantine Engine.exe"

#ifndef MyAppVersion
  #define MyAppVersion "0.1.0-dev"
#endif

#ifndef MySourceDir
  #error "MySourceDir must point to the portable launcher app directory."
#endif

#ifndef MyOutputDir
  #error "MyOutputDir must point to the installer output directory."
#endif

#ifndef MyIconPath
  #define MyIconPath ""
#endif

[Setup]
AppId={{8B6B8CC0-BD86-44B4-9D84-65A20F74FB3A}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\Quarantine Engine
DefaultGroupName=Quarantine Engine
DisableProgramGroupPage=yes
AllowNoIcons=yes
LicenseFile=
OutputDir={#MyOutputDir}
OutputBaseFilename=QuarantineLauncherSetup-{#MyAppVersion}
#if MyIconPath != ""
SetupIconFile={#MyIconPath}
#endif
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional icons:"; Flags: unchecked

[Files]
Source: "{#MySourceDir}\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion; Excludes: "launcher.settings.json"
Source: "{#MySourceDir}\launcher.settings.json"; DestDir: "{app}"; Flags: onlyifdoesntexist

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent
