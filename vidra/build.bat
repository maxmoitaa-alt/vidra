@echo off
setlocal
cd /d "%~dp0"
where cmake >nul 2>&1 || (echo [x] instale cmake & exit /b 1)
where ninja >nul 2>&1 || (echo [x] instale ninja & exit /b 1)
where python >nul 2>&1 || (echo [x] instale python & exit /b 1)
python tools\tokens_gen.py || exit /b 1
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1
cmake --build build -j %NUMBER_OF_PROCESSORS%
if errorlevel 1 exit /b 1
echo pronto: %CD%\build\vidra.exe
endlocal
