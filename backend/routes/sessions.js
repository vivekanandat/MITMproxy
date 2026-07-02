const express = require("express");
const pool = require("../db/pool");
const { verifyAccessToken } = require("../middleware/auth");
const { spawn } = require("child_process");
const path = require("path");


const router = express.Router();
router.use(verifyAccessToken);

// POST /api/sessions - start a new capture session
router.post("/", async (req, res) => {
  const { name } = req.body;
  const result = await pool.query(
    "INSERT INTO sessions (name, user_id) VALUES ($1, $2) RETURNING *",
    [name || `Capture ${new Date().toISOString()}`, req.user.id]
  );
    const port=8080;
    const session=result.rows[0];
    const proxyProcess = spawn(path.join(__dirname, "../../MITMproxy/sp"), [port.toString(), session.id], {
      detached: true,
      cwd: path.join(__dirname, "../../MITMproxy"),
    });
    proxyProcess.unref();
    //store pid
    await pool.query("UPDATE sessions SET proxy_pid = $1 WHERE id = $2", [proxyProcess.pid, session.id]);

    proxyProcess.stdout.on("data", (d) => console.log(`[proxy] ${d}`));
    proxyProcess.stderr.on("data", (d) => console.error(`[proxy err] ${d}`));
    proxyProcess.on("error", (err) => {
      console.error("Failed to start proxy:", err);
    });
  res.status(201).json(session);
});

// PATCH /api/sessions/:id/end
router.patch("/:id/end", async (req, res) => {
    console.log("end");
  const session = await pool.query("SELECT * FROM sessions WHERE id = $1", [req.params.id]);
  if (!session.rows[0]) return res.status(404).json({ error: "Session not found" });

  const pid = session.rows[0].proxy_pid;
  if (pid) {
    try {
        process.kill(-pid, "SIGINT");
    } catch (err) {
      console.error("Failed to kill proxy process:", err.message);
      // ESRCH means it's already dead — not fatal, just log and continue
    }
  }
  const result = await pool.query(
    "UPDATE sessions SET ended_at = now() WHERE id = $1 RETURNING *",
    [req.params.id]
  );
  if (!result.rows[0]) return res.status(404).json({ error: "Session not found" });
  res.json(result.rows[0]);
});

// GET /api/sessions - list past sessions
router.get("/", async (req, res) => {
  const result = await pool.query(
    `SELECT s.*, COUNT(r.id) AS request_count
     FROM sessions s
     LEFT JOIN requests r ON r.session_id = s.id
     WHERE s.user_id = $1
     GROUP BY s.id
     ORDER BY s.started_at DESC`,
    [req.user.id]
  );
  res.json(result.rows);
});

// GET /api/sessions/:id - session detail + summary stats
router.get("/:id", async (req, res) => {
  const session = await pool.query("SELECT * FROM sessions WHERE id = $1", [req.params.id]);
  if (!session.rows[0]) return res.status(404).json({ error: "Session not found" });

  const stats = await pool.query(
    `SELECT
       COUNT(*) AS total_requests,
       COUNT(*) FILTER (WHERE flagged) AS flagged_count,
       COUNT(DISTINCT host) AS unique_hosts
     FROM requests WHERE session_id = $1`,
    [req.params.id]
  );

  res.json({ ...session.rows[0], stats: stats.rows[0] });
});

// GET /api/sessions/:id/stats - aggregated breakdowns
router.get("/:id/stats", async (req, res) => {
  const topHosts = await pool.query(
    `SELECT host, COUNT(*) AS count FROM requests
     WHERE session_id = $1 GROUP BY host ORDER BY count DESC LIMIT 10`,
    [req.params.id]
  );
  const methodBreakdown = await pool.query(
    `SELECT method, COUNT(*) AS count FROM requests
     WHERE session_id = $1 GROUP BY method ORDER BY count DESC`,
    [req.params.id]
  );
  const overTime = await pool.query(
    `SELECT date_trunc('minute', timestamp) AS minute, COUNT(*) AS count
     FROM requests WHERE session_id = $1 GROUP BY minute ORDER BY minute`,
    [req.params.id]
  );

  res.json({
    topHosts: topHosts.rows,
    methodBreakdown: methodBreakdown.rows,
    overTime: overTime.rows,
  });
});

module.exports = router;
