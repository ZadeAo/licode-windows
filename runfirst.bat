@echo off
%1 mshta vbscript:CreateObject("Shell.Application").ShellExecute("cmd.exe","/c %~s0 ::","","runas",1)(window.close)&&exit

echo 添加SFUChat环境变量
set regpath=HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Environment
set evname=SFUChat
set evnpath=%~dp0
reg add "%regpath%" /v %evname% /d %evnpath% /f
set evname=WEBRTC13
set evnpath=%~dp0ThirdParty\webrtc\src\
reg add "%regpath%" /v %evname% /d %evnpath% /f
pause>nul