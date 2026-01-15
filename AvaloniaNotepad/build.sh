#!/bin/bash

# Restore dependencies
dotnet restore

# Build for Linux
dotnet publish -c Release -r linux-x64 --self-contained

# Build for macOS (Intel)
dotnet publish -c Release -r osx-x64 --self-contained

# Build for macOS (Apple Silicon)
dotnet publish -c Release -r osx-arm64 --self-contained
