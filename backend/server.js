require("dotenv").config();
const express = require("express");
const cors = require("cors");
const http = require("http");
const { Server } = require("socket.io");
const jwt = require("jsonwebtoken");

const authRoutes = require("./routes/auth");
const sessionRoutes = require("./routes/sessions");
const requestRoutes = require("./routes/requests");
const { startIngestWorker } = require("./ingest/tail");

const app = express();
app.use(cors());
app.use(express.json());

app.use("/api/auth", authRoutes);
app.use("/api/sessions", sessionRoutes);
app.use("/api", requestRoutes); // exposes /api/sessions/:id/requests and /api/requests/:id

app.get("/health", (req, res) => res.json({ ok: true }));

const server = http.createServer(app);
const io = new Server(server, {
  cors: { origin: "*" },
});

// Socket auth: client connects with { auth: { token } }
io.use((socket, next) => {
  const token = socket.handshake.auth?.token;
  if (!token) return next(new Error("Missing token"));
  try {
    socket.user = jwt.verify(token, process.env.JWT_SECRET);
    next();
  } catch (err) {
    next(new Error("Invalid token"));
  }
});

io.on("connection", (socket) => {
  socket.on("watch_session", (sessionId) => {
    socket.join(`session:${sessionId}`);
  });
  socket.on("unwatch_session", (sessionId) => {
    socket.leave(`session:${sessionId}`);
  });
});

// Start tailing the proxy's log file and pushing new events to clients
startIngestWorker(process.env.PROXY_LOG_PATH || "./ingest/proxy_events.log", io);

const PORT = process.env.PORT || 4000;
server.listen(PORT, () => console.log(`Server running on :${PORT}`));
