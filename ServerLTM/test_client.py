import socket
import json

HOST = "172.31.245.233"
PORT = 8080

def send_request(req):
    """Gửi request JSON và nhận response"""
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((HOST, PORT))
    msg = json.dumps(req) + "\r\n"
    s.sendall(msg.encode())
    
    # Nhận toàn bộ response (có thể > 8KB)
    chunks = []
    while True:
        chunk = s.recv(8192)
        if not chunk:
            break
        chunks.append(chunk)
        # Nếu chunk < 8192, có thể đã hết data
        if len(chunk) < 8192:
            break
    
    resp = b''.join(chunks).decode()
    s.close()
    try:
        return json.loads(resp)
    except:
        return resp

def register(username, password):
    req = {
        "action": "register",
        "data": {"username": username, "password": password}
    }
    return send_request(req)

def login(username, password):
    req = {
        "action": "login",
        "data": {"username": username, "password": password}
    }
    return send_request(req)

def list_projects(session):
    req = {"action": "list_projects", "session": session, "data": {}}
    return send_request(req)

def search_project(session, keyword):
    req = {"action": "search_project", "session": session, "data": {"keyword": keyword}}
    return send_request(req)

def create_project(session, name, desc):
    req = {"action": "create_project", "session": session, "data": {"project_name": name, "description": desc}}
    return send_request(req)

def add_member(session, project_id, username, role):
    req = {"action": "add_member", "session": session, "data": {"project_id": project_id, "username": username, "role": role}}
    return send_request(req)

def list_tasks(session, project_id):
    req = {"action": "list_tasks", "session": session, "data": {"project_id": project_id}}
    return send_request(req)

def create_task(session, project_id, task_name, desc):
    req = {"action": "create_task", "session": session, "data": {"project_id": project_id, "task_name": task_name, "description": desc}}
    return send_request(req)

def assign_task(session, task_id, user_id):
    req = {"action": "assign_task", "session": session, "data": {"task_id": task_id, "assigned_to": user_id}}
    return send_request(req)

def update_task(session, task_id, status):
    req = {"action": "update_task", "session": session, "data": {"task_id": task_id, "status": status}}
    return send_request(req)

def comment_task(session, task_id, comment):
    req = {"action": "comment_task", "session": session, "data": {"task_id": task_id, "comment": comment}}
    return send_request(req)

def get_task_detail(session, task_id):
    req = {"action": "get_task_detail", "session": session, "data": {"task_id": task_id}}
    return send_request(req)

def update_member(session, project_id, user_id, role):
    req = {"action": "update_member", "session": session, "data": {"project_id": project_id, "user_id": user_id, "role": role}}
    return send_request(req)

def list_members(session, project_id):
    req = {"action": "list_members", "session": session, "data": {"project_id": project_id}}
    return send_request(req)

if __name__ == "__main__":
    print("=== LOGIN ===")
    resp = login("teonhe2", "123456")
    print(resp)
    session = resp.get("data", {}).get("session", "")

    if not session:
        print("Login failed, cannot continue tests")
        exit(1)

    print("=== LIST PROJECTS ===")
    projects_resp = list_projects(session)
    
    # Kiểm tra số lượng projects
    if projects_resp.get("status") == "103":
        projects = projects_resp.get("data", [])
        print(f"Tổng số projects: {len(projects)}")
        
        # Đếm Thread_Project
        thread_projects = [p for p in projects if "Thread_Project_" in p.get("project_name", "")]
        print(f"Số Thread_Project_: {len(thread_projects)}")
        
        # In 5 projects đầu và 5 cuối
        print("\n5 projects đầu tiên:")
        for p in projects[:5]:
            print(f"  [{p['project_id']}] {p['project_name']}")
        
        if len(projects) > 10:
            print(f"\n... ({len(projects) - 10} projects ở giữa) ...\n")
            print("5 projects cuối cùng:")
            for p in projects[-5:]:
                print(f"  [{p['project_id']}] {p['project_name']}")
    else:
        print(projects_resp)

    print("=== CREATE PROJECT ===")
    create_resp = create_project(session, "TestProject2", "Project description")
    print(create_resp)

    project_id = 3
    if create_resp.get("status") == "105":
        project_id = create_resp.get("data", {}).get("project_id")

    if project_id:
        print("=== ADD MEMBER ===")
        print(add_member(session, project_id, "teonhe1", "MEMBER"))

        print("=== LIST MEMBERS ===")
        print(list_members(session, project_id))

        print("=== LIST TASKS ===")
        print(list_tasks(session, project_id))

        print("=== CREATE TASK ===")
        create_task_resp = create_task(session, project_id, "TestTask", "Task description")
        print(create_task_resp)

        task_id = create_task_resp.get("data", {}).get("task_id")

        if task_id:
            print("=== ASSIGN TASK ===")
            print(assign_task(session, task_id, 2))

            print("=== UPDATE TASK (PM/assignee) ===")
            print(update_task(session, task_id, "in_progress"))

            print("=== COMMENT TASK ===")
            print(comment_task(session, task_id, "This is a comment."))

        print("=== UPDATE TASK STATUS (final) ===")
        print(update_task(session, task_id, "done"))

        print("=== GET TASK DETAIL (with comments) ===")
        task_detail = get_task_detail(session, task_id)
        if task_detail.get("status") == "112":
            data = task_detail.get("data", {})
            print(f"Task ID: {data.get('task_id')}")
            print(f"Task Name: {data.get('task_name')}")
            print(f"Status: {data.get('status')}")
            print(f"Assigned To: {data.get('assigned_user')} (user_id={data.get('assigned_to')})")
            comments = data.get('comments', [])
            print(f"Comments ({len(comments)}):")
            for cmt in comments:
                print(f"  - [{cmt.get('username')}] {cmt.get('comment')} (at {cmt.get('created_at')})")
        else:
            print(task_detail)
