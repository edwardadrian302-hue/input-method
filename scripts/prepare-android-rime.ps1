$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$cmake = Join-Path $root '.tools/android-sdk/cmake/3.22.1/bin/cmake.exe'
$ndk = Join-Path $root '.tools/android-sdk/ndk/28.2.13676358'
$ninja = Join-Path $root '.tools/android-sdk/cmake/3.22.1/bin/ninja.exe'
foreach ($path in @($cmake,$ninja,(Join-Path $ndk 'build/cmake/android.toolchain.cmake'),(Join-Path $root 'vendor/android-deps/boost_1_89_0/boost/version.hpp'))) {
    if (!(Test-Path -LiteralPath $path)) { throw "Android dependency missing: $path" }
}
$revision = & git -C (Join-Path $root 'vendor/librime') rev-parse HEAD
if ($LASTEXITCODE -ne 0 -or $revision -ne '0e8ee4047f938adc7f1e32e0dfa1f6ab11dac9db') { throw 'Unexpected Rime source revision.' }
$env:TEMP = Join-Path $root 'build/android-tmp'
$env:TMP = $env:TEMP
New-Item -ItemType Directory -Force -Path $env:TEMP | Out-Null
$build = Join-Path $root 'build/android-rime/arm64-v8a'
$prefix = Join-Path $root '.tools/android-rime/arm64-v8a'
& $cmake -S (Join-Path $root 'android/native') -B $build -G Ninja "-DCMAKE_MAKE_PROGRAM=$ninja" "-DCMAKE_TOOLCHAIN_FILE=$ndk/build/cmake/android.toolchain.cmake" '-DANDROID_ABI=arm64-v8a' '-DANDROID_PLATFORM=android-30' '-DANDROID_STL=c++_shared' '-DCMAKE_BUILD_TYPE=Release' "-DCMAKE_INSTALL_PREFIX=$prefix"
if ($LASTEXITCODE -ne 0) { throw 'Android Rime configuration failed.' }
& $cmake --build $build --target rime --parallel 6
if ($LASTEXITCODE -ne 0) { throw 'Android Rime compilation failed.' }
& $cmake --install $build
if ($LASTEXITCODE -ne 0) { throw 'Android Rime staging failed.' }
Write-Host 'Android ARM64 Rime built; no app installed.'
