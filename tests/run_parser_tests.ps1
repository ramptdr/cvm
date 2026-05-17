$ErrorActionPreference = "Continue"

$root = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $root "cvm.exe"

g++ -std=c++17 -Wall -Wextra -Werror -Isrc `
    src/main.cpp `
    src/lexer/lexer.cpp `
    src/parser/parser.cpp `
    src/compiler/compiler.cpp `
    src/compiler/chunk.cpp `
    src/vm/vm.cpp `
    -o $exe

if ($LASTEXITCODE -ne 0) {
    Write-Host "[FAIL] build"
    exit $LASTEXITCODE
}

$failures = 0

foreach ($file in Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot "parser\valid") -Filter "*.cvm" | Sort-Object Name) {
    & $exe --ast --no-run $file.FullName > $null 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[PASS] parser/valid/$($file.Name)"
    } else {
        Write-Host "[FAIL] parser/valid/$($file.Name)"
        $failures++
    }
}

foreach ($file in Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot "parser\invalid") -Filter "*.cvm" | Sort-Object Name) {
    & $exe --ast --no-run $file.FullName > $null 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[PASS] parser/invalid/$($file.Name)"
    } else {
        Write-Host "[FAIL] parser/invalid/$($file.Name) unexpectedly parsed"
        $failures++
    }
}

if ($failures -gt 0) {
    Write-Host "Parser tests failed: $failures"
    exit 1
}

Write-Host "All parser tests passed."
