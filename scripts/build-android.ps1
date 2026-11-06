param([switch]$SkipNativeBuild)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$gradle = Join-Path $root '.tools/gradle-8.13/bin/gradle.bat'
$env:JAVA_HOME = 'D:\Environment\jdk-21.0.11'
$env:ANDROID_HOME = Join-Path $root '.tools/android-sdk'
$env:ANDROID_USER_HOME = Join-Path $root '.tools/android-user'
$env:GRADLE_USER_HOME = Join-Path $root '.tools/gradle-user-home'
$env:TEMP = Join-Path $root 'build/android-tmp'
$env:TMP = $env:TEMP
New-Item -ItemType Directory -Force -Path $env:TEMP | Out-Null
if (!(Test-Path -LiteralPath $gradle)) { throw 'Portable Gradle 8.13 missing.' }
if (!(Test-Path -LiteralPath (Join-Path $env:JAVA_HOME 'bin/java.exe'))) { throw 'D-drive Java 21 missing.' }
$sdkPath = $env:ANDROID_HOME.Replace('\','/').Replace(':','\:')
Set-Content -LiteralPath (Join-Path $root 'android/local.properties') -Value "sdk.dir=$sdkPath" -Encoding ASCII
& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'prepare-android-signing.ps1')
if ($LASTEXITCODE -ne 0) { throw 'Android signing preparation failed.' }
if (!$SkipNativeBuild) {
    & powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'prepare-android-rime.ps1')
    if ($LASTEXITCODE -ne 0) { throw 'Android native build failed.' }
}
& $gradle -p (Join-Path $root 'android') --no-daemon :app:assembleDebug :app:lintDebug
if ($LASTEXITCODE -ne 0) { throw 'Android APK build or lint failed.' }
$apk = Join-Path $root 'build/ChaoxiPinyin-0.4.1-preview-android-arm64.apk'
Copy-Item -LiteralPath (Join-Path $root 'android/app/build/outputs/apk/debug/app-debug.apk') -Destination $apk -Force
Get-FileHash -LiteralPath $apk -Algorithm SHA256
Write-Host 'APK created for manual installation. Back up .tools/android-signing privately for upgrades.'
