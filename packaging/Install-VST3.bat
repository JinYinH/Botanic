@echo off
title Botanic VST3 installer
echo Installing Botanic VST3 ...
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0install.ps1"
echo.
pause
