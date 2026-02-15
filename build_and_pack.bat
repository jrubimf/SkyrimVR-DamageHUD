@echo off
setlocal

set "ROOT=%~dp0"
set "BUILD=%ROOT%build"
set "STAGING=%ROOT%build\package"
set "ZIP=%ROOT%build\FloatingDamage.zip"

echo === Building FloatingDamage ===
cmake --preset vr-release 2>nul
cmake --build "%BUILD%" --config Release
if %ERRORLEVEL% neq 0 (
    echo BUILD FAILED
    exit /b 1
)

echo === Packaging ===
if exist "%STAGING%" rmdir /s /q "%STAGING%"
if exist "%ZIP%" del "%ZIP%"

mkdir "%STAGING%\SKSE\Plugins"
mkdir "%STAGING%\MCM\Config\FloatingDamage"

copy "%BUILD%\Release\FloatingDamage.dll" "%STAGING%\SKSE\Plugins\" >nul
copy "%ROOT%MCM\Config\FloatingDamage\config.json" "%STAGING%\MCM\Config\FloatingDamage\" >nul
copy "%ROOT%MCM\Config\FloatingDamage\settings.ini" "%STAGING%\MCM\Config\FloatingDamage\" >nul

powershell -NoProfile -Command "Compress-Archive -Path '%STAGING%\*' -DestinationPath '%ZIP%'"

echo === Done ===
echo Output: %ZIP%
