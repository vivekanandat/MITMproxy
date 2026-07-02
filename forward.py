import socket
import os
import threading
import subprocess
import tempfile

SOCK_PATH = "/tmp/mitm_editor.sock"

pending = {}     
pending_lock = threading.Lock()
next_id = 1

def recv_exact(conn, n):
    data = b""
    while len(data) < n:
        chunk = conn.recv(n - len(data))
        if not chunk:
            return None
        data += chunk
    return data

def preview(data: bytes) -> str:
    try:
        text = data.decode(errors="replace")
        lines = text.split("\r\n")
        line = lines[1] if len(lines) > 1 else lines[0]
        return (line[:100] + "...") if len(line) > 100 else line
    except Exception:
        return "<unable to read>"

def handle_conn(conn):
    global next_id
    len_bytes = recv_exact(conn, 4)
    if not len_bytes:
        conn.close()
        return
    length = int.from_bytes(len_bytes, "big")
    packet = recv_exact(conn, length)
    if packet is None:
        conn.close()
        return

    event = threading.Event()
    with pending_lock:
        pid = next_id
        next_id += 1
        pending[pid] = {"conn": conn, "data": packet, "event": event}

    event.wait() 

    with pending_lock:
        entry = pending.pop(pid, None)
    if entry is None:
        return
    out = entry["data"]
    try:
        conn.sendall(len(out).to_bytes(4, "big"))
        conn.sendall(out)
    except Exception:
        pass
    conn.close()

def server_loop():
    if os.path.exists(SOCK_PATH):
        os.remove(SOCK_PATH)
    server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server.bind(SOCK_PATH)
    server.listen(50)
    while True:
        conn, _ = server.accept()
        threading.Thread(target=handle_conn, args=(conn,), daemon=True).start()

def list_packets():
    with pending_lock:
        for pid, entry in pending.items():
            print(f"{pid}: {preview(entry['data'])}")

def edit_packet(pid):
    with pending_lock:
        entry = pending.get(pid)
        if not entry:
            print("no such id")
            return
        data = entry["data"]

    with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
        f.write(data)
        path = f.name

    subprocess.call(["vim", path])

    with open(path, "rb") as f:
        edited = f.read()
    os.remove(path)

    with pending_lock:
        if pid in pending:
            pending[pid]["data"] = edited
    print(f"updated {pid}")

def forward(pid):
    with pending_lock:
        entry = pending.get(pid)
        if not entry:
            print("no such id")
            return
        entry["event"].set()
    print(f"forwarded {pid}")

def forward_all():
    with pending_lock:
        ids = list(pending.keys())
    for pid in ids:
        forward(pid)

def repl():
    while True:
        cmd = input("> ").strip()
        if cmd == "l":
            list_packets()
        elif cmd.startswith("e "):
            edit_packet(int(cmd.split()[1]))
        elif cmd.startswith("f "):
            forward(int(cmd.split()[1]))
        elif cmd == "fa":
            forward_all()
        elif cmd == "a":
            print("auto-forwarding new packets until Ctrl+C")
            try:
                while True:
                    forward_all()
            except KeyboardInterrupt:
                print("\nstopped auto forwarding")
        elif cmd in ("quit", "e" if False else "quit"):
            break
        elif cmd == "q":
            break
        else:
            print("Commands: l | e <id> | f <id> | fa | a | q")

if __name__ == "__main__":
    threading.Thread(target=server_loop, daemon=True).start()
    repl()
