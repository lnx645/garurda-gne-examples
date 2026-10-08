# Bangun semua contoh ekstensi GNE untuk Windows.
# Butuh gcc (mingw-w64), mis. dari MSYS2: pacman -S mingw-w64-x86_64-gcc
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

if ($env:GNE_HEADER) {
    $inc = $env:GNE_HEADER
} elseif (Test-Path "C:\garurda\include\gne.h") {
    $inc = "C:\garurda\include"
} else {
    New-Item -ItemType Directory -Force ".cache" | Out-Null
    Write-Host "gne.h tidak ditemukan - mengunduh dari GitHub..."
    Invoke-WebRequest -Uri "https://raw.githubusercontent.com/lnx645/galang/master/include/gne.h" -OutFile ".cache\gne.h"
    $inc = ".cache"
}

foreach ($ex in @("hello", "kv")) {
    New-Item -ItemType Directory -Force "$ex\gne" | Out-Null
    gcc -shared -Wall -Wextra -I $inc -o "$ex\gne\$ex.dll" "$ex\$ex.c"
    Write-Host "OK: $ex\gne\$ex.dll"
}
Write-Host "Selesai. Coba: cd hello; gar run main.ga"
