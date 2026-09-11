# UCR updater (PowerShell) — works WITHOUT git.
# Downloads the latest branch ZIP from GitHub and replaces local files.
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$branch = 'arena/01a07d35-private-external-roblox'
$repo   = 'https://github.com/fak3hidden/Private-External-Roblox'
$url    = "$repo/archive/refs/heads/$branch.zip"

# The repo root is the folder this script lives in (update.cmd calls us from there too).
$target = Split-Path -Parent $MyInvocation.MyCommand.Path
$tmp    = Join-Path $env:TEMP 'ucr-update'
$zip    = Join-Path $env:TEMP 'ucr-update.zip'

try {
    Write-Host "[*] Downloading $url" -ForegroundColor Cyan
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Invoke-WebRequest -Uri $url -OutFile $zip -UseBasicParsing

    Write-Host "[*] Extracting..." -ForegroundColor Cyan
    if (Test-Path $tmp) { Remove-Item -Recurse -Force $tmp }
    Expand-Archive -Path $zip -DestinationPath $tmp -Force

    $src = (Get-ChildItem $tmp -Directory | Select-Object -First 1).FullName
    if (-not $src) { throw 'Extracted archive has no top-level folder.' }

    Write-Host "[*] Replacing files in: $target" -ForegroundColor Cyan
    # /E copy subdirs, /IS force-overwrite even "same" files, /R/W minimal retries.
    robocopy $src $target /E /IS /NFL /NDL /NP /NJH /NJS /R:1 /W:1 | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "robocopy failed (exit $LASTEXITCODE)" }

    Write-Host "[OK] Updated to $branch." -ForegroundColor Green
}
finally {
    Remove-Item -Force $zip -ErrorAction SilentlyContinue
    Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
}
