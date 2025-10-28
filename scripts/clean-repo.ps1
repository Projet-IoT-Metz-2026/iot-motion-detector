<#
  clean-repo.ps1
  Safe helper to remove local build artifacts and sensitive files from working tree.
  Run from the repository root in PowerShell.
#>

Write-Host "Cleaning local build artifacts and sensitive files (dry run by default)" -ForegroundColor Cyan

param(
    [switch]$Force
)

$targets = @(
    "backend\src\IoTHubProcessor\bin",
    "backend\src\IoTHubProcessor\obj",
    "**\bin\Debug*",
    "**\obj\Debug*",
    "infrastructure\dps-credentials.env"
)

foreach ($t in $targets) {
    $foundItems = Get-ChildItem -Path $t -ErrorAction SilentlyContinue -Force
    if (-not $foundItems) {
        continue
    }
    foreach ($m in $foundItems) {
        if ($Force) {
            Remove-Item -LiteralPath $m.FullName -Recurse -Force -ErrorAction SilentlyContinue
            Write-Host "Removed: $($m.FullName)" -ForegroundColor Yellow
        }
        else {
            Write-Host "Would remove: $($m.FullName)" -ForegroundColor DarkGray
        }
    }
}

Write-Host "Done. Rerun with -Force to actually delete these files." -ForegroundColor Green
