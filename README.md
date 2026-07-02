# MITM Proxy Dashboard

A full-stack observability layer built on top of a custom C++ TLS-intercepting MITM proxy —
turns raw intercepted traffic into a live, queryable dashboard with auth, session management,
and real-time visualization.

## Why this exists

A custom C++ MITM proxy that intercepts, inspects, and can modify live HTTPS traffic (via TLS interception with dynamically generated certs) — extended with a full observability and control layer: 
persistence, a REST API, real-time streaming, and a web dashboard. The proxy itself is the core engineering work; this project makes it usable and observable as a real tool instead of a console script.

## Architecture
<img width="300" height="200" alt="mitm_dashboard_architecture" src="https://github.com/user-attachments/assets/04106cf7-bd12-4286-8139-43def945699d" />
<svg width="100%" viewBox="0 0 680 454" role="img" style="" xmlns="http://www.w3.org/2000/svg">
<title style="fill:rgb(0, 0, 0);stroke:none;color:rgb(11, 11, 11);stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:16px;font-weight:400;text-anchor:start;dominant-baseline:auto">MITM proxy dashboard architecture</title>
<desc style="fill:rgb(0, 0, 0);stroke:none;color:rgb(11, 11, 11);stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:16px;font-weight:400;text-anchor:start;dominant-baseline:auto">The C++ MITM proxy writes JSON lines to a log file, which is tailed and inserted into Postgres by a Node/Express + Socket.io backend, which serves a React dashboard over REST and WebSocket.</desc>
<defs>
<marker id="arrow" viewBox="0 0 10 10" refX="8" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse"><path d="M2 1L8 5L2 9" fill="none" stroke="context-stroke" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/></marker>
</defs>

<g style="fill:rgb(0, 0, 0);stroke:none;color:rgb(11, 11, 11);stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:16px;font-weight:400;text-anchor:start;dominant-baseline:auto">
<rect x="190" y="20" width="300" height="56" rx="8" stroke-width="0.5" style="fill:rgb(250, 236, 231);stroke:rgb(153, 60, 29);color:rgb(11, 11, 11);stroke-width:0.5px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:16px;font-weight:400;text-anchor:start;dominant-baseline:auto"/>
<text x="340" y="40" text-anchor="middle" dominant-baseline="central" style="fill:rgb(113, 43, 19);stroke:none;color:rgb(11, 11, 11);stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:14px;font-weight:500;text-anchor:middle;dominant-baseline:central">C++ MITM proxy</text>
<text x="340" y="58" text-anchor="middle" dominant-baseline="central" style="fill:rgb(153, 60, 29);stroke:none;color:rgb(11, 11, 11);stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:12px;font-weight:400;text-anchor:middle;dominant-baseline:central">fork per connection, OpenSSL, cert-gen</text>
</g>

<line x1="340" y1="76" x2="340" y2="126" marker-end="url(#arrow)" style="fill:none;stroke:rgb(137, 135, 129);color:rgb(11, 11, 11);stroke-width:1.5px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:16px;font-weight:400;text-anchor:start;dominant-baseline:auto"/>
<text x="350" y="101" dominant-baseline="central" style="fill:rgb(82, 81, 78);stroke:none;color:rgb(11, 11, 11);stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:12px;font-weight:400;text-anchor:start;dominant-baseline:central">writes JSON lines</text>

<g style="fill:rgb(0, 0, 0);stroke:none;color:rgb(11, 11, 11);stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:16px;font-weight:400;text-anchor:start;dominant-baseline:auto">
<rect x="190" y="126" width="300" height="56" rx="8" stroke-width="0.5" style="fill:rgb(241, 239, 232);stroke:rgb(95, 94, 90);color:rgb(11, 11, 11);stroke-width:0.5px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:16px;font-weight:400;text-anchor:start;dominant-baseline:auto"/>
<text x="340" y="146" text-anchor="middle" dominant-baseline="central" style="fill:rgb(68, 68, 65);stroke:none;color:rgb(11, 11, 11);stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:14px;font-weight:500;text-anchor:middle;dominant-baseline:central">proxy_events.log</text>
<text x="340" y="164" text-anchor="middle" dominant-baseline="central" style="fill:rgb(95, 94, 90);stroke:none;color:rgb(11, 11, 11);stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:12px;font-weight:400;text-anchor:middle;dominant-baseline:central">newline-delimited JSON</text>
</g>

