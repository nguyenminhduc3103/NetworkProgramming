import socket
import json

HOST = "172.31.245.233"
PORT = 8080

def send_request(req):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((HOST, PORT))
    msg = json.dumps(req) + "\r\n"
    s.sendall(msg.encode())
    
    chunks = []
    while True:
        chunk = s.recv(8192)
        if not chunk:
            break
        chunks.append(chunk)
        if len(chunk) < 8192:
            break
    
    resp = b''.join(chunks).decode()
    s.close()
    return json.loads(resp)

def login(username, password):
    req = {"action": "login", "data": {"username": username, "password": password}}
    return send_request(req)

def list_members(session, project_id):
    req = {"action": "list_members", "session": session, "data": {"project_id": project_id}}
    return send_request(req)

# Test
print("[TEST] List members\n")

# Login
print("[1] Login...")
resp = login("teonhe2", "123456")
session = resp.get("data", {}).get("session")
print(f"    Session: {session[:16]}... OK\n")

# Test list_members for project 3
print("[2] List members for project 3...")
members_resp = list_members(session, 3)

print(f"    Status: {members_resp.get('status')}")
print(f"    Message: {members_resp.get('message')}")
print(f"    Data type: {type(members_resp.get('data'))}")
print(f"    Data length: {len(members_resp.get('data', []))}")

if isinstance(members_resp.get('data'), list):
    members = members_resp.get('data', [])
    print(f"\n    Members ({len(members)}):")
    for m in members:
        print(f"      - ID: {m.get('user_id')}, Name: {m.get('username')}, Role: {m.get('role')}")
else:
    print(f"\n    Full response:")
    print(json.dumps(members_resp, indent=2, ensure_ascii=False))
