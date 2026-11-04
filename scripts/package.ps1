$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
$files = @('ChaoxiPinyin.dll','ChaoxiPinyinBroker.exe','ChaoxiPinyinSettings.exe','rime.dll','data','licenses','install.ps1','uninstall.ps1','README.md','THIRD_PARTY.md')
foreach ($name in $files) { if (!(Test-Path (Join-Path 'dist' $name))) { throw "缺少构建产物：$name" } }
$hashes = Get-ChildItem dist -File | Where-Object { $_.Name -in $files } | Get-FileHash -Algorithm SHA256 | ForEach-Object { '{0}  {1}' -f $_.Hash.ToLower(),[IO.Path]::GetFileName($_.Path) }
$hashes | Set-Content dist/SHA256SUMS.txt -Encoding ascii
$paths = @($files + 'SHA256SUMS.txt' | ForEach-Object { Join-Path 'dist' $_ })
Compress-Archive -Path $paths -DestinationPath build/ChaoxiPinyin-0.6.0-preview-win-x64.zip -Force
Get-FileHash build/ChaoxiPinyin-0.6.0-preview-win-x64.zip -Algorithm SHA256
