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
    resp = s.recv(8192).decode()
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

def add_member(session, project_id, user_id, role):
    req = {"action": "add_member", "session": session, "data": {"project_id": project_id, "user_id": user_id, "role": role}}
    return send_request(req)

def list_tasks(session, project_id):
    req = {"action": "list_tasks", "session": session, "data": {"project_id": project_id}}
    return send_request(req)

def create_task(session, project_id, task_name, desc):
    req = {"action": "create_task", "session": session, "data": {"project_id": project_id, "task_name": task_name, "description": desc}}
    return send_request(req)

def assign_task(session, task_id, username):
    req = {"action": "assign_task", "session": session, "data": {"task_id": task_id, "username": username}}
    return send_request(req)

def update_task(session, task_id, status):
    req = {"action": "update_task", "session": session, "data": {"task_id": task_id, "status": status}}
    return send_request(req)

def comment_task(session, task_id, comment):
    req = {"action": "comment_task", "session": session, "data": {"task_id": task_id, "comment": comment}}
    return send_request(req)

def update_member(session, project_id, user_id, role):
    req = {"action": "update_member", "session": session, "data": {"project_id": project_id, "user_id": user_id, "role": role}}
    return send_request(req)


if __name__ == "__main__":
    print("=== REGISTER ===")
    print(register("teonhe", "123456"))

    print("=== LOGIN ===")
    resp = login("teonhe", "123456")
    print(resp)
    session = resp.get("data", {}).get("session", "")

    if not session:
        print("Login failed, cannot continue tests")
        exit(1)

    print("=== LIST PROJECTS ===")
    print(list_projects(session))

    print("=== CREATE PROJECT ===")
    create_resp = create_project(session, "TestProject", "Project description")
    print(create_resp)

    project_id = 1
    if create_resp.get("status") == "105":
        project_id = create_resp.get("data", {}).get("project_id")

    if project_id:
        print("=== ADD MEMBER ===")
        print(add_member(session, project_id, 1, "Member"))

        print("=== LIST TASKS ===")
        print(list_tasks(session, project_id))

        print("=== CREATE TASK ===")
        create_task_resp = create_task(session, project_id, "TestTask", "Task description")
        print(create_task_resp)

        task_id = create_task_resp.get("data", {}).get("task_id")

        if task_id:
            print("=== ASSIGN TASK ===")
            print(assign_task(session, task_id, "teonhe"))

            print("=== UPDATE TASK ===")
            print(update_task(session, task_id, "in_progress"))

            print("=== COMMENT TASK ===")
            print(comment_task(session, task_id, "This is a comment."))

        print("=== UPDATE MEMBER ===")
        print(update_member(session, project_id, 1, "PM"))
