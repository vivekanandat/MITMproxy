// Tails a newline-delimited JSON log file written by the C++ proxy
// and inserts each event into Postgres, then emits it over Socket.io
// to any dashboard clients watching that session.
//
// Expected log line shape (one JSON object per line):
// {
//   "session_id": "uuid",
//   "method": "GET",
//   "host": "example.com",
//   "path": "/index.html",
//   "status_code": 200,
//   "request_size": 512,
//   "response_size": 2048,
//   "tls": true,
//   "flagged": false,
//   "flagged_reason": null,
//   "headers": { ... },
//   "body_preview": "..."
// }

const fs = require("fs");
const readline = require("readline");
const pool = require("../db/pool");

function startIngestWorker(logPath, io) {
  console.log(`Ingest worker watching ${logPath}`);

  // Ensure the file exists so tailing doesn't error out immediately
  if (!fs.existsSync(logPath)) {
    fs.writeFileSync(logPath, "");
  }

  let position = fs.statSync(logPath).size;

  fs.watch(logPath, async () => {
    const stats = fs.statSync(logPath);
    if (stats.size <= position) return; // truncated or no new data

    const stream = fs.createReadStream(logPath, { start: position, end: stats.size });
    position = stats.size;

    const rl = readline.createInterface({ input: stream });
    for await (const line of rl) {
      if (!line.trim()) continue;
      try {
        const event = JSON.parse(line);
        const result = await pool.query(
          `INSERT INTO requests
             (session_id, method, host, path, status_code, request_size,
              response_size, tls, flagged, flagged_reason, headers, body_preview)
           VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12)
           RETURNING *`,
          [
            event.session_id,
            event.method,
            event.host,
            event.path,
            event.status_code,
            event.request_size || 0,
            event.response_size,
            !!event.tls,
            !!event.flagged,
            event.flagged_reason || null,
            event.headers || {},
            event.body_preview || null,
          ]
        );

        const saved = result.rows[0];
        io.to(`session:${event.session_id}`).emit("new_request", saved);
      } catch (err) {
        console.error("Failed to ingest log line:", err.message);
      }
    }
  });
}

module.exports = { startIngestWorker };
