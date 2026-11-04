; Build using scripts/build-installer.ps1; no network access at install time.
[Setup]
AppId={{3719C10E-98BF-4B9E-A4F2-D3CEBC8FB692}
AppName=潮汐拼音
AppVersion=0.5.0-preview
AppPublisher=潮汐拼音
DefaultDirName=D:\ChaoxiPinyin
DefaultGroupName=潮汐拼音
DisableDirPage=no
DisableProgramGroupPage=yes
OutputDir=..\build
OutputBaseFilename=ChaoxiPinyin-0.5.0-preview-win-x64-Setup
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
MinVersion=10.0
PrivilegesRequired=admin
WizardStyle=modern
SetupIconFile=..\assets\chaoxi.ico
Compression=lzma2
SolidCompression=yes
CloseApplications=no
RestartApplications=no
UninstallDisplayName=潮汐拼音 ChaoxiPinyin
UninstallDisplayIcon={app}\ChaoxiPinyinSettings.exe
SetupLogging=yes

[Languages]
Name: "chinesesimplified"; MessagesFile: "ChineseSimplified.isl"

[Files]
Source: "..\dist\ChaoxiPinyin.dll"; DestDir: "{app}"; Flags: ignoreversion regserver uninsrestartdelete
Source: "..\dist\ChaoxiPinyinBroker.exe"; DestDir: "{app}"; Flags: ignoreversion uninsrestartdelete
Source: "..\dist\ChaoxiPinyinSettings.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\rime.dll"; DestDir: "{app}"; Flags: ignoreversion uninsrestartdelete
Source: "..\dist\data\*"; DestDir: "{app}\data"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\dist\licenses\*"; DestDir: "{app}\licenses"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\dist\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\THIRD_PARTY.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\潮汐拼音设置"; Filename: "{app}\ChaoxiPinyinSettings.exe"
Name: "{group}\卸载潮汐拼音"; Filename: "{uninstallexe}"

[UninstallDelete]
Type: files; Name: "{app}\chaoxipinyin-install.marker"

[Code]
function GetFileAttributesW(Name: String): Cardinal;
  external 'GetFileAttributesW@kernel32.dll stdcall';

function ValidateDirectory(): String;
var
  Target, Parent, Previous: String;
  Found: TFindRec;
  NonEmpty, RegisteredHere: Boolean;
begin
  Result := '';
  RegisteredHere := False;
  Target := RemoveBackslashUnlessRoot(ExpandFileName(WizardDirValue));
  if (Length(Target) <= 3) or (Copy(Target, 2, 2) <> ':\') then begin
    Result := '请选择本地磁盘中的专用文件夹，不能安装到磁盘根目录。'; exit;
  end;
  Parent := Target;
  while Length(Parent) > 3 do begin
    if (GetFileAttributesW(Parent) <> $FFFFFFFF) and
       ((GetFileAttributesW(Parent) and $400) <> 0) then begin
      Result := '安装路径不能包含目录链接，请选择普通文件夹。'; exit;
    end;
    Parent := ExtractFileDir(Parent);
  end;
  if RegQueryStringValue(HKLM, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{3719C10E-98BF-4B9E-A4F2-D3CEBC8FB692}_is1', 'InstallLocation', Previous) then begin
    if CompareText(RemoveBackslashUnlessRoot(Previous), Target) <> 0 then begin
      Result := '请先卸载潮汐拼音，再更换安装目录。'; exit;
    end;
    RegisteredHere := FileExists(AddBackslash(Target) + 'ChaoxiPinyin.dll');
  end;
  NonEmpty := False;
  if FindFirst(AddBackslash(Target) + '*', Found) then begin
    try
      repeat
        if (Found.Name <> '.') and (Found.Name <> '..') then NonEmpty := True;
      until not FindNext(Found);
    finally
      FindClose(Found);
    end;
  end;
  if NonEmpty and not RegisteredHere and not FileExists(AddBackslash(Target) + 'chaoxipinyin-install.marker') then
    Result := '这个文件夹已有其他文件，请选择空文件夹。';
end;

function NextButtonClick(CurPageID: Integer): Boolean;
var
  Error: String;
begin
  Result := True;
  if CurPageID <> wpSelectDir then exit;
  Error := ValidateDirectory();
  if Error <> '' then begin
    MsgBox(Error, mbError, MB_OK); Result := False;
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  { Validate again at the actual installation boundary, including silent mode. }
  Result := ValidateDirectory();
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
    SaveStringToFile(ExpandConstant('{app}\chaoxipinyin-install.marker'), 'ChaoxiPinyin 0.5.0', False);
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageID = wpFinished then
    WizardForm.FinishedLabel.Caption := '潮汐拼音已安装。按 Win + 空格选择“潮汐拼音”，即可开始使用。';
end;

function InitializeUninstall(): Boolean;
begin
  Result := True;
  if not UninstallSilent then
    Result := MsgBox('请先切换到其他输入法。学习数据将保留，可在潮汐拼音设置中清除。继续卸载？', mbConfirmation, MB_YESNO) = IDYES;
end;
