param([switch]$SkipTests)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
$compiler = Join-Path $root '.tools\llvm-mingw-20261020-ucrt-x86_64\bin\x86_64-w64-mingw32-clang++.exe'
if (!(Test-Path $compiler)) { throw 'Portable compiler missing. See docs/BUILD.md.' }
# dist is a generated staging directory; recreate it to prevent stale binaries/data.
$stage = [IO.Path]::GetFullPath((Join-Path $root 'dist'))
if ($stage -cne ($root.TrimEnd('\') + '\dist')) { throw 'Unexpected staging path.' }
if (Test-Path -LiteralPath $stage) {
  $items = @((Get-Item -LiteralPath $stage -Force)) + @(Get-ChildItem -LiteralPath $stage -Force -Recurse)
  foreach ($item in $items) { if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Refusing linked staging directory.' } }
  Remove-Item -LiteralPath $stage -Recurse -Force
}
New-Item -ItemType Directory -Force dist,build,build/tests | Out-Null
New-Item -ItemType Directory -Force build/tmp | Out-Null
$env:TEMP = Join-Path $root 'build\tmp'
$env:TMP = $env:TEMP
$common = @('-std=c++17','-O2','-Wall','-Wextra','-Wno-unused-parameter','-Wno-missing-field-initializers','-D_WIN32_WINNT=0x0A00','-DUNICODE','-D_UNICODE','-Isrc','-I.tools/rime/dist/include','-static','-municode')
$libs = @('-lole32','-loleaut32','-luuid','-ladvapi32','-luser32','-lgdi32','-lshell32')
function Compile($sources,$output,$extra=@()) {
  & $compiler @common @sources @extra @libs '-o' $output
  if ($LASTEXITCODE -ne 0) { throw "Build failed: $output" }
}
$resourceCompiler = Join-Path (Split-Path $compiler -Parent) 'x86_64-w64-mingw32-windres.exe'
& $resourceCompiler src/app.rc -O coff -o build/app-icon.o
if ($LASTEXITCODE -ne 0) { throw 'Icon resource compilation failed.' }
Compile @('src/engine.cpp','src/broker.cpp','build/app-icon.o') 'dist/ChaoxiPinyinBroker.exe'
Compile @('src/settings.cpp','build/app-icon.o') 'dist/ChaoxiPinyinSettings.exe' @('-mwindows','-lcomdlg32','-lwinhttp')
Compile @('src/tsf.cpp','src/tsf.def','build/app-icon.o') 'dist/ChaoxiPinyin.dll' @('-shared','-Wl,--kill-at')
Compile @('src/engine.cpp','tests/engine_test.cpp') 'build/tests/EngineTest.exe'
Compile @('tests/tsf_test.cpp') 'build/tests/TsfTest.exe'
Compile @('tests/dictionary_test.cpp') 'build/tests/DictionaryTest.exe' @('-lwinhttp')
Compile @('tests/candidate_preview.cpp') 'build/CandidatePreview.exe'
Compile @('tests/settings_preview.cpp','build/app-icon.o') 'build/tests/SettingsPreview.exe' @('-lcomdlg32','-lwinhttp')
Copy-Item '.tools/rime/dist/lib/rime.dll' 'dist/rime.dll' -Force
Copy-Item data dist -Recurse -Force
Copy-Item licenses dist -Recurse -Force
Copy-Item scripts/install.ps1,scripts/uninstall.ps1 dist -Force
Copy-Item README.md,THIRD_PARTY.md dist -Force
if (!$SkipTests) {
  & 'build/tests/DictionaryTest.exe'
  if ($LASTEXITCODE -ne 0) { throw 'Dictionary tests failed.' }
  & 'build/tests/EngineTest.exe'
  if ($LASTEXITCODE -ne 0) { throw 'Engine integration tests failed.' }
  & 'build/tests/TsfTest.exe'
  if ($LASTEXITCODE -ne 0) { throw 'TSF integration tests failed.' }
  & 'build/tests/SettingsPreview.exe'
  if ($LASTEXITCODE -ne 0) { throw 'Settings rendering failed.' }
}
Write-Host 'Build completed: dist/'
