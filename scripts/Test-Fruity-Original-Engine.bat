@echo off
setlocal EnableExtensions DisableDelayedExpansion
REM 125A original engine probe. Run only in an isolated disposable Windows VM.
REM Use the exact verified archive layout; do not select arbitrary extracted paths.
set "ROOT=%~dp0"
set "PROBE=%ROOT%125A-Fruity-Probe32.exe"
set "SOURCEZIP=%ROOT%Fruity Soundfont Player.zip"
set "WORKDIR=%TEMP%\125A_Fruity_%RANDOM%_%RANDOM%"
set "RESULT=%ROOT%TEST_ERGEBNIS.txt"
set "WAV=%ROOT%TestOriginal.wav"
set "RC=0"
> "%RESULT%" echo 125A-Fruity-Original-Engine Test Lab - status: NOT RUN
if not exist "%PROBE%" (
  >> "%RESULT%" echo ERROR PRECHECK: probe32 executable missing
  exit /b 2
)
if not exist "%SOURCEZIP%" (
  >> "%RESULT%" echo ERROR PRECHECK: original ZIP missing
  exit /b 2
)
if exist "%WAV%" del /q "%WAV%" >nul 2>&1
mkdir "%WORKDIR%" >nul 2>&1
if errorlevel 1 (
  >> "%RESULT%" echo ERROR PRECHECK: cannot create temporary folder
  exit /b 2
)
powershell -NoProfile -NonInteractive -Command "$ErrorActionPreference='Stop'; Expand-Archive -LiteralPath $env:SOURCEZIP -DestinationPath $env:WORKDIR -Force" >> "%RESULT%" 2>&1
if errorlevel 1 (
  >> "%RESULT%" echo ERROR ZIP: extraction failed
  set "RC=3"
  goto :done
)
set "DLL=%WORKDIR%\Fruity Soundfont Player\FLSPro.dll"
set "BANK=%WORKDIR%\Fruity Soundfont Player\Data\Default.sf2"
if not exist "%DLL%" (
  >> "%RESULT%" echo ERROR ASSETS: expected FLSPro.dll not found after extraction
  set "RC=4"
  goto :done
)
if not exist "%BANK%" (
  >> "%RESULT%" echo ERROR ASSETS: expected Default.sf2 not found after extraction
  set "RC=4"
  goto :done
)
>> "%RESULT%" echo CHECK ZIP=OK DLL=OK BANK=OK
"%PROBE%" "%DLL%" "%BANK%" "%WAV%" >> "%RESULT%" 2>&1
set "RC=%ERRORLEVEL%"
if not "%RC%"=="0" (
  >> "%RESULT%" echo FAIL ORIGINAL ENGINE: probe exit code %RC%
  goto :done
)
if not exist "%WAV%" (
  >> "%RESULT%" echo FAIL ORIGINAL ENGINE: probe returned zero without creating WAV
  set "RC=8"
  goto :done
)
>> "%RESULT%" echo PASS ORIGINAL ENGINE PCM PROBE
:done
if exist "%WORKDIR%" rmdir /s /q "%WORKDIR%" >nul 2>&1
exit /b %RC%
