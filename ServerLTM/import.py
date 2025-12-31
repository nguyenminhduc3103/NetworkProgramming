import socket
import json
import threading
import time
import uuid
from concurrent.futures import ThreadPoolExecutor

HOST = "172.31.245.233"
PORT = 8080

# --- Giữ nguyên các hàm send_request, login, create_project của bạn ---
def send_request(req):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(20) # Thêm timeout để tránh treo luồng nếu server lag
        s.connect((HOST, PORT))
        msg = json.dumps(req) + "\r\n"
        s.sendall(msg.encode())
        resp = s.recv(8192).decode()
        s.close()
        return json.loads(resp)
    except Exception as e:
        return {"status": "error", "message": str(e)}

def login(username, password):
    req = {"action": "login", "data": {"username": username, "password": password}}
    return send_request(req)

def create_project(session, name, desc):
    req = {"action": "create_project", "session": session, "data": {"project_name": name, "description": desc}}
    return send_request(req)

# --- Hàm mô phỏng hành vi của 1 User ---
def simulate_user_workflow(user_idx):
    """
    Mỗi luồng sẽ thực hiện: Login -> Create Project
    Mỗi thread có session riêng
    """
    username = "teonhe2"  # Có thể dùng chung user hoặc tạo riêng
    password = "123456"
    
    # Delay nhỏ để tránh 100 threads login cùng lúc xóa session của nhau
    time.sleep(user_idx * 0.05)  # Mỗi thread delay 50ms
    
    start_time = time.time()
    
    # 1. Login - mỗi thread tự login để có session riêng
    resp = login(username, password)
    session = resp.get("data", {}).get("session")
    
    if session:
        # 2. Tạo project với tên riêng biệt để tránh trùng lặp (dùng UUID)
        unique_id = str(uuid.uuid4())[:8]
        project_name = f"Thread_Project_{user_idx}_{unique_id}"
        result = create_project(session, project_name, "Multi-thread testing")
        
        duration = time.time() - start_time
        status = result.get("status", "")
        message = result.get("message", "")
        
        # Check if project was created successfully (status code 103)
        if status == "103" or "created successfully" in message.lower():
            print(f"[Thread {user_idx}] ✓ Created: {project_name} - Time: {duration:.2f}s")
            return True
        else:
            print(f"[Thread {user_idx}] ✗ Failed: {message} (status: {status}) - Time: {duration:.2f}s")
            return False
    else:
        print(f"[Thread {user_idx}] Login Failed!")
        return False

# --- Hàm thực thi Test ---
def run_multithreaded_test(num_users):
    print(f"--- Bắt đầu test với {num_users} luồng đồng thời ---")
    start_test = time.time()
    
    with ThreadPoolExecutor(max_workers=num_users) as executor:
        # Gửi tất cả các "user" vào hàng đợi thực thi
        results = list(executor.map(simulate_user_workflow, range(num_users)))
    
    end_test = time.time()
    success_count = results.count(True)
    
    print("--- KẾT QUẢ TEST ---")
    print(f"Tổng số request thành công: {success_count}/{num_users}")
    print(f"Tổng thời gian thực hiện: {end_test - start_test:.2f} giây")

if __name__ == "__main__":
    # Bạn có thể điều chỉnh số lượng luồng ở đây
    # Thử bắt đầu với 10, sau đó tăng lên 50, 100 để xem giới hạn của Server
    run_multithreaded_test(num_users=100)