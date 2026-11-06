$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$directory = Join-Path $root '.tools/android-signing'
New-Item -ItemType Directory -Force -Path $directory | Out-Null
$properties = Join-Path $directory 'signing.properties'
$key = Join-Path $directory 'ChaoxiPinyin.jks'
if (!(Test-Path -LiteralPath $properties)) {
    if (Test-Path -LiteralPath $key) { throw 'Signing key exists without its properties. Restore the properties backup.' }
    $bytes = New-Object byte[] 32
    $rng = [Security.Cryptography.RandomNumberGenerator]::Create()
    $rng.GetBytes($bytes); $rng.Dispose()
    $password = [Convert]::ToBase64String($bytes)
    Set-Content -LiteralPath $properties -Value @('keyAlias=chaoxi',"storePassword=$password") -Encoding ASCII
}
$values = @{}
Get-Content -LiteralPath $properties | ForEach-Object { $pair = $_ -split '=',2; if ($pair.Count -eq 2) { $values[$pair[0]] = $pair[1] } }
if (!(Test-Path -LiteralPath $key)) {
    $env:CHAOXI_ANDROID_KEY_PASSWORD = $values['storePassword']
    try {
        & 'D:\Environment\jdk-21.0.11\bin\keytool.exe' -genkeypair -keystore $key -storetype PKCS12 -alias chaoxi -keyalg RSA -keysize 3072 -validity 36500 -dname 'CN=ChaoxiPinyin' -storepass:env CHAOXI_ANDROID_KEY_PASSWORD -keypass:env CHAOXI_ANDROID_KEY_PASSWORD
        if ($LASTEXITCODE -ne 0) { throw 'Android signing key generation failed.' }
    } finally { Remove-Item Env:CHAOXI_ANDROID_KEY_PASSWORD -ErrorAction SilentlyContinue }
}
Write-Host 'Personal APK signing key ready. Back up .tools/android-signing privately for future upgrades.'
