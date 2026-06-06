@echo off
setlocal

if not exist "build" (
    md "build"
)

gcc -Wall -Wextra -std=c11 -O2 ^
    src\main.c ^
    src\scanner.c ^
    src\stack.c ^
    src\queue.c ^
    src\utils.c ^
    -o "build\recursive_file_folder_counter.exe"

if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

echo Build successful: build\recursive_file_folder_counter.exe
