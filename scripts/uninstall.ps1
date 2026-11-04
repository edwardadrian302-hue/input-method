$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath($PSScriptRoot)
if (!(Test-Path (Join-Path $root 'chaoxipinyin-install.marker'))) { throw '缺少潮汐拼音安装标记，拒绝卸载。' }
$principal = [Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
if (!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) { throw '请以管理员身份运行卸载脚本。' }
$dll = Join-Path $root 'ChaoxiPinyin.dll'
$proc = Start-Process -FilePath "$env:SystemRoot\System32\regsvr32.exe" -ArgumentList @('/s','/u',('"' + $dll + '"')) -WindowStyle Hidden -Wait -PassThru
if ($proc.ExitCode -ne 0) { throw "注销失败：$($proc.ExitCode)" }
Write-Host '输入法注册已移除。请注销并重新登录，然后可删除该安装文件夹。'
Write-Host '本地学习数据保留在 %LOCALAPPDATA%\ChaoxiPinyin；如需清除，请在卸载前使用设置窗口。'
# Loaded DLLs are deliberately not force-deleted; do not kill browsers or alter other input methods.
