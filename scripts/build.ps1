param([switch]$SkipTests)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
$compiler = Join-Path $root '.tools\llvm-mingw-20261020-ucrt-x86_64\bin\x86_64-w64-mingw32-clang++.exe'
if (!(Test-Path $compiler)) { throw 'Portable compiler missing. See docs/BUILD.md.' }
New-Item -ItemType Directory -Force dist,build | Out-Null
New-Item -ItemType Directory -Force build/tmp | Out-Null
$env:TEMP = Join-Path $root 'build\tmp'
$env:TMP = $env:TEMP
$common = @('-std=c++17','-O2','-Wall','-Wextra','-Wno-unused-parameter','-Wno-missing-field-initializers','-D_WIN32_WINNT=0x0A00','-DUNICODE','-D_UNICODE','-Isrc','-I.tools/rime/dist/include','-static','-municode')
$libs = @('-lole32','-loleaut32','-luuid','-ladvapi32','-luser32','-lgdi32','-lshell32')
function Compile($sources,$output,$extra=@()) {
  & $compiler @common @sources @extra @libs '-o' $output
  if ($LASTEXITCODE -ne 0) { throw "Build failed: $output" }
}
Compile @('src/engine.cpp','src/broker.cpp') 'dist/JianPinBroker.exe'
Compile @('src/settings.cpp') 'dist/JianPinSettings.exe' @('-mwindows')
Compile @('src/tsf.cpp','src/tsf.def') 'dist/JianPin.dll' @('-shared','-Wl,--kill-at')
Compile @('src/engine.cpp','tests/engine_test.cpp') 'dist/EngineTest.exe'
Compile @('tests/tsf_test.cpp') 'dist/TsfTest.exe'
Copy-Item '.tools/rime/dist/lib/rime.dll' 'dist/rime.dll' -Force
Copy-Item data dist -Recurse -Force
Copy-Item licenses dist -Recurse -Force
Copy-Item scripts/install.ps1,scripts/uninstall.ps1 dist -Force
Copy-Item README.md,THIRD_PARTY.md,sources.lock.json,downloads.lock.json dist -Force
Copy-Item docs dist -Recurse -Force
if (!$SkipTests) {
  & 'dist/EngineTest.exe'
  if ($LASTEXITCODE -ne 0) { throw 'Engine integration tests failed.' }
  & 'dist/TsfTest.exe'
  if ($LASTEXITCODE -ne 0) { throw 'TSF integration tests failed.' }
}
Write-Host 'Build completed: dist/'
