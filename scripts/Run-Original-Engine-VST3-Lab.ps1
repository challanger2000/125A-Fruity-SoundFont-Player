[CmdletBinding()]
param(
 [string]$OriginalZip = (Join-Path $PSScriptRoot 'Fruity Soundfont Player.zip'),
 [string]$Vst3Bundle = (Join-Path $PSScriptRoot '125A Fruity SoundFont Player.vst3'),
 [string]$TestLabHost = (Join-Path $PSScriptRoot '125A_Test_Lab_Host.exe'),
 [string]$OutputDirectory = (Join-Path $PSScriptRoot 'LAB-ERGEBNIS'),
 [string]$ExpectedEngineSha256 = '4c592e1b21640bdea0bfa6f09bde6e882d88f1a9556f4e6e0e77e986fbe65a0f',
 [string]$ExpectedBankSha256 = '4a35e84de6f0198bfa92bbbee11846bcea4f34cdda1794d8d935e3870a37757e',
 [switch]$ApproveExecution
)
# This runs a user-provided, untrusted native DLL. Disposable isolated Windows VM ONLY.
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$realEngine='4c592e1b21640bdea0bfa6f09bde6e882d88f1a9556f4e6e0e77e986fbe65a0f'
$realBank='4a35e84de6f0198bfa92bbbee11846bcea4f34cdda1794d8d935e3870a37757e'
$OutputDirectory=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$log=[System.Collections.Generic.List[string]]::new()
$log.Add('125A Fruity original 32-bit DLL via real 64-bit VST3 and 125A Test Lab')
$log.Add('STATUS=STARTED')
$log.Add('RELEASE_APPROVED=NO')
$work=Join-Path $OutputDirectory 'temporary-vst3'
function Write-Result([string]$status,[string]$detail) {
 $log[1]="STATUS=$status"
 $log.Add("DETAIL=$detail")
 $file=Join-Path $OutputDirectory 'VST3-TEST-ERGEBNIS.txt'
 [IO.File]::WriteAllLines($file,$log,[Text.UTF8Encoding]::new($false))
 Write-Host ($log -join [Environment]::NewLine)
}
function Require-File([string]$path,[string]$label) {
 if(-not(Test-Path -LiteralPath $path -PathType Leaf)) {throw "Missing $label : $path"}
}
function Native-Render([string]$bundle,[string]$wave) {
 $psi=[Diagnostics.ProcessStartInfo]::new()
 $psi.FileName=[IO.Path]::GetFullPath($TestLabHost)
 # Windows PowerShell 5.1 uses .NET Framework (no ProcessStartInfo.ArgumentList).
 # Windows paths cannot contain double-quotes; quote both arguments.
 $psi.Arguments = '--render "' + $bundle + '" "' + $wave + '"'
 $psi.RedirectStandardOutput=$true
 $psi.RedirectStandardError=$true
 $psi.UseShellExecute=$false
 $psi.CreateNoWindow=$true
 $proc=[Diagnostics.Process]::new()
 $proc.StartInfo=$psi
 try {
  if(-not $proc.Start()) {throw 'Lab host could not start'}
  if(-not $proc.WaitForExit(90000)) {
   $proc.Kill()
   throw 'Lab host timed out'
  }
  $out=$proc.StandardOutput.ReadToEnd()
  $err=$proc.StandardError.ReadToEnd()
  $log.Add('LAB_HOST_EXIT='+$proc.ExitCode)
  $log.Add('LAB_LOG='+($out+$err).Replace([Environment]::NewLine,' | '))
  if($proc.ExitCode -ne 0){throw 'Native Test Lab host returned nonzero'}
 }finally{$proc.Dispose()}
}
function Count-PCM([string]$wave){
 Require-File $wave 'rendered WAV'
 $b=[IO.File]::ReadAllBytes($wave)
 if($b.Length -lt 1800044 -or
    [Text.Encoding]::ASCII.GetString($b,0,4) -ne 'RIFF' -or
    [Text.Encoding]::ASCII.GetString($b,8,4) -ne 'WAVE' -or
    [BitConverter]::ToUInt16($b,20) -ne 1 -or
    [BitConverter]::ToUInt16($b,22) -ne 2 -or
    [BitConverter]::ToUInt32($b,24) -ne 44100 -or
    [BitConverter]::ToUInt16($b,34) -ne 24 -or
    [Text.Encoding]::ASCII.GetString($b,36,4) -ne 'data'){
   throw 'Lab produced incomplete/non-PCM24 stereo 44.1kHz WAV'
 }
 $bytes=[BitConverter]::ToUInt32($b,40)
 if($bytes -gt ($b.Length-44) -or $bytes -lt 1800000){throw 'Invalid PCM payload size'}
 $n=0
 for($i=44;$i -lt 44+[int]$bytes;$i++){if($b[$i] -ne 0){$n++}}
 return $n
}
try {
 if($env:OS -ne 'Windows_NT'){throw 'This test requires Windows'}
 if(-not $ApproveExecution){throw 'Explicit -ApproveExecution required on an isolated Windows test machine'}
 Require-File $OriginalZip 'original user ZIP'
 Require-File $TestLabHost 'native 125A Test Lab host'
 if(-not(Test-Path -LiteralPath $Vst3Bundle -PathType Container)){throw 'VST3 bundle missing'}
 Require-File (Join-Path $Vst3Bundle 'Contents/x86_64-win/125A Fruity SoundFont Player.vst3') 'x64 VST3 module'
 Require-File (Join-Path $Vst3Bundle 'Contents/Resources/125A-Fruity-Helper32.exe') 'x86 helper'
 if($ExpectedEngineSha256 -notmatch '^[a-fA-F0-9]{64}$' -or $ExpectedBankSha256 -notmatch '^[a-fA-F0-9]{64}$'){throw 'Missing exact SHA256 pins'}
 $log.Add('MODULE_SHA256='+((Get-FileHash -LiteralPath (Join-Path $Vst3Bundle 'Contents/x86_64-win/125A Fruity SoundFont Player.vst3') -Algorithm SHA256).Hash.ToLowerInvariant()))
 $log.Add('HOST_SHA256='+((Get-FileHash -LiteralPath $TestLabHost -Algorithm SHA256).Hash.ToLowerInvariant()))
 Add-Type -AssemblyName System.IO.Compression.FileSystem
 if(Test-Path -LiteralPath $work){Remove-Item -LiteralPath $work -Recurse -Force}
 New-Item -ItemType Directory -Path $work -Force | Out-Null
 Copy-Item -LiteralPath $Vst3Bundle -Destination $work -Recurse -Force
 $copy=Join-Path $work (Split-Path -Leaf $Vst3Bundle)
 $resources=Join-Path $copy 'Contents/Resources'
 $zip=[IO.Compression.ZipFile]::OpenRead([IO.Path]::GetFullPath($OriginalZip))
 try{
  $engineEntry=$zip.GetEntry('Fruity Soundfont Player/FLSPro.dll')
  $bankEntry=$zip.GetEntry('Fruity Soundfont Player/Data/Default.sf2')
  if($null -eq $engineEntry -or $null -eq $bankEntry){throw 'ZIP does not contain exact original engine and SF2 paths'}
  if($engineEntry.Length -gt 10000000 -or $bankEntry.Length -gt 30000000){throw 'Archive member exceeds safety limit'}
  foreach($pair in @(@($engineEntry,(Join-Path $resources 'FLSPro.dll')), @($bankEntry,(Join-Path $resources 'SynthBank.sf2')))){
   $input=$pair[0].Open()
   try{
    $output=[IO.File]::Create([string]$pair[1])
    try{$input.CopyTo($output)}finally{$output.Dispose()}
   }finally{$input.Dispose()}
  }
 }finally{$zip.Dispose()}
 $sha=(Get-FileHash -LiteralPath (Join-Path $resources 'FLSPro.dll') -Algorithm SHA256).Hash.ToLowerInvariant()
 $bankSha=(Get-FileHash -LiteralPath (Join-Path $resources 'SynthBank.sf2') -Algorithm SHA256).Hash.ToLowerInvariant()
 if($sha -ne $ExpectedEngineSha256.ToLowerInvariant() -or $bankSha -ne $ExpectedBankSha256.ToLowerInvariant()){throw 'Original engine or bank SHA256 mismatch'}
 $identity=if($sha -eq $realEngine -and $bankSha -eq $realBank){'USER_ORIGINAL_MATCH'}else{'MOCK_ONLY'}
 $log.Add("ENGINE_IDENTITY=$identity")
 $log.Add('ENGINE_SHA256='+$sha)
 $positive=Join-Path $OutputDirectory 'VST3-Positive.wav'
 if(Test-Path -LiteralPath $positive){Remove-Item -LiteralPath $positive -Force}
 Native-Render $copy $positive
 $positiveBytes=Count-PCM $positive
 $log.Add("MIDI_RENDER_NONZERO_PCM_BYTES=$positiveBytes")
 if($positiveBytes -lt 2000){throw 'SILENT: original VST3 helper produced no MIDI-triggered sound'}
 Remove-Item -LiteralPath (Join-Path $resources 'FLSPro.dll') -Force
 Remove-Item -LiteralPath (Join-Path $resources 'SynthBank.sf2') -Force
 $negative=Join-Path $OutputDirectory 'VST3-Negative-No-Engine.wav'
 if(Test-Path -LiteralPath $negative){Remove-Item -LiteralPath $negative -Force}
 Native-Render $copy $negative
 $negativeBytes=Count-PCM $negative
 $log.Add("NO_ENGINE_NONZERO_PCM_BYTES=$negativeBytes")
 if($negativeBytes -ne 0){throw 'Negative control produced PCM without engine'}
 Write-Result 'PASS' 'Actual x64 VST3/32-bit engine audible and missing-engine silent; DAW/release NOT VERIFIED'
 exit 0
}catch{
 Write-Result 'FAIL' $_.Exception.Message
 exit 1
}finally{
 if(Test-Path -LiteralPath $work){Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue}
}
