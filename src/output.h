/*
 * output.h - JSON Output Mode for Frontend Integration
 * -----------------------------------------------------
 * Declares functions that serialize ComparisonResult data to JSON
 * format on stdout. When the program is invoked with the --json flag,
 * all output goes through this module instead of the human-readable
 * table printer in utils.c.
 *
 * Frontend Integration Design:
 *   The executable acts as a backend process. A web or desktop frontend
 *   spawns it as a child process with the --json flag and reads its
 *   stdout. Because the output is self-contained JSON, the frontend
 *   can parse it with any standard JSON library without needing to
 *   understand the C code at all.
 *
 *   Example invocation from a frontend (Node.js):
 *     const proc = spawn('recursive_file_folder_counter.exe',
 *                        ['--json', '--path', 'C:\\Users']);
 *     proc.stdout → parse JSON → render charts / tables.
 *
 * JSON Schema (one object per run):
 *   {
 *     "target": "<path>",
 *     "growthPolicy": "double" | "linear",
 *     "algorithms": {
 *       "recursiveDfs": { <AlgorithmStats> },
 *       "iterativeDfs": { <AlgorithmStats> },
 *       "bfs":          { <AlgorithmStats> }
 *     },
 *     "fastest": "<algorithm name>",
 *     "countsMatch": true | false
 *   }
 *
 *   AlgorithmStats fields:
 *     fileCount, folderCount, totalItems, maxDepth,
 *     skippedFolders, accessErrors, timeTaken,
 *     peakMemoryNodes, reallocCount, timePerNodeNs, cyclesDetected
 */

#ifndef OUTPUT_H
#define OUTPUT_H

#include "scanner.h"

/*
 * printJsonResult - Emit the full ComparisonResult as a JSON object.
 *
 * Writes a single, self-contained JSON object to stdout. The output
 * includes all ScanStats fields for each algorithm plus metadata
 * (target path, growth policy, fastest algorithm, counts match flag).
 *
 * Parameters:
 *   targetPath - The directory or drive that was scanned.
 *   result     - Pointer to the ComparisonResult from runComparison().
 *   options    - Pointer to the ScanOptions used for the run (needed
 *                to include growthPolicy in the JSON).
 */
void printJsonResult(const char *targetPath,
                     const ComparisonResult *result,
                     const ScanOptions *options);

#endif /* OUTPUT_H */
