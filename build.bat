@echo off
REM Compila o usbscope.exe no Windows. Detecta MSVC (cl) ou MinGW (g++).
setlocal
cd /d "%~dp0"

set SRC=src\main.cpp src\term_win.cpp src\usbenum_win.cpp src\textutil.cpp src\banner.cpp src\usbdesc.cpp src\usbids.cpp src\device.cpp src\app.cpp

where cl >nul 2>nul
if %errorlevel%==0 (
    echo [MSVC] compilando...
    cl /nologo /std:c++17 /O2 /EHsc /utf-8 /Feusbscope.exe %SRC% setupapi.lib cfgmgr32.lib
    if %errorlevel%==0 ( echo. & echo ok -^> usbscope.exe ) else ( echo FALHOU )
    del *.obj >nul 2>nul
    goto :eof
)

where g++ >nul 2>nul
if %errorlevel%==0 (
    echo [MinGW] compilando...
    g++ -std=c++17 -O2 -municode -Wall %SRC% -o usbscope.exe -lsetupapi -lcfgmgr32 -static -static-libgcc -static-libstdc++
    if %errorlevel%==0 ( echo. & echo ok -^> usbscope.exe ) else ( echo FALHOU )
    goto :eof
)

echo Nenhum compilador encontrado.
echo  - MSVC: abra o "x64 Native Tools Command Prompt for VS" e rode build.bat
echo  - MinGW: instale o MSYS2 e o pacote mingw-w64-x86_64-gcc, e rode build.bat
exit /b 1
