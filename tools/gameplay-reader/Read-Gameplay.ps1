# SPDX-License-Identifier: MIT
param([switch]$SelfTest)
$ErrorActionPreference='Stop'
Add-Type -Path (Join-Path $PSScriptRoot 'GameplayReader.cs')
Add-Type -AssemblyName System.IO.Compression.FileSystem
function Get-Digest([byte[]]$Bytes) {
 $sha=[Security.Cryptography.SHA256]::Create()
 try{return ([BitConverter]::ToString($sha.ComputeHash($Bytes))).Replace('-','').ToLowerInvariant()}finally{$sha.Dispose()}
}
function Assert-Source([byte[]]$Bytes,[string]$Name) {
 if($Name -ine 'pakchunk18-WindowsNoEditor.pak' -or $Bytes.Length -ne 105427118){throw 'WRONG_PAK_OR_BUILD: select pakchunk18-WindowsNoEditor.pak from the Paks folder previously inspected.'}
 $tail=[byte[]]::new(4096);[Array]::Copy($Bytes,$Bytes.Length-4096,$tail,0,4096)
 if((Get-Digest $tail) -ne 'da18d8ab27de8bbb8daa873f1a4854e2ffa81784d393905e207abc104cdf753c'){throw 'BUILD_FOOTER_MISMATCH: this package differs from the report already supplied.'}
}
function New-TestEntry([byte[]]$Image,[byte]$Flags=16) {
 $payload=[byte[]]::new($Image.Length+11);[Array]::Copy($Image,0,$payload,11,$Image.Length)
 $sha=[Security.Cryptography.SHA1]::Create();try{$digest=$sha.ComputeHash($Image)}finally{$sha.Dispose()}
 $stream=[IO.MemoryStream]::new();$writer=[IO.BinaryWriter]::new($stream)
 try{$writer.Write([long]0);$writer.Write([long]$Image.Length);$writer.Write([long]$Image.Length);$writer.Write([uint32]0);$writer.Write($digest);$writer.Write($Flags);$writer.Write([uint32]0);$writer.Write($payload);return ,$stream.ToArray()}finally{$writer.Dispose();$stream.Dispose()}
}
function Test-Reader {
 $root=Join-Path ([IO.Path]::GetTempPath()) ('sora-game-test-'+[Guid]::NewGuid().ToString('N'));[void][IO.Directory]::CreateDirectory($root)
 try {
  $game=Join-Path $root 'Aki.Game.dll';$decoy=Join-Path $root 'Decoy.dll'
  Add-Type -TypeDefinition 'using System.Reflection; [assembly: AssemblyVersion("1.2.3.4")] public class SyntheticGame { public static string Label = "Aki.Game"; }' -OutputAssembly $game -OutputType Library
  Add-Type -TypeDefinition 'public class SyntheticDecoy { public static string Label = "Aki.Game"; }' -OutputAssembly $decoy -OutputType Library
  $image=[IO.File]::ReadAllBytes($game);$entry=New-TestEntry $image;$other=New-TestEntry ([IO.File]::ReadAllBytes($decoy))
  $pak=[byte[]]($other+$entry+[byte[]]::new(300));$dataEnd=$other.Length+$entry.Length
  $before=Get-Digest $pak;$result=[GameplayReader]::Find($pak,$dataEnd,$root)
  if($result.Name -ne 'Aki.Game' -or $result.Version -ne '1.2.3.4' -or (Get-Digest $result.Bytes) -ne (Get-Digest $image) -or $result.EntryOffset -ne $other.Length){throw 'TEST_IDENTITY_OR_PAYLOAD'}
  if((Get-Digest $pak) -ne $before){throw 'TEST_SOURCE_CHANGED'}
  $encrypted=New-TestEntry $image 17
  $corrupt=[byte[]]$entry.Clone();$corrupt[28]=$corrupt[28] -bxor 1
  foreach($bad in @(@{Data=$encrypted;End=$encrypted.Length},@{Data=$corrupt;End=$corrupt.Length},@{Data=$entry;End=$entry.Length-1},@{Data=$other;End=$other.Length})) {
   $rejected=$false;try{[void][GameplayReader]::Find($bad.Data,$bad.End,$root)}catch{$rejected=$true};if(!$rejected){throw 'TEST_BAD_INPUT_ACCEPTED'}
  }
  $wrong=$false;try{Assert-Source $entry 'wrong.pak'}catch{$wrong=$true};if(!$wrong){throw 'TEST_BUILD_ACCEPTED'}
  $export=Join-Path $root 'export';[void][IO.Directory]::CreateDirectory($export);[IO.File]::WriteAllBytes((Join-Path $export 'Aki.Game.dll'),$result.Bytes)
  $zip=Join-Path $root 'result.zip';[IO.Compression.ZipFile]::CreateFromDirectory($export,$zip)
  $archive=[IO.Compression.ZipFile]::OpenRead($zip);try{if($archive.Entries.Count -ne 1 -or $archive.Entries[0].FullName -ne 'Aki.Game.dll'){throw 'TEST_ZIP'}}finally{$archive.Dispose()}
  Write-Output 'PASS real managed PE identity/payload, decoy rejection, encrypted/corrupt/truncated entry rejection, source unchanged, build guard and ZIP output.'
 }finally{if(Test-Path -LiteralPath $root){Remove-Item -LiteralPath $root -Recurse -Force}}
}
if($SelfTest){Test-Reader;exit 0}
$temp=$null;$dialog=$null
try {
 Add-Type -AssemblyName System.Windows.Forms
 Write-Output 'READ ONLY: scans the 100 MB pakchunk18 previously inspected for a plaintext Aki.Game assembly. No decryption, game memory access, code execution or modification of game files.'
 $dialog=[Windows.Forms.OpenFileDialog]::new();$dialog.Filter='pakchunk18-WindowsNoEditor.pak|pakchunk18-WindowsNoEditor.pak';$dialog.Title='Select Client\Content\Paks\pakchunk18-WindowsNoEditor.pak';$dialog.CheckFileExists=$true
 if($dialog.ShowDialog() -ne [Windows.Forms.DialogResult]::OK){Write-Output 'Cancelled.';exit 0}
 $path=$dialog.FileName;$info=Get-Item -LiteralPath $path
 if($info.Length -ne 105427118){throw 'WRONG_SIZE: this is not the pakchunk18 already inspected.'}
 $stream=[IO.File]::Open($path,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::Read)
 try{if($stream.Length -ne 105427118){throw 'SOURCE_SIZE_CHANGED'};$data=[byte[]]::new(105427118);$done=0;while($done -lt $data.Length){$n=$stream.Read($data,$done,$data.Length-$done);if($n -le 0){throw 'SHORT_READ'};$done+=$n}}finally{$stream.Dispose()}
 Assert-Source $data $info.Name
 Write-Output 'Build footer verified. Checking plaintext entries and managed assembly identity...'
 $temp=Join-Path ([IO.Path]::GetTempPath()) ('sora-game-read-'+[Guid]::NewGuid().ToString('N'));[void][IO.Directory]::CreateDirectory($temp)
 $result=[GameplayReader]::Find($data,105426321,$temp)
 $export=Join-Path $temp 'export';[void][IO.Directory]::CreateDirectory($export);[IO.File]::WriteAllBytes((Join-Path $export 'Aki.Game.dll'),$result.Bytes)
 $report=[ordered]@{schema='SORA-WW-GAMEPLAY-ASSEMBLY-2';source_pak=$info.Name;source_bytes=$data.Length;source_sha256=(Get-Digest $data);entry_offset=$result.EntryOffset;entry_sha1_verified=$true;assembly_name=$result.Name;assembly_version=$result.Version;assembly_sha256=(Get-Digest $result.Bytes);assembly_bytes=$result.Bytes.Length;alignment_padding_bytes=11;sha1_region='PE bytes, excluding alignment padding';source_modified=$false;assembly_executed=$false;scope='Plaintext uncompressed entry only. Index encryption was not decrypted.'}
 $json=$report|ConvertTo-Json -Depth 4;[IO.File]::WriteAllText((Join-Path $export 'assembly-info.json'),$json,[Text.UTF8Encoding]::new($false))
 $desktop=[Environment]::GetFolderPath('Desktop');if(!$desktop){throw 'Desktop folder unavailable.'}
 $name='Sora-WW-AkiGame-'+[Guid]::NewGuid().ToString('N').Substring(0,8)+'.zip';$zip=Join-Path $desktop $name
 [IO.Compression.ZipFile]::CreateFromDirectory($export,$zip)
 Write-Output ('DONE: '+$name+' on Desktop. Send this ZIP privately in the conversation; do not commit it to GitHub.')
 Write-Output ('ZIP size: '+(Get-Item -LiteralPath $zip).Length+' bytes. Game files were not changed.')
}catch{Write-Output ('STOPPED: '+$_.Exception.GetBaseException().Message);exit 1}
finally{if($dialog){$dialog.Dispose()};if($temp -and (Test-Path -LiteralPath $temp)){Remove-Item -LiteralPath $temp -Recurse -Force}}
