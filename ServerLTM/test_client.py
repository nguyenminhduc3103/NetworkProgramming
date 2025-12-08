
import socket
import json

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("172.31.245.233", 8080))

req = {
    "action": "login",
    "session": "",
    "data": {
        "username": "teonhe",
        "password": "123456"
    }
}

msg = json.dumps(req) + "\r\n"
s.sendall(msg.encode())

resp = s.recv(4096).decode()
print("SERVER:", resp)
'''
import socket
import json

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("172.31.245.233", 8080))

req = {
    "action": "register",
    "session": "",
    "data": {
        "username": "teonhe",
        "password": "123456"
    }
}

msg = json.dumps(req) + "\r\n"
s.sendall(msg.encode())

resp = s.recv(4096).decode()
print("SERVER:", resp)

'''