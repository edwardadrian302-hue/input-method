$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if ([IO.Path]::GetPathRoot($root) -ne 'D:\') { throw '打包工具必须放在 D 盘项目目录。' }
Set-Location $root
$lock = Get-Content installer.lock.json -Raw -Encoding UTF8 | ConvertFrom-Json
New-Item -ItemType Directory -Force .downloads,.tools | Out-Null
$file = Join-Path $root ('.downloads\innosetup-' + $lock.version + '.exe')
if (!(Test-Path -LiteralPath $file)) { Invoke-WebRequest -UseBasicParsing -Uri $lock.url -OutFile $file }
if ((Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLower() -ne $lock.sha256) { throw 'Inno Setup 下载校验失败。' }
$signature = Get-AuthenticodeSignature -LiteralPath $file
if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -ne $lock.publisher) { throw 'Inno Setup 官方数字签名校验失败。' }
$language = Join-Path $PSScriptRoot 'ChineseSimplified.isl'
if ((Get-FileHash -LiteralPath $language -Algorithm SHA256).Hash.ToLower() -ne $lock.language_sha256) { throw '安装向导语言文件校验失败。' }
$process = Start-Process -FilePath $file -ArgumentList @('/PORTABLE=1','/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/NOICONS',('/DIR="' + (Join-Path $root '.tools\inno-setup') + '"')) -WindowStyle Hidden -Wait -PassThru
if ($process.ExitCode -ne 0) { throw ('便携打包工具准备失败：' + $process.ExitCode) }
Copy-Item -LiteralPath (Join-Path $root '.tools\inno-setup\License.txt') -Destination (Join-Path $root 'licenses\inno-setup.txt') -Force
Write-Host '打包工具已准备在 D 盘 .tools\inno-setup；运行 scripts/build-installer.ps1 生成 EXE。'
