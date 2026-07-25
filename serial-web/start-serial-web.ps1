$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$serverScript = Join-Path $root "server.js"

if (-not (Test-Path -LiteralPath $serverScript)) {
    throw "未找到 server.js：$serverScript"
}

$node = Get-Command node -ErrorAction Stop

Write-Host "Serial web is running at http://localhost:8080/"
Write-Host "Press Ctrl+C to stop."

& $node.Source $serverScript
