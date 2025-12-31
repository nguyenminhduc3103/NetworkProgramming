import socket
import json

HOST = "127.0.0.1"
PORT = 8080

def send_request(sock, obj):
    msg = json.dumps(obj) + "\r\n"   # IMPORTANT!
    sock.sendall(msg.encode())

    data = sock.recv(8192).decode()
    print("SERVER:", data)

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))

    # ========= Example: LOGIN =========
    login_payload = {
        "action": "login",
        "data": {
            "username": "test",
            "password": "123456"
        }
    }

    send_request(s, login_payload)

    # Suppose server returns session token → put it here
    session = "YOUR_SESSION_TOKEN"

    # ========= Example: LIST PROJECTS =========
    list_projects = {
        "action": "list_projects",
        "session": session,
        "data": {}
    }

    send_request(s, list_projects)
