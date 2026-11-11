@echo off
setlocal enabledelayedexpansion
title DaMA PulseForge - VST3 Installer
color 0B

echo.
echo  =============================================================
echo    DaMA PulseForge - One-Click VST3 Installer for Windows
echo  =============================================================
echo.

set "SYS_VST3_DIR=C:\Program Files\Common Files\VST3\DaMA"
set "TARGET=%SYS_VST3_DIR%\PulseForge.vst3"

:: Determine source path
set "SRC=%~dp0PulseForge.vst3"
if not exist "!SRC!" (
    if exist "%~dp0build\PulseForge_artefacts\Release\VST3\PulseForge.vst3" (
        set "SRC=%~dp0build\PulseForge_artefacts\Release\VST3\PulseForge.vst3"
    )
)

if not exist "!SRC!" (
    color 0C
    echo [ERRORE] File 'PulseForge.vst3' non trovato nella cartella corrente!
    echo Assicurati che lo script si trovi nella stessa cartella del file PulseForge.vst3.
    echo.
    pause
    exit /b 1
)

:: Check if Ableton Live or other DAW is running
tasklist /FI "IMAGENAME eq Live.exe" 2>NUL | find /I /N "Live.exe">NUL
if "%ERRORLEVEL%"=="0" (
    color 0E
    echo [ATTENZIONE] Ableton Live e attualmente aperto!
    echo Si consiglia di chiudere Ableton Live per permettere la sovrascrittura del plugin.
    echo.
    set /p "RETRY=Premi INVIO dopo aver chiuso Ableton Live per continuare..."
)

echo.
echo [*] Creazione cartella di destinazione: %SYS_VST3_DIR%
if not exist "%SYS_VST3_DIR%" (
    mkdir "%SYS_VST3_DIR%" >nul 2>&1
)

echo [*] Copia di PulseForge.vst3 nella directory VST3 di sistema...
xcopy /E /I /Y /Q "!SRC!" "!TARGET!" >nul 2>&1

if "%ERRORLEVEL%"=="0" (
    color 0A
    echo.
    echo [*] Reset cache di scansione Ableton Live (rimozione vecchie versioni memorizzate)...
    powershell -NoProfile -Command "Get-ChildItem -Path ('$env:APPDATA\Ableton') -Filter 'PluginScanDb.txt' -Recurse -ErrorAction SilentlyContinue | ForEach-Object { (Get-Content $_.FullName) | Where-Object { $_ -notmatch 'PulseForge' } | Set-Content $_.FullName }" >nul 2>&1
    echo [OK] Cache di scansione Ableton resettata: la DAW effettuera una scansione pulita all'avvio!

    echo.
    echo  =============================================================
    echo    INSTALLAZIONE COMPLETATA CON SUCCESSO!
    echo  =============================================================
    echo.
    echo  Il plugin e stato installato in:
    echo  !TARGET!
    echo.
    echo  Puoi avviare Ableton Live / la tua DAW e caricare DaMA PulseForge!
    echo.
) else (
    color 0C
    echo.
    echo  =============================================================
    echo   [ERRORE] Accesso Negato durante la copia.
    echo  =============================================================
    echo.
    echo   Windows richiede i permessi di Amministratore per scrivere
    echo   in 'C:\Program Files\Common Files\VST3\'.
    echo.
    echo   Fai click destro su questo file 'install_vst3.bat'
    echo   e seleziona 'Esegui come amministratore'.
    echo.
)

pause
