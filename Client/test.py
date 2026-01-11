import socket
import time

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("172.31.245.233", 8080))

# Gửi JSON + \r
s.sendall(b'{"action":"login","data":{}}\r')

# Đóng phía ghi NGAY LẬP TỨC
s.shutdown(socket.SHUT_WR)

# Đợi để server cố đọc \n
time.sleep(0.1)

try:
    data = s.recv(4096)
    print("SERVER RESPONSE:", data)
except Exception as e:
    print("ERROR:", e)

s.close()
