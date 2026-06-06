# Recursive File Folder Counter

Terminal-based C application that counts files and folders using three traversal algorithms:

- Recursive DFS
- Iterative DFS with an explicit stack
- BFS with an explicit queue

The program compares all three algorithms for the same scan task and prints file count, folder count, total items, maximum depth, skipped folders, access errors, and execution time.

## Features

- Scan a specific folder.
- Scan a specific drive such as `C:\`.
- Scan all accessible fixed drives.
- Compare Recursive DFS, Iterative DFS, and BFS.
- Skip Windows reparse points, symbolic links, and junctions to avoid cycles.
- Continue scanning when some folders are inaccessible.
- Accept folder paths containing spaces.

## Build

This project targets Windows and uses the Windows API.

With GCC installed, run:

```bat
build.bat
```

The executable will be created at:

```text
build\recursive_file_folder_counter.exe
```

## Run

```bat
build\recursive_file_folder_counter.exe
```

Menu:

```text
1. Scan a specific folder
2. Scan a specific drive
3. Scan all accessible fixed drives
4. Exit
```

Each scan automatically runs all three algorithms and prints a comparison table.

## Sample Output

```text
Scan target: C:\Users\Suhas\Documents

+----------------+------------+-------------+-------------+-----------+---------+---------------+------------+
| Algorithm      | Files      | Folders     | Total Items | Max Depth | Skipped | Access Errors | Time (sec) |
+----------------+------------+-------------+-------------+-----------+---------+---------------+------------+
| Recursive DFS  |      12450 |        1820 |       14270 |        16 |       3 |             7 |     1.8400 |
| Iterative DFS  |      12450 |        1820 |       14270 |        16 |       3 |             7 |     1.7600 |
| BFS            |      12450 |        1820 |       14270 |        16 |       3 |             7 |     1.9300 |
+----------------+------------+-------------+-------------+-----------+---------+---------------+------------+

Fastest algorithm: Iterative DFS
Result check: All algorithms produced matching counts.
```

## DAA Explanation

The computer filesystem can be modeled as a tree:

- A selected folder or drive is the root.
- Subfolders are child nodes.
- Files are leaf nodes.

The program traverses this tree and counts every reachable file and folder.

| Algorithm | Traversal Style | Data Structure | Time Complexity | Space Complexity |
|---|---|---|---|---|
| Recursive DFS | Depth-first | Call stack | `O(F + D)` | `O(H)` |
| Iterative DFS | Depth-first | Explicit stack | `O(F + D)` | `O(H)` average, up to `O(D)` |
| BFS | Level-order | Queue | `O(F + D)` | Up to `O(W)` |

Where:

- `F` is the number of files.
- `D` is the number of directories.
- `H` is the maximum folder depth.
- `W` is the maximum number of folders at one level.

All three algorithms visit every reachable file and directory once, so their theoretical time complexity is the same. Their practical time differs because of data structure overhead, traversal order, disk behavior, and operating system caching.

## Safety Notes

- `.` and `..` are skipped.
- Reparse points are skipped to avoid infinite traversal cycles.
- Inaccessible folders are counted as access errors and scanning continues.
- Full-computer scan means all accessible fixed drives. Removable, network, and optical drives are excluded.

## Suggested Tests

- Empty folder.
- Folder with only files.
- Folder with nested subfolders.
- Folder whose path contains spaces.
- Invalid path.
- Protected folder with permission errors.
- Folder containing a junction or symbolic link.
- Specific drive scan.
- All fixed drives scan.
