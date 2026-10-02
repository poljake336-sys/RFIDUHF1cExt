@echo off
setlocal
cd /d "%~dp0android"

rem Только ASCII-пути: проект E:\RFIDUHF1cExt и SDK C:\Android\Sdk
set JAVA_HOME=C:\Program Files\Android\Android Studio\jbr
set ANDROID_HOME=C:\Android\Sdk
set ANDROID_SDK_ROOT=C:\Android\Sdk
set PATH=%JAVA_HOME%\bin;%ANDROID_HOME%\cmake\3.22.1\bin;%PATH%

echo JAVA_HOME=%JAVA_HOME%
echo ANDROID_HOME=%ANDROID_HOME%
echo Project=%CD%

echo === assembleRelease ===
call gradlew.bat assembleRelease --stacktrace
if errorlevel 1 (
  echo BUILD FAILED
  exit /b 1
)

cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\Make-Package.ps1"
echo === Done: dist\RFIDUHF.zip ===
dir "%~dp0dist"
