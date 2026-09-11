@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"
title Ollama AI Booter

REM =============================================================
REM  Ollama AI Booter
REM  Creates a coding-assistant model ("ucr-dev") that is told how
REM  to help with the private external/internal project, using the
REM  instructions in SystemPrompt.txt.
REM
REM  Usage:
REM    ollamaaibooter.bat                    (interactive model picker)
REM    ollamaaibooter.bat qwen2.5-coder:7b   (skip picker, use this model)
REM
REM  No Ollama? The SAME instructions work in LM Studio
REM  (lmstudio.ai): load any model and paste SystemPrompt.txt into
REM  the chat's System Prompt box. See the on-screen notes at the end.
REM =============================================================

set "MODEL_NAME=ucr-dev"
set "BASE_MODEL=%~1"

if "%~1"=="" (
    echo.
    echo  Pick the base model for %MODEL_NAME%:
    echo.
    echo   1. llama3.1:8b            ^(general, the default^)
    echo   2. qwen2.5-coder:7b       ^(small + fast, code-focused^)
    echo   3. qwen2.5-coder:14b      ^(better code reasoning^)
    echo   4. qwen2.5-coder:32b      ^(strongest, needs a big GPU^)
    echo   5. deepseek-coder-v2:16b
    echo   6. codellama:13b
    echo   7. mistral:7b
    echo   8. gemma2:9b
    echo.
    echo   ...or type ANY Ollama model tag, e.g.  phi3:14b  or  llama3.1:70b
    echo.
    set /p "BASE_MODEL=Model [1-8 or a tag]: "

    if "!BASE_MODEL!"=="1"  set "BASE_MODEL=llama3.1:8b"
    if "!BASE_MODEL!"=="2"  set "BASE_MODEL=qwen2.5-coder:7b"
    if "!BASE_MODEL!"=="3"  set "BASE_MODEL=qwen2.5-coder:14b"
    if "!BASE_MODEL!"=="4"  set "BASE_MODEL=qwen2.5-coder:32b"
    if "!BASE_MODEL!"=="5"  set "BASE_MODEL=deepseek-coder-v2:16b"
    if "!BASE_MODEL!"=="6"  set "BASE_MODEL=codellama:13b"
    if "!BASE_MODEL!"=="7"  set "BASE_MODEL=mistral:7b"
    if "!BASE_MODEL!"=="8"  set "BASE_MODEL=gemma2:9b"
)

if "!BASE_MODEL!"=="" set "BASE_MODEL=llama3.1:8b"

echo.
echo ============================================================
echo  Ollama AI Booter
echo  Model: %MODEL_NAME%    base: !BASE_MODEL!
echo ============================================================
echo.

if not exist "SystemPrompt.txt" (
    echo [ERR] SystemPrompt.txt not found next to this script.
    goto end
)

REM --- build the Modelfile for Ollama ---------------------------
>Modelfile echo FROM !BASE_MODEL!
>>Modelfile echo.
>>Modelfile echo SYSTEM """
type SystemPrompt.txt >> Modelfile
>>Modelfile echo """
echo [OK] Wrote Modelfile.

REM --- run with Ollama if it is installed -----------------------
where ollama >nul 2>nul
if errorlevel 1 goto lmstudio

echo [*] Creating model "%MODEL_NAME%" ...
echo     First run downloads !BASE_MODEL! - this can take a while.
ollama create %MODEL_NAME% -f Modelfile
if errorlevel 1 (
    echo [ERR] ollama create failed. Is the Ollama app running?
    goto lmstudio
)

echo.
echo [OK] Model ready. Launching it now.
echo     Type /bye to exit, /help for commands.
echo.
ollama run %MODEL_NAME%
goto end

:lmstudio
echo.
echo ------------------------------------------------------------------
echo  LM Studio path (no Ollama needed):
echo   1. Install LM Studio from https://lmstudio.ai
echo   2. Search + download a model, e.g. "Llama 3.1 8B Instruct" or
echo      "Qwen2.5 Coder 7B Instruct"
echo   3. Load the model, then in the chat open the System Prompt and
echo      paste the ENTIRE contents of this file:
echo          %~dp0SystemPrompt.txt
echo   4. Optional: enable the Local Server (port 1234) to use the model
echo      as an OpenAI-compatible endpoint from other tools.
echo ------------------------------------------------------------------
goto end

:end
echo.
pause
