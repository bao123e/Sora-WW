# SPDX-License-Identifier: MIT
# Read-only collection of fixed-size PAK footer samples. Never reads full PAKs.
param([switch]$SelfTest)
$ErrorActionPreference='Stop'
function Get-SampleWindow([long]$Length){
 if($Length -le 0){throw 'EMPTY_FILE'}
 $count=[int][Math]::Min([long]4096,$Length)
 return [pscustomobject]@{Offset=[long]($Length-$count);Bytes=$count}
}
function Read-PakFooter([IO.FileInfo]$File){
 $stream=$null;$sha=$null
 try{
  $stream=[IO.File]::Open($File.FullName,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite)
  $length=[long]$stream.Length;$window=Get-SampleWindow $length
  [void]$stream.Seek($window.Offset,[IO.SeekOrigin]::Begin)
  $buffer=[byte[]]::new($window.Bytes);$done=0
  while($done -lt $buffer.Length){$n=$stream.Read($buffer,$done,$buffer.Length-$done);if($n -le 0){throw 'SHORT_READ'};$done+=$n}
  if($stream.Length -ne $length){throw 'FILE_CHANGED'}
  $sha=[Security.Cryptography.SHA256]::Create();$digest=([BitConverter]::ToString($sha.ComputeHash($buffer))).Replace('-','').ToLowerInvariant()
  return [ordered]@{name=$File.Name;file_bytes=$length;tail_offset=$window.Offset;tail_bytes=$buffer.Length;tail_sha256=$digest;tail_base64=[Convert]::ToBase64String($buffer);status='OK'}
 }catch{return [ordered]@{name=$File.Name;status='READ_FAILED'}}
 finally{if($sha){$sha.Dispose()};if($stream){$stream.Dispose()}}
}
function Get-PakReport([string]$Directory){
 $folder=Get-Item -LiteralPath $Directory
 if(!$folder.PSIsContainer -or $folder.Name -notmatch '^(Paks|SD)$'){throw 'Select a .pak file inside Client\Content\Paks or Client\Content\SD.'}
 $files=@(Get-ChildItem -LiteralPath $folder.FullName -Filter '*.pak' -File | Sort-Object Name)
 if($files.Count -eq 0){throw 'No .pak files found.'}
 if($files.Count -gt 2048){throw 'Too many .pak files. Select only the game Paks or SD folder.'}
 $samples=@(foreach($file in $files){Read-PakFooter $file})
 if(@($samples | Where-Object {$_.status -eq 'OK'}).Count -eq 0){throw 'Could not read any PAK footer. No report was written.'}
 return [ordered]@{schema='SORA-WW-PAK-FOOTERS-1';sample_limit_bytes=4096;source_folder=$folder.Name;file_count=$files.Count;source_files_modified=$false;contains_full_paks=$false;files=$samples}
}
function Test-Collector {
 $root=Join-Path ([IO.Path]::GetTempPath()) ('wuwa-pak-test-'+[Guid]::NewGuid().ToString('N'));$paks=Join-Path $root 'Paks';[void][IO.Directory]::CreateDirectory($paks)
 try{
  $bytes=[byte[]]::new(10123);for($i=0;$i -lt $bytes.Length;$i++){$bytes[$i]=[byte]($i%251)}
  $first=Join-Path $paks 'pakchunk0-WindowsNoEditor.pak';[IO.File]::WriteAllBytes($first,$bytes)
  [IO.File]::WriteAllBytes((Join-Path $paks 'pakchunk1-WindowsNoEditor.pak'),[byte[]](1,2,3))
  [IO.File]::WriteAllBytes((Join-Path $paks 'pakchunk0-WindowsNoEditor.sig'),[byte[]](9,9,9))
  [void][IO.Directory]::CreateDirectory((Join-Path $paks 'Ignored'));[IO.File]::WriteAllBytes((Join-Path $paks 'Ignored\nested.pak'),[byte[]](4,5))
  $before=(Get-FileHash -LiteralPath $first -Algorithm SHA256).Hash
  $report=Get-PakReport $paks
  if($report.file_count -ne 2 -or $report.files.Count -ne 2){throw 'TEST_FILE_SELECTION'}
  $one=$report.files[0];if($one.status -ne 'OK' -or $one.tail_bytes -ne 4096 -or $one.tail_offset -ne 6027){throw 'TEST_FOOTER_BOUND'}
  $data=[Convert]::FromBase64String($one.tail_base64);for($i=0;$i -lt 4096;$i++){if($data[$i] -ne $bytes[6027+$i]){throw 'TEST_SAMPLE_CONTENT'}}
  if($report.files[1].tail_bytes -ne 3 -or $report.files[1].tail_offset -ne 0){throw 'TEST_SMALL_FILE'}
  $large=Get-SampleWindow ([long]7*1024*1024*1024);if($large.Offset -ne ([long]7*1024*1024*1024-4096) -or $large.Bytes -ne 4096){throw 'TEST_64BIT_OFFSET'}
  if((Get-FileHash -LiteralPath $first -Algorithm SHA256).Hash -ne $before){throw 'TEST_SOURCE_CHANGED'}
  $roundTrip=($report | ConvertTo-Json -Depth 6) | ConvertFrom-Json
  if($roundTrip.files.Count -ne 2 -or $roundTrip.contains_full_paks -or $roundTrip.source_files_modified){throw 'TEST_REPORT_ROUNDTRIP'}
  if(($report | ConvertTo-Json -Depth 6).Contains($root)){throw 'TEST_PRIVATE_PATH_LEAK'}
  $sd=Join-Path $root 'SD';[void][IO.Directory]::CreateDirectory($sd)
  $sdFile=Join-Path $sd 'pakchunk4-SD-WindowsNoEditor.pak';[IO.File]::WriteAllBytes($sdFile,$bytes)
  $sdBefore=(Get-FileHash -LiteralPath $sdFile -Algorithm SHA256).Hash
  [IO.File]::WriteAllBytes((Join-Path $sd 'pakchunk4-SD-WindowsNoEditor.sig'),[byte[]](9))
  $sdReport=Get-PakReport $sd
  if($sdReport.source_folder -ne 'SD' -or $sdReport.file_count -ne 1 -or $sdReport.files[0].tail_bytes -ne 4096 -or $sdReport.files[0].tail_offset -ne 6027){throw 'TEST_SD_SELECTION'}
  if($sdReport.files[0].tail_sha256 -ne $report.files[0].tail_sha256){throw 'TEST_SD_HASH'}
  if((Get-FileHash -LiteralPath $sdFile -Algorithm SHA256).Hash -ne $sdBefore){throw 'TEST_SD_SOURCE_CHANGED'}
  $rejected=$false;try{Get-PakReport $root | Out-Null}catch{$rejected=$true}
  if(!$rejected){throw 'TEST_UNSUPPORTED_FOLDER_ACCEPTED'}
  Write-Output 'PASS Paks and SD collection, footer bytes/hash, 64-bit offsets, small files, .sig/subfolder exclusion, source unchanged, unsupported folder rejection and path-free JSON.'
 }finally{if(Test-Path -LiteralPath $root){Remove-Item -LiteralPath $root -Recurse -Force}}
}
if($SelfTest){Test-Collector;exit 0}
Add-Type -AssemblyName System.Windows.Forms
$dialog=[Windows.Forms.OpenFileDialog]::new();$dialog.Title='Select ANY .pak file inside Client\Content\Paks or Client\Content\SD';$dialog.Filter='Unreal PAK files (*.pak)|*.pak';$dialog.Multiselect=$false;$dialog.CheckFileExists=$true
try{
 Write-Output 'READ ONLY: select any .pak in the game Paks or SD folder. Only the final 4 KB of each .pak will be read.'
 if($dialog.ShowDialog() -ne [Windows.Forms.DialogResult]::OK){Write-Output 'Cancelled. No files were changed.';exit 0}
 $report=Get-PakReport ([IO.Path]::GetDirectoryName($dialog.FileName))
 $desktop=[Environment]::GetFolderPath('Desktop');if(!$desktop){throw 'Desktop folder unavailable.'}
 $name='Sora-WW-Pak-Headers.json';$output=Join-Path $desktop $name
 if(Test-Path -LiteralPath $output){$name='Sora-WW-Pak-Headers-'+[Guid]::NewGuid().ToString('N').Substring(0,8)+'.json';$output=Join-Path $desktop $name}
 $json=$report | ConvertTo-Json -Depth 6
 $encoding=[Text.UTF8Encoding]::new($false);$bytes=$encoding.GetBytes($json);$target=[IO.File]::Open($output,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
 try{$target.Write($bytes,0,$bytes.Length)}finally{$target.Dispose()}
 Write-Output ('DONE: '+$name+' was created on your Desktop. Upload that JSON file to the conversation.')
 Write-Output ('Files sampled: '+$report.file_count+'. PAK files were not changed. No network request was sent.')
}catch{Write-Output ('STOPPED: '+$_.Exception.GetBaseException().Message);exit 1}
finally{$dialog.Dispose()}
