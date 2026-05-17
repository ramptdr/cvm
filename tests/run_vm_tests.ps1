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
$valid = @(
    @{Name="arithmetic"; File="vm\valid\arithmetic.cvm"; Input=$null; Expect="20"},
    @{Name="unary_minus"; File="vm\valid\unary_minus.cvm"; Input=$null; Expect="-5"},
    @{Name="assignment"; File="vm\valid\assignment.cvm"; Input=$null; Expect="2"},
    @{Name="while_print"; File="vm\valid\while_print.cvm"; Input=$null; Expect="1`n2`n3"},
    @{Name="if_then"; File="vm\valid\if_then.cvm"; Input=$null; Expect="11"},
    @{Name="if_else"; File="vm\valid\if_else.cvm"; Input=$null; Expect="22"},
    @{Name="shadowing"; File="vm\valid\shadowing.cvm"; Input=$null; Expect="2`n1"},
    @{Name="booleans_comparisons"; File="vm\valid\booleans_comparisons.cvm"; Input=$null; Expect="true`nfalse`n1`n1`n1"},
    @{Name="input"; File="vm\valid\input.cvm"; Input="42"; Expect="42"},
    @{Name="truthy_ints"; File="vm\valid\truthy_ints.cvm"; Input=$null; Expect="1`n3"},
    @{Name="empty_block"; File="vm\valid\empty_block.cvm"; Input=$null; Expect=""}
)

foreach ($case in $valid) {
    $path = Join-Path $PSScriptRoot $case.File
    if ($null -eq $case.Input) {
        $output = & $exe $path 2>&1
    } else {
        $output = $case.Input | & $exe $path 2>&1
    }

    $text = ($output -join "`n").Trim()
    $expect = $case.Expect.Trim()
    if ($LASTEXITCODE -eq 0 -and $text -eq $expect) {
        Write-Host "[PASS] vm/valid/$($case.Name)"
    } else {
        Write-Host "[FAIL] vm/valid/$($case.Name)"
        Write-Host "       expected: [$expect]"
        Write-Host "       actual:   [$text]"
        $failures++
    }
}

$invalid = @(
    @{Name="division_by_zero"; File="vm\invalid\division_by_zero.cvm"; Input=$null; MustContain="division by zero"},
    @{Name="bool_arithmetic"; File="vm\invalid\bool_arithmetic.cvm"; Input=$null; MustContain="expected integer in ADD"},
    @{Name="negate_bool"; File="vm\invalid\negate_bool.cvm"; Input=$null; MustContain="expected integer in NEGATE"},
    @{Name="bool_less_than"; File="vm\invalid\bool_less_than.cvm"; Input=$null; MustContain="expected integer in LT"},
    @{Name="input_non_integer"; File="vm\invalid\input_non_integer.cvm"; Input="abc"; MustContain="INPUT failed"},
    @{Name="undeclared_print"; File="vm\invalid\undeclared_print.cvm"; Input=$null; MustContain="undeclared variable"},
    @{Name="unsupported_semicolon"; File="vm\invalid\unsupported_semicolon.cvm"; Input=$null; MustContain="unknown character"},
    @{Name="unsupported_parentheses"; File="vm\invalid\unsupported_parentheses.cvm"; Input=$null; MustContain="unknown character"},
    @{Name="chained_comparison"; File="vm\invalid\chained_comparison.cvm"; Input=$null; MustContain="only one comparison"}
)

foreach ($case in $invalid) {
    $path = Join-Path $PSScriptRoot $case.File
    if ($null -eq $case.Input) {
        $output = & $exe $path 2>&1
    } else {
        $output = $case.Input | & $exe $path 2>&1
    }

    $text = ($output -join "`n")
    if ($LASTEXITCODE -ne 0 -and $text.Contains($case.MustContain)) {
        Write-Host "[PASS] vm/invalid/$($case.Name)"
    } else {
        Write-Host "[FAIL] vm/invalid/$($case.Name)"
        Write-Host "       expected error containing: $($case.MustContain)"
        Write-Host "       actual: $text"
        $failures++
    }
}

if ($failures -gt 0) {
    Write-Host "VM tests failed: $failures"
    exit 1
}

Write-Host "All VM tests passed."
