# SPDX-License-Identifier: MIT
# Reads only the verified index region and footer; no decrypt/extract/game changes.
param([switch]$SelfTest)
$ErrorActionPreference='Stop'
function Get-Digest([byte[]]$Bytes,[string]$Algorithm='SHA256',[int]$Offset=0,[int]$Count=-1){
 if($Count -lt 0){$Count=$Bytes.Length-$Offset}
 $hash=[Security.Cryptography.HashAlgorithm]::Create($Algorithm)
 try{return ([BitConverter]::ToString($hash.ComputeHash($Bytes,$Offset,$Count))).Replace('-','').ToLowerInvariant()}finally{$hash.Dispose()}
}
function Read-ExactRange([IO.FileStream]$Stream,[long]$Offset,[int]$Count){
 if($Offset -lt 0 -or $Count -le 0 -or $Offset+$Count -gt $Stream.Length -or $Count -gt 20*1024*1024){throw 'INVALID_READ_BOUND'}
 [void]$Stream.Seek($Offset,[IO.SeekOrigin]::Begin);$data=[byte[]]::new($Count);$done=0
 while($done -lt $Count){$read=$Stream.Read($data,$done,$Count-$done);if($read -le 0){throw 'SHORT_READ'};$done+=$read}
 return ,$data
}
function Read-IndexRegion([string]$Path,[hashtable]$Expected){
 if([IO.Path]::GetFileName($Path) -ine $Expected.Name){throw 'Select pakchunk43-WindowsNoEditor.pak from the same game installation.'}
 $stream=[IO.File]::Open($Path,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite)
 try{
  if($stream.Length -ne $Expected.Size){throw 'PAK_SIZE_CHANGED: send a fresh header report for the updated build.'}
  $tail=Read-ExactRange $stream ($stream.Length-4096) 4096
  if((Get-Digest $tail) -ne $Expected.FooterSHA256){throw 'PAK_FOOTER_CHANGED: send a fresh header report.'}
  $region=Read-ExactRange $stream $Expected.Offset $Expected.RegionBytes
  if((Get-Digest $region 'SHA1' 0 $Expected.IndexBytes) -ne $Expected.IndexSHA1){throw 'PRIMARY_INDEX_HASH_MISMATCH: no report was written.'}
  $tailAfter=Read-ExactRange $stream ($stream.Length-4096) 4096
  if($stream.Length -ne $Expected.Size -or (Get-Digest $tailAfter) -ne $Expected.FooterSHA256){throw 'PAK_CHANGED_DURING_READ'}
  return [ordered]@{schema='SORA-WW-PAK43-INDEX-1';name=$Expected.Name;file_bytes=$Expected.Size;pak_version=12;encrypted_index_flag=0;index_offset=$Expected.Offset;index_bytes=$Expected.IndexBytes;index_sha1=$Expected.IndexSHA1;footer_sha256=$Expected.FooterSHA256;region_offset=$Expected.Offset;region_bytes=$region.Length;region_sha256=(Get-Digest $region);region_base64=[Convert]::ToBase64String($region);source_modified=$false;primary_index_hash_verified=$true;full_pak_included=$false}
 }finally{$stream.Dispose()}
}
function Test-IndexReader {
 $root=Join-Path ([IO.Path]::GetTempPath()) ('wuwa-index-test-'+[Guid]::NewGuid().ToString('N'));[void][IO.Directory]::CreateDirectory($root)
 try{
  $path=Join-Path $root 'pakchunk43-WindowsNoEditor.pak';$data=[byte[]]::new(20000);for($i=0;$i -lt $data.Length;$i++){$data[$i]=[byte]($i%251)};[IO.File]::WriteAllBytes($path,$data)
  $before=Get-Digest $data;$expected=@{Name='pakchunk43-WindowsNoEditor.pak';Size=[long]20000;Offset=[long]512;RegionBytes=19267;IndexBytes=1024;FooterSHA256=(Get-Digest $data 'SHA256' 15904 4096);IndexSHA1=(Get-Digest $data 'SHA1' 512 1024)}
  $report=Read-IndexRegion $path $expected;$raw=[Convert]::FromBase64String($report.region_base64)
  if($raw.Length -ne 19267 -or (Get-Digest $raw 'SHA1' 0 1024) -ne $expected.IndexSHA1){throw 'TEST_VALID_RANGE'}
  if(($report | ConvertTo-Json -Depth 4).Contains($root)){throw 'TEST_PRIVATE_PATH_LEAK'}
  foreach($key in @('Size','FooterSHA256','IndexSHA1')){
   $bad=$expected.Clone();if($key -eq 'Size'){$bad[$key]=20001}else{$bad[$key]='invalid'}
   $failed=$false;try{$null=Read-IndexRegion $path $bad}catch{$failed=$true};if(!$failed){throw ('TEST_DID_NOT_REJECT_'+$key)}
  }
  $stream=[IO.File]::OpenRead($path);try{$failed=$false;try{$null=Read-ExactRange $stream 0 (21*1024*1024)}catch{$failed=$true};if(!$failed){throw 'TEST_SIZE_BOUND'}}finally{$stream.Dispose()}
  if((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $before){throw 'TEST_SOURCE_CHANGED'}
  if($report.full_pak_included -or $report.source_modified -or !$report.primary_index_hash_verified){throw 'TEST_REPORT_FLAGS'}
  Write-Output 'PASS exact bounded region, primary SHA1, footer snapshot, changed-build rejection and source unchanged; synthetic data only.'
 }finally{if(Test-Path -LiteralPath $root){Remove-Item -LiteralPath $root -Recurse -Force}}
}
if($SelfTest){Test-IndexReader;exit 0}
$expected=@{Name='pakchunk43-WindowsNoEditor.pak';Size=[long]690064639;Offset=[long]672682329;RegionBytes=17382089;IndexBytes=4554724;FooterSHA256='8f2eb4a52b1f68b87c7b28cfb5925ecd856c1a020e6d02f4945bdd9d6d806e3f';IndexSHA1='06470b0e276620de21e908e6f8198cd0fc7def40'}
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.IO.Compression
$dialog=[Windows.Forms.OpenFileDialog]::new();$dialog.Title='Select pakchunk43-WindowsNoEditor.pak';$dialog.Filter='Expected PAK (pakchunk43-WindowsNoEditor.pak)|pakchunk43-WindowsNoEditor.pak';$dialog.CheckFileExists=$true;$dialog.Multiselect=$false
try{
 Write-Output 'READ ONLY: this reads the index region (16.6 MiB) and footer of pakchunk43, not the full PAK.'
 if($dialog.ShowDialog() -ne [Windows.Forms.DialogResult]::OK){Write-Output 'Cancelled. No report was written.';exit 0}
 $report=Read-IndexRegion $dialog.FileName $expected
 $desktop=[Environment]::GetFolderPath('Desktop');if(!$desktop){throw 'Desktop folder unavailable.'}
 $name='Sora-WW-Pak43-Index.zip';$output=Join-Path $desktop $name
 if(Test-Path -LiteralPath $output){$name='Sora-WW-Pak43-Index-'+[Guid]::NewGuid().ToString('N').Substring(0,8)+'.zip';$output=Join-Path $desktop $name}
 $target=[IO.File]::Open($output,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
 try{
  $zip=[IO.Compression.ZipArchive]::new($target,[IO.Compression.ZipArchiveMode]::Create,$true)
  try{$entry=$zip.CreateEntry('Sora-WW-Pak43-Index.json',[IO.Compression.CompressionLevel]::Optimal);$writer=[IO.StreamWriter]::new($entry.Open(),[Text.UTF8Encoding]::new($false));try{$writer.Write(($report | ConvertTo-Json -Depth 4))}finally{$writer.Dispose()}}finally{$zip.Dispose()}
 }finally{$target.Dispose()}
 Write-Output ('DONE: '+$name+' was created on your Desktop. Upload this ZIP to the conversation.')
 Write-Output 'The source PAK and SIG were not changed. No decryption, game memory access or network request was performed.'
}catch{Write-Output ('STOPPED: '+$_.Exception.GetBaseException().Message);exit 1}
finally{$dialog.Dispose()}
