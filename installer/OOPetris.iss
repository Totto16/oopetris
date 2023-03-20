; Defines
#define AppVersion                "1.0.0"
#define AppVersionInfoVersion     "1.0.0"
#define AppName                   "OOPetris"
#define AppPublisher              "mgerhold"
#define AppURL                    "https://github.com/mgerhold/oopetris"
#define MyAppId                     "{{88C6A6D9-324C-46E8-BA87-563D14021442}"
#define MyAppPath                   "{pf}\Superb\WorldSavingApp"

// https://www.c-sharpcorner.com/UploadFile/tunturu/installer-using-inno-setup/
// https://jrsoftware.org/ishelp/
// https://www.meandmark.com/blog/2012/07/creating-a-windows-installer-with-inno-setup/
// https://flowerinthenight.com/blog/2017/02/28/simple-innosetup

[Setup]
AlwaysRestart = No
UninstallRestartComputer = No
CloseApplications = No
RestartApplications = No
SetupIconFile = .\logo.ico
DisableReadyPage = Yes
UninstallDisplayName = {#AppName}
UninstallDisplayIcon = {app}\logo.ico
ArchitecturesInstallIn64BitMode = x64
ArchitecturesAllowed = x64
AppId = {#MyAppId}
AppName = {#AppName}
AppVerName = {#AppName} {#AppVersion}
AppVersion = {#AppVersion}
AppPublisher = {#AppPublisher}
DefaultDirName = {#MyAppPath}
DisableDirPage = Yes
DefaultGroupName = Superb
DisableProgramGroupPage = Yes
OutputDir = Installer
OutputBaseFilename = SuperbWorldSavingApp_{#AppVersion}
SetupLogging = Yes
SolidCompression = Yes
VersionInfoDescription = {#AppName}
VersionInfoProductVersion = {#AppVersion}
VersionInfoProductName = {#AppName}
VersionInfoVersion = {#AppVersionInfoVersion}
VersionInfoCompany = {#AppPublisher}
VersionInfoTextVersion = {#AppVersion}


[Files]
; x64
Source: .\bin\x64\Release\superb.exe; DestDir: {code:InstallDir}; Flags: ignoreversion restartreplace uninsrestartdelete
Source: .\bin\x64\Release\superb.exe.config; DestDir: {code:InstallDir}; Flags: ignoreversion restartreplace uninsrestartdelete
Source: .\logo.ico; DestDir: {code:InstallDir}; Flags: ignoreversion restartreplace uninsrestartdelete

[Icons]

[Registry]

[Run]
; Install app.
Filename: {code:InstallDir}\superb.exe; Parameters: --install; Flags: waituntilterminated runhidden
Filename: cmd.exe; Parameters: /c net start superb; Flags: waituntilterminated runhidden

[UninstallRun]
; Uninstall service
Filename: cmd.exe; Parameters: /c net stop superb; Flags: waituntilterminated runhidden
Filename: {code:InstallDir}\superb.exe; Parameters: --uninstall; Flags: waituntilterminated runhidden

[Dirs]
Name: {app}; Flags: uninsalwaysuninstall

[InstallDelete]

[UninstallDelete]
Type: filesandordirs; Name: {code:InstallDir}\InstallUtil.InstallLog
Type: filesandordirs; Name: {code:InstallDir}\superb.InstallLog

[Code]
var
    strInstallPath: String;
    bInstallPathCached: Boolean;
    strCheckSumBefore: String;
    strCheckSumAfter: String;
    bInstallSuccess: Boolean;
// Return install directory (default or custom). Caller can specify destination directory via /DIR="<dir>" paramerter to setup.exe installer.
function InstallDir(param: String): String;
var
    strParamTail: String;
    strDirTail: String;
    strPfEnv: String;
    strInstallDirTemp: String;
begin
    if bInstallPathCached = false then
    begin
        strInstallPath := '{#SetupSetting("DefaultDirName")}';
        strPfEnv := ExpandConstant('{%ProgramFiles}')
        StringChange(strInstallPath, '{pf}', strPfEnv);
        // Do not use ..(x86) dir in x64 bit systems by default. Note that this will have no meaning if user will input
        // custom path in installation.
        StringChange(strInstallPath, ' (x86)', '');
        strParamTail := GetCmdTail();
        strDirTail := Copy(strParamTail, Pos('/DIR=', strParamTail), Length(strParamTail));
        if Pos('/DIR=', strDirTail) = 1 then
        begin
            strInstallPath := '';
            strInstallDirTemp := Copy(strDirTail, 7, Length(strDirTail));
            strInstallPath := Copy(strInstallDirTemp, 1, Pos('"', strInstallDirTemp) - 1);
        end;
        bInstallPathCached := true;
    end;
    result := strInstallPath;
end;
// Called during Setup's initialization. Return False to abort Setup, True otherwise. Note: This is an override function.
function InitializeSetup(): Boolean;
begin
    result := true;
    exit;
    // strError := SetupMessage(msgWinVersionTooLowError);
    // StringChange(strError, '%1', 'Windows');
    // StringChange(strError, '%2', '8');
    // SuppressibleMsgBox(strError, mbInformation, MB_OK, MB_OK);
    result := false;
end;
// Note: This is an override function.
//
// PrepareToInstall() override function ensures that all processes are stopped before starting installation. And for some
// reasons that installation will not succeed (or cancelled), we need to restart processes as well. We do it here.
procedure DeinitializeSetup();
var
    resultCode: Integer;
begin
    if bInstallSuccess = false then
    begin
        Exec(ExpandConstant('{sys}\net.exe'), 'start superb', '', SW_HIDE, ewWaitUntilTerminated, resultCode);
    end;
end;
// Note: This is an override function.
//
// Quit applications that needs to be overwritten during installation. Added to support upgrade to InnoSetup 5.5.3. Return
// emptry string for success, error string to cancel and display returned string.
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
    resultCode: Integer;
    strPrep: String;
begin
    strPrep := SetupMessage(msgWizardPreparing) + '...';
    WizardForm.StatusLabel.Caption := strPrep;    
    Exec(ExpandConstant('{sys}\net.exe'), 'stop superb', '', SW_HIDE, ewWaitUntilTerminated, resultCode);
    result := '';
end;
// You can use this event function to perform your own pre-install and post-install tasks. Called with CurStep=ssInstall
// just before the actual installation starts, with CurStep=ssPostInstall just after the actual installation finishes,
// and with CurStep=ssDone just before Setup terminates after a successful install.
// Note: This is an override function.
procedure CurStepChanged(CurStep: TSetupStep);
var
    resultCode: Integer;
    strPrep: String;
    bModifyRestart: Boolean;
begin
    if CurStep = ssInstall then 
    begin
        // Pre-install phase
        strPrep := SetupMessage(msgWizardPreparing) + '...';
        WizardForm.StatusLabel.Caption := strPrep;
        strCheckSumBefore := MakePendingFileRenameOperationsChecksum;
    end 
    else if CurStep = ssPostInstall then 
    begin
        // Post-install phase
        strCheckSumAfter := MakePendingFileRenameOperationsChecksum;
    end 
    else if CurStep = ssDone then 
    begin
        // Before setup terminates after a successful install.
        strCheckSumAfter := MakePendingFileRenameOperationsChecksum;
        bModifyRestart := false;
        if strCheckSumAfter = strCheckSumBefore then 
        begin
            // Only do this when InnoSetup will not require the system to be restarted.
            Exec(ExpandConstant('{sys}\net.exe'), 'start superb', '', SW_HIDE, ewWaitUntilTerminated, resultCode);
            bInstallSuccess := true;
        end
        else
        begin
            bModifyRestart := true;
        end;
    end;
end;
// You can use this event function to perform your own pre-uninstall and post-uninstall tasks. CurUninstallStep = ssInstall
// during uninstallation phase then CurUninstallStep = usPostUninstall after successful uninstallation.
// Note: This is an override function.
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
    resultCode: Integer;
begin
    if CurUninstallStep = usUninstall then 
    begin
        // Uninstall phase
        Exec(ExpandConstant('{sys}\net.exe'), 'stop superb', '', SW_HIDE, ewWaitUntilTerminated, resultCode);
    end
    else if CurUninstallStep = usPostUninstall then
    begin
        // Post-uninstall phase
        ;
    end
    else if CurUninstallStep = usDone then
    begin
        // Uninstallation done phase
        ;
    end;
end;