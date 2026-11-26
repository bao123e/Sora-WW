# SPDX-License-Identifier: MIT
param([switch]$SelfTest)
$ErrorActionPreference='Stop'
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
  Write-Output 'PASS real process module enumeration, filename-only records, runtime signal matching and JSON round trip.'
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
  if($main.FileVersionInfo.ProductName -notmatch 'Wuthering Waves'){throw 'UNEXPECTED_PRODUCT: EXE product identity does not match Wuthering Waves.'}
  Write-Output 'Game found. Reading module metadata and hashing the EXE file on disk...'
  $modules=Get-ModuleInventory $target
  $hash=(Get-FileHash -LiteralPath $main.FileName -Algorithm SHA256).Hash.ToLowerInvariant()
  $report=[ordered]@{
   schema='SORA-WW-RUNTIME-PROFILE-1'
   exe=[ordered]@{name=$main.ModuleName;product=$main.FileVersionInfo.ProductName;product_version=$main.FileVersionInfo.ProductVersion;sha256=$hash;matches_uploaded_exe=($hash -eq 'f648064a90fa818912ce037af9d5c3fd6b94973a415a64862ff52b3cd23e3761')}
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
