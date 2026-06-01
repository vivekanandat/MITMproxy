import os
import signal
import time
import threading

def delete_later(path):
    time.sleep(1)
    try:
        os.remove(path)
    except FileNotFoundError:
        pass

def get_packets():
    packets = {}

    files = sorted(
        f for f in os.listdir("packets")
        if f.isdigit()
    )

    for i, pid in enumerate(files, start=1):
        path = os.path.join("packets", pid)

        try:
            with open(path, "r", errors="replace") as f:
                f.readline()  # skip line 1
                second_line = f.readline().strip()

                if len(second_line) > 100:
                    second_line = second_line[:100] + "..."

        except Exception:
            second_line = "<unable to read>"

        packets[i] = [pid, second_line]

    return packets


def forward_pid(pid_str):
    pid = int(pid_str)
    path = os.path.join("packets", pid_str)

    try:
        os.kill(pid, signal.SIGCONT)
        print(f"Resumed {pid}")
    except ProcessLookupError:
        print(f"PID {pid} does not exist")

    threading.Thread(
        target=delete_later,
        args=(path,),
        daemon=True
    ).start()

while True:
    cmd = input("> ").strip()

    if cmd == "l":
        packets = get_packets()

        for k, (pid, first_line) in packets.items():
            print(f"{k}: [{pid}] {first_line}")
    elif cmd.startswith("f "):
        key = int(cmd.split()[1])

        packets = get_packets()

        if key in packets:
            pid = packets[key][0]
            forward_pid(pid)
    
    elif cmd == "fa":
        packets = get_packets()

        for pid, _ in packets.values():
            forward_pid(pid)

    elif cmd == "a":
        print("Forwarding all packets until Ctrl+C")

        try:
            while True:
                packets = get_packets()

                for pid, _ in packets.values():
                    forward_pid(pid)


        except KeyboardInterrupt:
            print("\nStopped auto forwarding")

    elif cmd in ("quit", "e"):
        break

    else:
        print("Commands:")
        print("  l")
        print("  f <key>")
        print("  fa")
        print("  a")
        print("  e")
