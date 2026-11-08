@echo off
setlocal
if "%~3"=="" (
 echo Usage: prepare-local-bundle.bat "FLSPro.dll" "Bank.sf2" "Candidate.vst3"
 echo DO NOT run torrent DLLs outside an isolated disposable Windows VM.
 exit /b 2
)
if not exist "%~1" (echo DLL missing&exit /b 3)
if not exist "%~2" (echo SoundFont missing&exit /b 3)
if not exist "%~3\Contents\Resources\125A-Fruity-Helper32.exe" (
 echo Missing x86 helper in destination bundle
 exit /b 4
)
copy /y "%~1" "%~3\Contents\Resources\FLSPro.dll" >nul || exit /b 5
copy /y "%~2" "%~3\Contents\Resources\SynthBank.sf2" >nul || exit /b 6
echo Development bundle prepared; never distribute it with the proprietary DLL.
exit /b 0
