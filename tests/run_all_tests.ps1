$ErrorActionPreference = "Continue"

$testsRoot = $PSScriptRoot

powershell -ExecutionPolicy Bypass -File (Join-Path $testsRoot "run_parser_tests.ps1")
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

powershell -ExecutionPolicy Bypass -File (Join-Path $testsRoot "run_compiler_tests.ps1")
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

powershell -ExecutionPolicy Bypass -File (Join-Path $testsRoot "run_vm_tests.ps1")
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Host "All CVM tests passed."
