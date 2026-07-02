const express = require("express");
const pool = require("../db/pool");
const { verifyAccessToken } = require("../middleware/auth");

const router = express.Router();
router.use(verifyAccessToken);

// GET /api/sessions/:id/requests - paginated, filterable
router.get("/sessions/:id/requests", async (req, res) => {
  const { host, method, flagged, page = 1, limit = 50 } = req.query;
  const offset = (page - 1) * limit;

  const conditions = ["session_id = $1"];
  const params = [req.params.id];

  if (host) {
    params.push(`%${host}%`);
    conditions.push(`host ILIKE $${params.length}`);
  }
  if (method) {
    params.push(method.toUpperCase());
    conditions.push(`method = $${params.length}`);
  }
  if (flagged === "true") {
    conditions.push("flagged = true");
  }

  params.push(limit, offset);
  const query = `
    SELECT id, method, host, path, status_code, request_size, response_size,
           tls, flagged, timestamp
    FROM requests
    WHERE ${conditions.join(" AND ")}
    ORDER BY timestamp DESC
    LIMIT $${params.length - 1} OFFSET $${params.length}
  `;

  const result = await pool.query(query, params);
  res.json(result.rows);
});

// GET /api/requests/:id - single request detail
router.get("/requests/:id", async (req, res) => {
  const result = await pool.query("SELECT * FROM requests WHERE id = $1", [req.params.id]);
  if (!result.rows[0]) return res.status(404).json({ error: "Request not found" });
  res.json(result.rows[0]);
});

module.exports = router;
