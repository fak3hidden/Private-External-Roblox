@echo off
setlocal
title UCR Updater

REM =============================================================
REM  UCR repo updater
REM  Pulls the latest code and replaces local files with it.
REM  Run it from the repo folder (where this file lives).
REM =============================================================

set "BRANCH=arena/01a07d35-private-external-roblox"
set "REMOTE=https://github.com/fak3hidden/Private-External-Roblox.git"

cd /d "%~dp0"

echo ============================================================
echo  UCR updater  ^|  branch: %BRANCH%
echo ============================================================
echo.

REM --- is git installed? ----------------------------------------
where git >nul 2>nul
if errorlevel 1 (
    echo [ERR] git was not found on PATH.
    echo        Install it from https://git-scm.com/download/win
    echo        (default options are fine), then run this again.
    goto fail
)

REM --- first run: this folder is a plain ZIP extract, no repo ----
if not exist ".git" (
    echo [*] No .git folder here - first run. Setting this folder up as a repo...
    git init -q
    if errorlevel 1 goto fail
    git remote remove origin >nul 2>nul
    git remote add origin "%REMOTE%"
)

echo [*] Fetching latest code from origin...
git fetch --prune origin "+refs/heads/%BRANCH%:refs/remotes/origin/%BRANCH%"
if errorlevel 1 (
    echo [ERR] Fetch failed - check your internet connection and try again.
    goto fail
)

REM --- replace old files with the latest tracked version --------
echo [*] Replacing local files with the latest %BRANCH% ...
git checkout -B "%BRANCH%" "origin/%BRANCH%" --force
if errorlevel 1 (
    echo [ERR] Checkout failed.
    goto fail
)

REM --- make a plain `git pull` work from now on -----------------
git branch --set-upstream-to="origin/%BRANCH%" "%BRANCH%" >nul 2>nul

echo.
echo [OK] Updated. You are now on:
git log -1 --oneline
echo.
echo Files were replaced with the latest version.
echo You can also run `git pull` here directly from now on.
goto end

:fail
echo.
echo [!] Update did NOT complete - read the message above.

:end
echo.
pause
