SET warlockDir=%WARLOCK%

SET buildToolExe=%warlockDir%\BuildTool\bin\Release\HEv3-BuildTool.exe

call "%buildToolExe%" RegisterExtModule %warlockDir% %~dp0

pause