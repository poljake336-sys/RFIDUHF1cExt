# Собирает dist/RFIDUHF.zip из результатов Gradle + MANIFEST
$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
if (-not (Test-Path (Join-Path $root "android"))) {
    $root = $PSScriptRoot
    if (-not (Test-Path (Join-Path $root "android"))) {
        $root = Split-Path $PSScriptRoot -Parent
    }
}
# Script lives in RFIDUHF1cExt/tools
$proj = Split-Path $PSScriptRoot -Parent
$android = Join-Path $proj "android"
$package = Join-Path $proj "package"
$dist = Join-Path $proj "dist"
$stage = Join-Path $dist "stage"

Write-Host "Project: $proj"

# Find APK
$apkCandidates = @(
    (Join-Path $android "app\build\outputs\apk\release\org_rfiduhf_addin-release.apk"),
    (Join-Path $android "app\build\outputs\apk\release\app-release.apk"),
    (Join-Path $android "app\build\outputs\apk\release\org_rfiduhf_addin-release-unsigned.apk"),
    (Join-Path $android "app\build\outputs\apk\debug\org_rfiduhf_addin-debug.apk"),
    (Join-Path $android "app\build\outputs\apk\debug\app-debug.apk")
)
$apk = $apkCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $apk) {
    $apk = Get-ChildItem (Join-Path $android "app\build\outputs\apk") -Recurse -Filter "*.apk" -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1 -ExpandProperty FullName
}
if (-not $apk) { throw "APK not found. Run gradlew assembleRelease first." }
Write-Host "APK: $apk"

# Find .so from cmake/ninja intermediates or merged native libs
$soMap = @{
    "armeabi-v7a" = "rfiduhf_ARM.so"
    "arm64-v8a"   = "rfiduhf_ARM64.so"
    "x86"         = "rfiduhf_x86.so"
}

function Find-So([string]$abi) {
    $paths = @(
        (Join-Path $android "app\build\intermediates\cxx\release\*\obj\$abi\liborg_rfiduhf_addin.so"),
        (Join-Path $android "app\build\intermediates\cxx\debug\*\obj\$abi\liborg_rfiduhf_addin.so"),
        (Join-Path $android "app\build\intermediates\merged_native_libs\release\out\lib\$abi\liborg_rfiduhf_addin.so"),
        (Join-Path $android "app\build\intermediates\merged_native_libs\debug\out\lib\$abi\liborg_rfiduhf_addin.so"),
        (Join-Path $android "app\build\intermediates\stripped_native_libs\release\out\lib\$abi\liborg_rfiduhf_addin.so")
    )
    foreach ($p in $paths) {
        $hit = Get-Item $p -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($hit) { return $hit.FullName }
    }
    $hit = Get-ChildItem (Join-Path $android "app\build") -Recurse -Filter "liborg_rfiduhf_addin.so" -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match [regex]::Escape($abi) } |
        Select-Object -First 1
    if ($hit) { return $hit.FullName }
    return $null
}

Remove-Item $stage -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $stage, $dist | Out-Null

Copy-Item (Join-Path $package "MANIFEST.XML") $stage
Copy-Item (Join-Path $package "ANDROID_MANIFEST_EXTENTIONS.XML") $stage
Copy-Item $apk (Join-Path $stage "org_rfiduhf_addin.apk")

foreach ($abi in $soMap.Keys) {
    $src = Find-So $abi
    if (-not $src) {
        Write-Warning "Missing .so for $abi"
        continue
    }
    $dstName = $soMap[$abi]
    Copy-Item $src (Join-Path $stage $dstName) -Force
    Write-Host "SO $abi -> $dstName"
}

$zip = Join-Path $dist "RFIDUHF.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip -Force
Write-Host "DONE: $zip"
Get-ChildItem $stage | Format-Table Name, Length
Get-Item $zip | Format-List FullName, Length
