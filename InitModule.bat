SET warlockDir=%WARLOCK%

SET buildToolExe=%warlockDir%\WarBuild\warbuild.bat

call "%buildToolExe%" RegisterExtModule -root "%warlockDir%" %~dp0

pause
