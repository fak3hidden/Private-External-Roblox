@echo off
setlocal
title UCR Updater

REM =============================================================
REM  UCR repo updater
REM  Pulls the latest code and replaces local files with it.
REM  Tries git first (so plain `git pull` works afterwards);
REM  falls back to a built-in PowerShell downloader that needs
REM  nothing installed. Run it from the repo folder.
REM =============================================================

set "BRANCH=arena/01a07d35-private-external-roblox"
set "REMOTE=https://github.com/fak3hidden/Private-External-Roblox.git"

cd /d "%~dp0"

echo ============================================================
echo  UCR updater  ^|  branch: %BRANCH%
echo ============================================================
echo.

REM --- git present? ---------------------------------------------
where git >nul 2>nul
if errorlevel 1 (
    echo [*] git not installed - using the built-in download updater.
    goto ps
)

REM --- first run in a ZIP-extracted folder: no repo yet ---------
if not exist ".git" (
    echo [*] No .git folder here - first run, initializing...
    git init -q 2>nul
    git remote remove origin >nul 2>nul
    git remote add origin "%REMOTE%"
)

echo [*] Trying git update...
git fetch --prune origin "+refs/heads/%BRANCH%:refs/remotes/origin/%BRANCH%" >nul 2>nul
if errorlevel 1 (
    echo [*] git fetch failed - falling back to the download updater.
    goto ps
)

git checkout -B "%BRANCH%" "origin/%BRANCH%" --force >nul 2>nul
if errorlevel 1 (
    echo [*] git checkout failed - falling back to the download updater.
    goto ps
)

git branch --set-upstream-to="origin/%BRANCH%" "%BRANCH%" >nul 2>nul

echo [OK] Updated via git. `git pull` works here from now on.
git log -1 --oneline
goto done

REM --- no-git / git-broken path: PowerShell download + replace ---
:ps
echo [*] Downloading latest code and replacing files...
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0update.ps1"
if errorlevel 1 (
    echo [ERR] Update did NOT complete - see the message above.
    goto end
)

:done
echo.
echo Update finished.

:end
echo.
pause
