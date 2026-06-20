# Recursive File Folder Counter (DAA Edition)

A high-performance Windows C application and interactive Node.js Web Dashboard designed to analyze, benchmark, and visualize fundamental tree-traversal algorithms (**Recursive DFS**, **Iterative DFS**, and **BFS**) under the lens of **Design and Analysis of Algorithms (DAA)**.

The system performs real-world scans on arbitrary directories, stress-tests itself against extreme synthetic directory structures, and provides empirical validation for time/space complexity, amortized growth analysis, and graph theory cycle detection.

---

## 🚀 Quick Start

### 1. Build the C Backend
This project targets Windows and uses native Win32 APIs for high-performance directory traversal. With GCC installed, compile the backend using the provided batch script:
```powershell
.\build.bat
```
The compiled executable is created at `build\recursive_file_folder_counter.exe`.

### 2. Run the Interactive Web Dashboard
The web dashboard is a zero-dependency application that does not require any `npm install` packages.
```powershell
cd frontend
node server.js
```
Open **[http://localhost:3000](http://localhost:3000)** in your browser.

---

## 🎨 Web Dashboard features

* **Visual Comparison Charts**: Real-time bar charts rendering **Execution Time (ns)**, **Peak Memory Nodes**, **Reallocation Count**, and **Time per Node (ns)**.
* **Empirical Insights & Takeaways**: Dynamic analysis card generated on every directory scan and benchmark, explaining *why* a particular algorithm was faster/slower and *how* the directory shape (wide vs. deep) impacted the space footprints.
* **Interactive Sibling Visualizations**: Fully animated SVG tree widgets showcasing DFS (pre-order) and BFS (level-order) traversal patterns in real-time.
* **CLI fallback**: Standard interactive command-line interface available by running `build\recursive_file_folder_counter.exe`.

---

## 🧪 Synthetic Benchmark Stress Tests

The application generates extreme synthetic filesystem structures inside the system's temporary directory to test boundary-case behaviors:

1. **Deep Tree (Chain Graph)**: 
   * *Structure*: 200 nested levels, 1 subfolder per level.
   * *DAA Goal*: Validates **$O(h)$ space complexity** for DFS. Highlights the hardware call stack frame accumulation in Recursive DFS vs. the lightweight iterative heap approach.
   * *Win32 Bypass*: Prepends the Win32 extended path prefix (`\\?\`) to bypass the legacy 260-character path limit (`MAX_PATH`) and create the full 200-level directory chain.
2. **Wide Tree (Star Graph)**:
   * *Structure*: 1,000 folders directly at the root level.
   * *DAA Goal*: Validates **$O(w)$ space complexity** for BFS. Proves how BFS is forced to hold the entire level's frontier simultaneously, consuming $1000\times$ more memory than Recursive DFS on flat hierarchies.

---

## 🔬 Core DAA Concepts Evaluated

| Feature & Concept | implementation details |
| :--- | :--- |
| **Space Complexity & Peak Memory** | Tracks `peakMemoryNodes` representing the maximum occupancy of the DFS Stack / BFS Queue. Proves DFS is bound by height $O(h)$ and BFS is bound by width $O(w)$ dynamically. |
| **Amortized Analysis (Resizing)** | Compares **Double Growth** (`GROWTH_DOUBLE` - capacity doubles on resize, $O(1)$ amortized push) vs **Linear Growth** (`GROWTH_LINEAR` - capacity grows by a fixed step, $O(N)$ amortized push, $O(N^2)$ total copy time) via `reallocCount`. |
| **Constant-Factor Isolation** | The `Time/Node (ns)` metric isolates the constant factor $c$ hidden inside $O(N)$ time complexity. This quantifies the performance difference between recursion (compiler/hardware call stack overhead) and custom heap-allocated structures. |
| **Graph Theory Cycle Detection** | An open-addressing hash table (`hashtable.c`) stores filesystem device IDs to dynamically identify and skip genuine directory loops (e.g., recursive junctions, circular mount points) rather than relying solely on file attribute flags. |

---

## 🛡️ Stability & Memory Optimizations

* **Stack Overflow Prevention**: The recursive DFS algorithm has been optimized to use **dynamic heap buffers** (`malloc` / `free`) for search patterns and path strings instead of allocating massive stack buffers (`char path[32768]`). This reduces each stack frame from $\sim 64\text{KB}$ to just a few pointers ($\sim 16\text{B}$), allowing safe traversal to any depth.
* **Leak Protection**: Cleanups are integrated on all error conditions and traversal termination paths to guarantee zero memory leaks.
* **Reparse Point Skipping**: Default configurations filter out junction points, symbolic links, and circular reparse targets to keep scans safe and accurate.
