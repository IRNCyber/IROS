import argparse
import os
import socket
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent


def find_app_repo(name: str) -> Path:
    repo = ROOT / "apps-src" / name
    return repo


def send(conn: socket.socket, data: str) -> None:
    conn.sendall(data.encode("utf-8", errors="replace"))


def send_eot(conn: socket.socket) -> None:
    conn.sendall(b"\x04")


def run_python(repo_dir: Path, entry: str, conn: socket.socket) -> int:
    entry = entry.strip()
    if not entry:
        # Guess common entrypoints
        for cand in ["main.py", "app.py", "run.py", "__main__.py"]:
            if (repo_dir / cand).exists():
                entry = cand
                break
    if not entry:
        send(conn, "No entrypoint specified/found.\n")
        return 1

    entry_path = repo_dir / entry
    if not entry_path.exists():
        send(conn, f"Entrypoint not found: {entry}\n")
        return 1

    # Optional deps
    req = repo_dir / "requirements.txt"
    if req.exists():
        send(conn, "Installing requirements.txt...\n")
        subprocess.run([sys.executable, "-m", "pip", "install", "-r", str(req)], check=False)

    send(conn, f"Running: python {entry}\n")
    proc = subprocess.Popen(
        [sys.executable, str(entry_path)],
        cwd=str(repo_dir),
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    assert proc.stdout is not None
    for line in proc.stdout:
        send(conn, line)
    return proc.wait()


def run_exec(code: str, conn: socket.socket) -> int:
    proc = subprocess.Popen(
        [sys.executable, "-c", code],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    assert proc.stdout is not None
    for line in proc.stdout:
        send(conn, line)
    return proc.wait()


def handle_line(line: str, conn: socket.socket) -> None:
    line = line.strip("\r\n")
    if not line:
        send_eot(conn)
        return

    if line == "PING":
        send(conn, "PONG\n")
        send_eot(conn)
        return

    if line.startswith("RUN "):
        _, rest = line.split(" ", 1)
        parts = rest.split(" ", 1)
        name = parts[0].strip()
        entry = parts[1] if len(parts) > 1 else ""
        repo = find_app_repo(name)
        if not repo.exists():
            send(conn, f"Repo not installed: {repo}\n")
            send(conn, "Install it with: tools\\iros-install.ps1 <git-url>\n")
            send_eot(conn)
            return
        rc = run_python(repo, entry, conn)
        send(conn, f"\n[exit {rc}]\n")
        send_eot(conn)
        return

    if line.startswith("EXEC "):
        code = line[5:]
        rc = run_exec(code, conn)
        send(conn, f"\n[exit {rc}]\n")
        send_eot(conn)
        return

    send(conn, "Unknown command\n")
    send_eot(conn)


def main() -> int:
    parser = argparse.ArgumentParser(description="IROS Python bridge (serial over TCP from QEMU).")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=4444)
    parser.add_argument(
        "--wait-seconds",
        type=int,
        default=30,
        help="How long to wait for QEMU to start listening (0 disables waiting).",
    )
    args = parser.parse_args()

    # QEMU runs the serial as a TCP server; we connect as a client.
    last_exc: Exception | None = None
    if args.wait_seconds and args.wait_seconds > 0:
        for _ in range(args.wait_seconds):
            try:
                s = socket.create_connection((args.host, args.port), timeout=1)
                break
            except Exception as e:  # noqa: BLE001
                last_exc = e
        else:
            print(
                f"Failed to connect to QEMU serial bridge at {args.host}:{args.port} after {args.wait_seconds}s.",
                file=sys.stderr,
            )
            if last_exc:
                print(f"Last error: {last_exc}", file=sys.stderr)
            print(
                "Start IROS with the bridge enabled, then retry:\n"
                "  .\\run.ps1 -PyBridge\n"
                "  python tools\\iros-pybridge.py",
                file=sys.stderr,
            )
            return 2
    else:
        try:
            s = socket.create_connection((args.host, args.port), timeout=10)
        except Exception as e:  # noqa: BLE001
            print(f"Failed to connect to {args.host}:{args.port}: {e}", file=sys.stderr)
            print("Start QEMU with: .\\run.ps1 -PyBridge", file=sys.stderr)
            return 2

    s.settimeout(None)

    buf = b""
    while True:
        b = s.recv(1)
        if not b:
            break
        if b == b"\n":
            try:
                line = buf.decode("utf-8", errors="replace")
            finally:
                buf = b""
            handle_line(line, s)
        else:
            buf += b

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
