# esp32_bridge_server.py
import socket
import threading
import sys

HOST = "0.0.0.0"
PORT = 5055

def recv_loop(conn):
    try:
        with conn.makefile("rb", buffering=0) as f:
            for raw in f:
                try:
                    line = raw.decode("utf-8", errors="replace").rstrip("\r\n")
                except Exception:
                    line = raw.decode("utf-8", "replace").strip()
                print(f"[esp32] {line}")
    except Exception as e:
        print(f"[server] recv_loop ended: {e}")

def send_loop(conn):
    try:
        for line in sys.stdin:
            msg = line.strip()
            if not msg:
                continue
            # Normalize spacing for commands like "I 200"
            wire = (msg + "\n").encode("utf-8")
            try:
                conn.sendall(wire)
                print(f"[you→esp32] {msg}")
            except (BrokenPipeError, ConnectionResetError):
                print("[server] connection closed while sending")
                break
    except Exception as e:
        print(f"[server] send_loop ended: {e}")

def main():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as srv:
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind((HOST, PORT))
        srv.listen(1)
        print(f"[server] listening on {HOST}:{PORT} ...")

        conn, addr = srv.accept()
        print(f"[server] client connected from {addr}")

        t_recv = threading.Thread(target=recv_loop, args=(conn,), daemon=True)
        t_send = threading.Thread(target=send_loop, args=(conn,), daemon=True)
        t_recv.start()
        t_send.start()

        try:
            t_recv.join()
        except KeyboardInterrupt:
            pass
        print("[server] shutting down")

if __name__ == "__main__":
    main()
