# SPDX-License-Identifier: MIT
param([switch]$SelfTest)
$ErrorActionPreference='Stop'
function New-ExeProfile([string]$Name,$Version,[string]$Hash) {
 $matches=($Hash -eq 'f648064a90fa818912ce037af9d5c3fd6b94973a415a64862ff52b3cd23e3761')
 $identity='DIFFERENT_FROM_UPLOADED_EXE'
 if($matches){$identity='MATCHES_UPLOADED_EXE'}
 return [ordered]@{name=$Name;product=$Version.ProductName;product_version=$Version.ProductVersion;sha256=$Hash;matches_uploaded_exe=$matches;identity_status=$identity;product_caption_matches=([string]$Version.ProductName -match 'Wuthering Waves')}
}
function Convert-ModuleRecord($Module) {
 return [ordered]@{name=$Module.ModuleName;file_version=$Module.FileVersionInfo.FileVersion;product_version=$Module.FileVersionInfo.ProductVersion}
}
function Get-ModuleInventory([Diagnostics.Process]$Target) {
 $items=@($Target.Modules | ForEach-Object {Convert-ModuleRecord $_} | Sort-Object {$_.name})
 if($items.Count -eq 0){throw 'NO_MODULES_VISIBLE'}
 return ,$items
}
function Get-RuntimeSignals($Modules) {
 $names=@($Modules | ForEach-Object {$_.name})
 return [ordered]@{
  managed_runtime_modules=@($names | Where-Object {$_ -match '^(coreclr|clr|mono.*|GameAssembly|il2cpp)\.dll$'})
  javascript_runtime_modules=@($names | Where-Object {$_ -match '(puerts|v8|quickjs|javascript)'})
  interpretation='Names are diagnostic signals only. A runtime can be linked statically; absence of a DLL does not establish absence of a runtime.'
 }
}
function Test-Collector {
 $target=[Diagnostics.Process]::GetCurrentProcess()
 try {
  $modules=Get-ModuleInventory $target
  if($modules.Count -lt 1){throw 'TEST_EMPTY_INVENTORY'}
  foreach($m in $modules){if(!$m.name -or $m.name.Contains('\') -or $m.name.Contains('/')){throw 'TEST_PATH_LEAK'}}
  $signals=Get-RuntimeSignals @(@{name='coreclr.dll'},@{name='puerts.dll'},@{name='unrelated.dll'})
  if($signals.managed_runtime_modules.Count -ne 1 -or $signals.javascript_runtime_modules.Count -ne 1){throw 'TEST_RUNTIME_SIGNALS'}
  $empty=Get-RuntimeSignals @(@{name='unrelated.dll'})
  if($empty.managed_runtime_modules.Count -ne 0 -or $empty.javascript_runtime_modules.Count -ne 0){throw 'TEST_FALSE_SIGNAL'}
  $roundTrip=(@{modules=$modules;signals=$signals}|ConvertTo-Json -Depth 6)|ConvertFrom-Json
  if($roundTrip.modules.Count -ne $modules.Count){throw 'TEST_JSON'}
  $known='f648064a90fa818912ce037af9d5c3fd6b94973a415a64862ff52b3cd23e3761'
  $missing=New-ExeProfile 'Client-Win64-Shipping.exe' $null $known
  if(!$missing.matches_uploaded_exe -or $missing.identity_status -ne 'MATCHES_UPLOADED_EXE' -or $missing.product_caption_matches){throw 'TEST_MISSING_PRODUCT_REJECTED'}
  $different=New-ExeProfile 'Client-Win64-Shipping.exe' @{ProductName='Other';ProductVersion='1'} ('0'*64)
  if($different.matches_uploaded_exe -or $different.identity_status -ne 'DIFFERENT_FROM_UPLOADED_EXE'){throw 'TEST_DIFFERENT_HASH'}
  $captionOnly=New-ExeProfile 'Client-Win64-Shipping.exe' @{ProductName='Wuthering Waves'} ('0'*64)
  if($captionOnly.matches_uploaded_exe -or !$captionOnly.product_caption_matches){throw 'TEST_CAPTION_NOT_IDENTITY'}
  $selfVersion=[Diagnostics.FileVersionInfo]::GetVersionInfo($target.MainModule.FileName)
  $selfHash=(Get-FileHash -LiteralPath $target.MainModule.FileName -Algorithm SHA256).Hash.ToLowerInvariant()
  $selfProfile=New-ExeProfile $target.MainModule.ModuleName $selfVersion $selfHash
  if($selfProfile.sha256.Length -ne 64){throw 'TEST_DISK_IDENTITY'}
  Write-Output 'PASS Windows module enumeration, EXE disk metadata/hash, missing or different product caption, hash mismatch handling, runtime signals and JSON.'
 }finally{$target.Dispose()}
}
if($SelfTest){Test-Collector;exit 0}
try {
 if(![Environment]::Is64BitProcess){throw 'Use Inspect-Runtime.cmd to start 64-bit PowerShell.'}
 Write-Output 'READ ONLY: checks the running game EXE identity and loaded module names/versions. No game memory contents, screenshots, input or network requests.'
 $targets=@(Get-Process -Name 'Client-Win64-Shipping' -ErrorAction SilentlyContinue)
 if($targets.Count -eq 0){throw 'GAME_NOT_RUNNING: open Wuthering Waves and enter the game, then run this helper again.'}
 if($targets.Count -ne 1){throw 'MULTIPLE_GAME_PROCESSES: leave only the intended game instance open.'}
 $target=$targets[0]
 try {
  $main=$target.MainModule
  Write-Output 'Candidate game process found. Reading module metadata and hashing the EXE file on disk...'
  $modules=Get-ModuleInventory $target
  $hash=(Get-FileHash -LiteralPath $main.FileName -Algorithm SHA256).Hash.ToLowerInvariant()
  $version=$null
  try{$version=[Diagnostics.FileVersionInfo]::GetVersionInfo($main.FileName)}catch{}
  $identity=New-ExeProfile $main.ModuleName $version $hash
  Write-Output ('EXE identity: '+$identity.identity_status+'. Product caption is informational only.')
  $report=[ordered]@{
   schema='SORA-WW-RUNTIME-PROFILE-1'
   collector_version=2
   exe=$identity
   module_count=$modules.Count;modules=$modules;runtime_signals=(Get-RuntimeSignals $modules)
   limitations='One snapshot of OS module metadata. Does not validate gameplay functions, skill cooldowns, damage behavior or backend support.'
  }
 }finally{$target.Dispose()}
 $desktop=[Environment]::GetFolderPath('Desktop');if(!$desktop){throw 'Desktop folder unavailable.'}
 $name='Sora-WW-Runtime-'+[Guid]::NewGuid().ToString('N').Substring(0,8)+'.json'
 $output=Join-Path $desktop $name
 $bytes=([Text.UTF8Encoding]::new($false)).GetBytes(($report|ConvertTo-Json -Depth 6))
 $stream=[IO.File]::Open($output,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
 try{$stream.Write($bytes,0,$bytes.Length)}finally{$stream.Dispose()}
 Write-Output ('DONE: '+$name+' on Desktop. Send this JSON in the conversation.')
}catch {
 Write-Output ('STOPPED: '+$_.Exception.GetBaseException().Message)
 Write-Output 'No gameplay change was attempted. If access is denied, send this error text; this helper will not bypass the restriction.'
 exit 1
}
