/**
 * server.js - Zero-dependency Node.js backend for the DAA Dashboard
 * ------------------------------------------------------------------
 * Uses only Node.js built-in modules (http, child_process, path, url).
 * No npm install required.
 *
 * How it works:
 *   1. Serves index.html to the browser on http://localhost:3000
 *   2. Accepts POST /scan { "path": "C:\\Users" } from the frontend
 *   3. Spawns the C exe with --json --path <dir>
 *   4. Returns the JSON result to the frontend
 *   5. Accepts POST /benchmark to run the synthetic benchmark
 */

const http         = require('http');
const { execFile } = require('child_process');
const path         = require('path');
const fs           = require('fs');

/* ------------------------------------------------------------------
 * Configuration
 * ------------------------------------------------------------------ */

const PORT    = 3000;
const EXE     = path.join(__dirname, '..', 'build', 'recursive_file_folder_counter.exe');
const HTML    = path.join(__dirname, 'index.html');

/* ------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------ */

function sendJson(res, statusCode, obj) {
    const body = JSON.stringify(obj);
    res.writeHead(statusCode, {
        'Content-Type':                'application/json',
        'Access-Control-Allow-Origin': '*',
        'Content-Length':              Buffer.byteLength(body)
    });
    res.end(body);
}

function sendHtml(res) {
    fs.readFile(HTML, 'utf8', (err, data) => {
        if (err) {
            res.writeHead(500);
            res.end('Could not load index.html');
            return;
        }
        res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
        res.end(data);
    });
}

function readBody(req, cb) {
    let body = '';
    req.on('data', chunk => { body += chunk; });
    req.on('end', () => {
        try { cb(null, JSON.parse(body)); }
        catch (e) { cb(new Error('Invalid JSON')); }
    });
}

/**
 * runScanner - Spawn the C exe and collect its JSON output.
 *
 * @param {string[]} args   Command-line arguments after the exe name.
 * @param {function} cb     Called with (error, parsedResult).
 */
function runScanner(args, cb) {
    execFile(EXE, args, { maxBuffer: 10 * 1024 * 1024 }, (err, stdout, stderr) => {
        if (err) {
            return cb(new Error(`Scanner failed: ${err.message}\n${stderr}`));
        }
        try {
            /* The exe prints one JSON object per scan result, one per line.
             * For the benchmark there will be two (deep + wide). */
            const lines   = stdout.trim().split('\n').filter(l => l.trim());
            const results = lines.map(l => JSON.parse(l));
            cb(null, results.length === 1 ? results[0] : results);
        } catch (e) {
            cb(new Error(`JSON parse error: ${e.message}\nOutput was:\n${stdout}`));
        }
    });
}

/* ------------------------------------------------------------------
 * HTTP Server
 * ------------------------------------------------------------------ */

const server = http.createServer((req, res) => {

    /* CORS pre-flight */
    if (req.method === 'OPTIONS') {
        res.writeHead(204, { 'Access-Control-Allow-Origin': '*',
                             'Access-Control-Allow-Methods': 'POST, GET, OPTIONS',
                             'Access-Control-Allow-Headers': 'Content-Type' });
        res.end();
        return;
    }

    /* Serve the dashboard */
    if (req.method === 'GET' && req.url === '/') {
        sendHtml(res);
        return;
    }

    /* POST /scan — scan a user-specified directory */
    if (req.method === 'POST' && req.url === '/scan') {
        readBody(req, (err, body) => {
            if (err) { return sendJson(res, 400, { error: err.message }); }

            const dirPath = (body && body.path) ? body.path.trim() : '';
            if (!dirPath) { return sendJson(res, 400, { error: 'No path provided.' }); }

            const cycleDetect = body.cycleDetection ? '1' : '0';
            const growthPol   = body.growthPolicy === 'linear' ? 'linear' : 'double';

            /* Pass options via environment variables read by the server;
             * the exe uses defaults (double, no cycle detection) when
             * called with --json --path so options are fixed for now. */
            runScanner(['--json', '--path', dirPath], (err, result) => {
                if (err) { return sendJson(res, 500, { error: err.message }); }
                sendJson(res, 200, { type: 'scan', result });
            });
        });
        return;
    }

    /* POST /benchmark — run synthetic deep+wide tree benchmark */
    if (req.method === 'POST' && req.url === '/benchmark') {
        /* The benchmark writes two JSON lines (deep + wide). */
        execFile(EXE, ['--json', '--benchmark'],
                 { maxBuffer: 10 * 1024 * 1024 },
                 (err, stdout, stderr) => {
            if (err) {
                return sendJson(res, 500, { error: `Benchmark failed: ${err.message}\n${stderr}` });
            }
            try {
                const lines = stdout.trim().split('\n').filter(l => l.trim());
                if (lines.length < 2) {
                    return sendJson(res, 500, {
                        error: `Expected 2 JSON lines, got ${lines.length}.`,
                        rawStdout: stdout.substring(0, 1000)
                    });
                }
                sendJson(res, 200, {
                    type:     'benchmark',
                    deepTree: JSON.parse(lines[0]),
                    wideTree: JSON.parse(lines[1])
                });
            } catch (e) {
                sendJson(res, 500, { error: `JSON parse error: ${e.message}` });
            }
        });
        return;
    }

    res.writeHead(404);
    res.end('Not found');
});

server.listen(PORT, '127.0.0.1', () => {
    console.log(`\n  DAA Dashboard running at http://localhost:${PORT}\n`);
    console.log(`  C backend: ${EXE}\n`);
});
