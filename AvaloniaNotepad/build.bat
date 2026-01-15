@echo off

REM Restore dependencies
dotnet restore

REM Build for Windows
dotnet publish -c Release -r win-x64 --self-contained
