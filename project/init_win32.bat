@echo off

SET PGTECH_DIR=C:\Users\Drio\Documents\pgtech

mkdir data
mkdir code\include
mkdir code\libraries

if exist code\include\engine			rd /s /q code\include\engine
if exist code\include\runtime			rd /s /q code\include\runtime
if exist code\include\third_party 	rd /s /q code\include\third_party
if exist code\libraries\engine			rd /s /q code\libraries\engine
if exist code\libraries\third_party	rd /s /q code\libraries\third_party
if exist code\platform\win32\src 	rd /s /q code\platform\win32\src

mklink /j code\include\engine 			%PGTECH_DIR%\pgtech\lib_engine\include
mklink /j code\include\runtime			%PGTECH_DIR%\pgtech\lib_runtime\include
mklink /j code\include\third_party 	%PGTECH_DIR%\pgtech\third_party
mklink /j code\libraries\engine 		%PGTECH_DIR%\pgtech\platform\win32\build
mklink /j code\libraries\third_party	%PGTECH_DIR%\pgtech\platform\win32\libraries
mklink /j code\platform\win32\src 	code\src

pause


