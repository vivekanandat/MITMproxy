# MITM Proxy Dashboard

A full-stack observability layer built on top of a custom C++ TLS-intercepting MITM proxy —
turns raw intercepted traffic into a live, queryable dashboard with auth, session management,
and real-time visualization.

## Why this exists

A custom C++ MITM proxy that intercepts, inspects, and can modify live HTTPS traffic (via TLS interception with dynamically generated certs) — extended with a full observability and control layer: 
persistence, a REST API, real-time streaming, and a web dashboard. The proxy itself is the core engineering work; this project makes it usable and observable as a real tool instead of a console script.

## Architecture

┌─────────┐      writes JSON lines       ┌──────────────────┐  
│  C++ MITM Proxy    │ ──────────────────────────▶  │proxy_events.log  │  
│  (fork per conn,   │                              └──────────────────┘  
│  OpenSSL, cert-gen)                                       │  
└─────────┘                              tailed by fs.watch  
│  
▼  
┌────────────────────┐  
│  Node/Express API  │  
│  + Socket.io       │  
│  + Postgres        │  
└────────────────────┘  
│  
REST + WebSocket  
│  
▼  
┌──────────────────────┐  
│  React Dashboard     │  
│  (Tailwind, Recharts)│  
└──────────────────────┘  
## Stack

- **Proxy:** C++, OpenSSL, raw sockets, fork-per-connection
- **Backend:** Node.js, Express, Socket.io, PostgreSQL, JWT auth
- **Frontend:** React (Vite), Tailwind CSS, Recharts, socket.io-client

## Features

- JWT-based login (register/login)
- Start/stop capture sessions from the UI — spawns and kills the C++ proxy as a managed child process
- Live traffic view: real-time table + requests/sec chart via WebSocket
- Historical session view: top hosts, method breakdown, paginated request log
- Filterable by host, method, and flagged status
- Per-request detail (headers, body preview, TLS flag)

## Project structure

mitm-dashboard/
proxy/            C++ MITM proxy (source + compiled binary + CA certs)
backend/           Express API, Socket.io server, Postgres schema/migrations
frontend/          React dashboard (Vite + Tailwind + Recharts)

## Setup

### 1. Database

```bash
sudo -u postgres psql -c "CREATE USER <youruser> WITH PASSWORD '<yourpass>';"
sudo -u postgres psql -c "CREATE DATABASE mitm_dashboard OWNER <youruser>;"
```

> If you hit a `collation version mismatch` error, run:
> ```bash
> sudo -u postgres psql -c "ALTER DATABASE postgres REFRESH COLLATION VERSION;"
> sudo -u postgres psql -c "ALTER DATABASE template1 REFRESH COLLATION VERSION;"
> ```

### 2. Backend

```bash
cd backend
cp .env.example .env   # set DATABASE_URL, JWT_SECRET, PROXY_LOG_PATH
npm install
npm run migrate
npm run dev             # http://localhost:4000
```

Create your first user:
```bash
curl -X POST http://localhost:4000/api/auth/register \
  -H "Content-Type: application/json" \
  -d '{"email":"you@example.com","password":"yourpassword"}'
```

### 3. Frontend

```bash
cd frontend
cp .env.example .env   # set VITE_API_BASE
npm install
npm run dev             # http://localhost:5173
```

### 4. Proxy

The proxy binary lives in `proxy/`. The backend spawns it automatically when you click
"Start New Capture," passing `<port> <session_id>` as arguments — no manual step needed
once paths are configured correctly in `backend/routes/sessions.js` (`spawn` path and `cwd`
must both point at the `proxy/` folder).

To run it manually for testing:
```bash
cd proxy
./sp 8080 <session-id>
```

## How a capture works end-to-end

1. Log in on the dashboard, click **Start New Capture**
2. Backend inserts a `sessions` row, spawns the proxy with that session's UUID, stores its PID
3. Proxy intercepts traffic, appends one JSON line per completed request to `proxy_events.log`
4. Backend's ingest worker tails that file, inserts each event into `requests`, and pushes it
   over WebSocket to any client watching that session
5. Dashboard's Live Capture view updates in real time
6. Click **End Session** → backend sends `SIGTERM` to the proxy's process group, marks the
   session ended

## Known gotchas (learned the hard way)

- **Proxy `cwd` matters.** The proxy loads certs from relative paths (`CA/...`). When spawned
  from Node, `cwd` must explicitly be set to the proxy's own directory, or cert loading fails
  with `ENOENT`.
- **`SIGCHLD` set to `SIG_IGN` breaks `system()`.** If the proxy globally ignores `SIGCHLD` to
  avoid zombies, `system()` calls (used for cert generation) fail with `ECHILD` because they
  can't `wait()` on their own child. Reap children explicitly with a `waitpid(WNOHANG)` handler
  instead of blanket-ignoring the signal.
- **Log path should be absolute**, or explicitly relative to wherever the proxy's `cwd` actually
  ends up — relative paths resolved differently depending on whether the proxy was launched
  manually vs. spawned by Node caused silent log-write failures.
- **Killing the proxy needs the whole process group**, not just the spawned PID — the proxy
  forks a child per connection, so `spawn(..., { detached: true })` + `process.kill(-pid, "SIGTERM")`
  (negative PID) is required to actually stop everything.

## Roadmap

- [ ] Real flagging logic (currently `flagged`/`flagged_reason` are always false/empty)
- [ ] Request detail modal in the frontend (backend endpoint already supports it)
- [ ] Pagination controls on the historical session view
- [ ] Deploy backend + Postgres (Render) and frontend (Vercel); proxy stays local (needs raw
      network access)
