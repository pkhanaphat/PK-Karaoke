@echo off
"C:\Program Files\CMake\bin\cmake.exe" -S . -B Build -G "Visual Studio 18 2026"
echo Exit code: %ERRORLEVEL%
