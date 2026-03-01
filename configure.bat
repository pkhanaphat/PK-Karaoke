@echo off
call "D:\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" 10.0.22621.0
"C:\Program Files\CMake\bin\cmake.exe" -S . -B Build -G "Visual Studio 17 2022"
echo Exit code: %ERRORLEVEL%
