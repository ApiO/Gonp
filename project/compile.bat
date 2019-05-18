@echo off


SET PGTECH_DIR=C:\Users\Drio\Documents\pgtech

set WIN32_BUILD_PATH=%PGTECH_DIR%\pgtech\platform\win32\build\Debug\

call %WIN32_BUILD_PATH%compile_server.exe cfg: %WIN32_BUILD_PATH%data sch:  %WIN32_BUILD_PATH%schemas src: resources data: data

pause