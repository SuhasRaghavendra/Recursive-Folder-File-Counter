@echo off
setlocal

REM ============================================================
REM  build.bat - Compile the Recursive File Folder Counter
REM ============================================================
REM  Source files:
REM    main.c       - CLI entry point and interactive menu
REM    scanner.c    - Recursive DFS, Iterative DFS, and BFS algorithms
REM    stack.c      - Dynamic heap stack (used by Iterative DFS)
REM    queue.c      - Dynamic circular queue (used by BFS)
REM    hashtable.c  - Open-addressing hash table (cycle detection)
REM    synth.c      - Synthetic tree generator (DAA benchmark)
REM    output.c     - JSON serialiser (frontend integration)
REM    utils.c      - Path helpers, timer, and table printing
REM ============================================================

if not exist "build" (
    md "build"
)

gcc -Wall -Wextra -std=c11 -O2 ^
    src\main.c       ^
    src\scanner.c    ^
    src\stack.c      ^
    src\queue.c      ^
    src\hashtable.c  ^
    src\synth.c      ^
    src\output.c     ^
    src\utils.c      ^
    -o "build\recursive_file_folder_counter.exe"

if errorlevel 1 (
    echo.
    echo Build FAILED. Check the errors above.
    exit /b 1
)

echo.
echo Build successful: build\recursive_file_folder_counter.exe
echo.
echo Usage:
echo   build\recursive_file_folder_counter.exe              -- interactive mode
echo   build\recursive_file_folder_counter.exe --json       -- JSON output mode
echo   build\recursive_file_folder_counter.exe --json --path "C:\Users"