<line x1="340" y1="182" x2="340" y2="232" marker-end="url(#arrow)" style="fill:none;stroke:rgb(137, 135, 129);color:rgb(11, 11, 11);stroke-width:1.5px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:16px;font-weight:400;text-anchor:start;dominant-baseline:auto"/>
<text x="350" y="207" dominant-baseline="central" style="fill:rgb(82, 81, 78);stroke:none;color:rgb(11, 11, 11);stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:12px;font-weight:400;text-anchor:start;dominant-baseline:central">tailed by fs.watch</text>

<g style="fill:rgb(0, 0, 0);stroke:none;color:rgb(11, 11, 11);stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:16px;font-weight:400;text-anchor:start;dominant-baseline:auto">
<rect x="190" y="232" width="300" height="56" rx="8" stroke-width="0.5" style="fill:rgb(225, 245, 238);stroke:rgb(15, 110, 86);color:rgb(11, 11, 11);stroke-width:0.5px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:16px;font-weight:400;text-anchor:start;dominant-baseline:auto"/>
<text x="340" y="252" text-anchor="middle" dominant-baseline="central" style="fill:rgb(8, 80, 65);stroke:none;color:rgb(11, 11, 11);stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:14px;font-weight:500;text-anchor:middle;dominant-baseline:central">Node / Express API</text>
<text x="340" y="270" text-anchor="middle" dominant-baseline="central" style="fill:rgb(15, 110, 86);stroke:none;color:rgb(11, 11, 11);stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:12px;font-weight:400;text-anchor:middle;dominant-baseline:central">Socket.io + PostgreSQL</text>
</g>

<line x1="340" y1="288" x2="340" y2="338" marker-end="url(#arrow)" style="fill:none;stroke:rgb(137, 135, 129);color:rgb(11, 11, 11);stroke-width:1.5px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:16px;font-weight:400;text-anchor:start;dominant-baseline:auto"/>
<text x="350" y="313" dominant-baseline="central" style="fill:rgb(82, 81, 78);stroke:none;color:rgb(11, 11, 11);stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:12px;font-weight:400;text-anchor:start;dominant-baseline:central">REST + WebSocket</text>

<g style="fill:rgb(0, 0, 0);stroke:none;color:rgb(11, 11, 11);stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:16px;font-weight:400;text-anchor:start;dominant-baseline:auto">
<rect x="190" y="338" width="300" height="56" rx="8" stroke-width="0.5" style="fill:rgb(238, 237, 254);stroke:rgb(83, 74, 183);color:rgb(11, 11, 11);stroke-width:0.5px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:16px;font-weight:400;text-anchor:start;dominant-baseline:auto"/>
<text x="340" y="358" text-anchor="middle" dominant-baseline="central" style="fill:rgb(60, 52, 137);stroke:none;color:rgb(11, 11, 11);stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:14px;font-weight:500;text-anchor:middle;dominant-baseline:central">React dashboard</text>
<text x="340" y="376" text-anchor="middle" dominant-baseline="central" style="fill:rgb(83, 74, 183);stroke:none;color:rgb(11, 11, 11);stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;opacity:1;font-family:&quot;Anthropic Sans&quot;, -apple-system, BlinkMacSystemFont, &quot;Segoe UI&quot;, sans-serif;font-size:12px;font-weight:400;text-anchor:middle;dominant-baseline:central">Tailwind, Recharts</text>
</g>
</svg>


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
