param([string]$InstallDir = 'D:\JianPin')
$ErrorActionPreference = 'Stop'
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = [Security.Principal.WindowsPrincipal]::new($identity)
if (!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) { throw '请以管理员身份运行 PowerShell，再执行此安装脚本。' }
$source = $PSScriptRoot
if (!(Test-Path (Join-Path $source 'JianPin.dll'))) { throw '请从编译后的 dist 目录运行安装脚本。' }
$target = [IO.Path]::GetFullPath($InstallDir)
if ([IO.Path]::GetPathRoot($target).TrimEnd('\') -eq $target.TrimEnd('\')) { throw '安装目录不能是磁盘根目录。' }
if (Test-Path $target) {
  $existing = @(Get-ChildItem -LiteralPath $target -Force)
  if ($existing.Count -gt 0 -and !(Test-Path (Join-Path $target 'jianpin-install.marker'))) { throw '目标目录不为空且不属于简拼，请选择一个新目录。' }
  if ((Get-Item -LiteralPath $target).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw '安装目录不能是链接。' }
}
New-Item -ItemType Directory -Force -Path $target | Out-Null
foreach ($name in @('JianPin.dll','JianPinBroker.exe','JianPinSettings.exe','rime.dll','data','licenses','docs','README.md','THIRD_PARTY.md','sources.lock.json','downloads.lock.json','uninstall.ps1')) {
  Copy-Item -LiteralPath (Join-Path $source $name) -Destination $target -Recurse -Force
}
Set-Content -LiteralPath (Join-Path $target 'jianpin-install.marker') -Value 'JianPin 0.1.0' -Encoding UTF8
$dll = Join-Path $target 'JianPin.dll'
$proc = Start-Process -FilePath "$env:SystemRoot\System32\regsvr32.exe" -ArgumentList @('/s', ('"' + $dll + '"')) -WindowStyle Hidden -Wait -PassThru
if ($proc.ExitCode -ne 0) { throw "注册失败，退出码 $($proc.ExitCode)。文件已保留供检查。" }
Write-Host ('已安装到 {0}。请注销并重新登录，用 Win+空格选择：简拼 · 离线双拼。' -f $target)
Write-Host '设置：运行安装目录中的 JianPinSettings.exe。未修改默认输入法。'
