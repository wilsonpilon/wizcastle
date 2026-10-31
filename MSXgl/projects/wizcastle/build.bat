:: ____________________________
:: ██▀▀█▀▀██▀▀▀▀▀▀▀█▀▀█        │   ▄▄▄                ▄▄      
:: ██  ▀  █▄  ▀██▄ ▀ ▄█ ▄▀▀ █  │  ▀█▄  ▄▀██ ▄█▄█ ██▀▄ ██  ▄███
:: █  █ █  ▀▀  ▄█  █  █ ▀▄█ █▄ │  ▄▄█▀ ▀▄██ ██ █ ██▀  ▀█▄ ▀█▄▄
:: ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀────────┘                 ▀▀
::  The Wizard's Castle - MSX2+ Deluxe Edition build script
::────────────────────────────────────────────────────────────────────
@echo off

if not defined MSXGL_PATH set MSXGL_PATH=..\..

%MSXGL_PATH%\tools\build\Node\node.exe %MSXGL_PATH%\engine\script\js\build.js %1 %2 %3 %4 %5 %6 %7 %8 %9
