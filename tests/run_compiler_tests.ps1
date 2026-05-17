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

foreach ($file in Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot "compiler\valid") -Filter "*.cvm" | Sort-Object Name) {
    & $exe --bytecode --no-run $file.FullName > $null 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[PASS] compiler/valid/$($file.Name)"
    } else {
        Write-Host "[FAIL] compiler/valid/$($file.Name)"
        $failures++
    }
}

foreach ($file in Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot "compiler\invalid") -Filter "*.cvm" | Sort-Object Name) {
    & $exe --bytecode --no-run $file.FullName > $null 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[PASS] compiler/invalid/$($file.Name)"
    } else {
        Write-Host "[FAIL] compiler/invalid/$($file.Name) unexpectedly compiled"
        $failures++
    }
}

$shadowing = Join-Path $PSScriptRoot "compiler\valid\shadowing.cvm"
$shadowOutput = & $exe --bytecode --no-run $shadowing 2>&1 | Out-String
$defineCount = ([regex]::Matches($shadowOutput, "DEFINE_VAR")).Count
$slotOneUsed = $shadowOutput.Contains("var[1]")

if ($defineCount -eq 2 -and $slotOneUsed) {
    Write-Host "[PASS] compiler/semantic/shadowing_slots"
} else {
    Write-Host "[FAIL] compiler/semantic/shadowing_slots"
    $failures++
}

if ($failures -gt 0) {
    Write-Host "Compiler tests failed: $failures"
    exit 1
}

Write-Host "All compiler tests passed."
