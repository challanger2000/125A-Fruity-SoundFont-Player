@echo off
setlocal EnableExtensions
REM Run only in an isolated disposable Windows test machine.
powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "%~dp0Run-Original-Engine-VST3-Lab.ps1" -OriginalZip "%~dp0Fruity Soundfont Player.zip" -Vst3Bundle "%~dp0125A Fruity SoundFont Player.vst3" -TestLabHost "%~dp0125A_Test_Lab_Host.exe" -OutputDirectory "%~dp0LAB-ERGEBNIS" -ApproveExecution
set "RC=%ERRORLEVEL%"
echo.
echo Report in LAB-ERGEBNIS\VST3-TEST-ERGEBNIS.txt
exit /b %RC%
