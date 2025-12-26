import streamlit as st
import socket
import json

# ============================
# CONFIG SERVER
# ============================

SERVER_HOST = "127.0.0.1"
SERVER_PORT = 5000

# ============================
# HELPER FUNCTION
# ============================

def send_request(action, session="", data={}):
    """
    send request to server and receive response with CRLF delimiter
    """
    request = json.dumps({
        "action": action,
        "session": session,
        "data": data
    }, separators=(',', ':')) + "\r\n"
    
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((SERVER_HOST, SERVER_PORT))
            s.sendall(request.encode())
            
            buffer = ""
            while True:
                chunk = s.recv(4096).decode()
                if not chunk:
                    break
                buffer += chunk
                if "\r\n" in buffer:
                    resp_str, _ = buffer.split("\r\n", 1)
                    return json.loads(resp_str)
    except Exception as e:
        return {"status": "error", "message": str(e)}
    
    return {"status": "error", "message": "No response from server"}

# ============================
# SESSION STATE
# ============================

if "session" not in st.session_state:
    st.session_state.session = ""
if "username" not in st.session_state:
    st.session_state.username = ""
if "selected_project" not in st.session_state:
    st.session_state.selected_project = None
if "user_role" not in st.session_state:
    st.session_state.user_role = {}

# ============================
# STREAMLIT UI
# ============================

st.title("🟦 Client Project Manager")

# ----------------------------
# LOGIN FORM
# ----------------------------
if st.session_state.session == "":
    st.subheader("🔐 Đăng nhập / Đăng ký")
    
    col1, col2 = st.columns(2)
    with col1:
        username = st.text_input("Username")
    with col2:
        password = st.text_input("Password", type="password")
    
    col3, col4 = st.columns(2)
    with col3:
        if st.button("Login"):
            res = send_request("login", session="", data={"username": username, "password": password})
            if res.get("status") == "101":  # S_LOGIN_OK
                st.session_state.session = res["data"]["session"]
                st.session_state.username = username
                st.success("✅ Đăng nhập thành công!")
                st.rerun()
            else:
                st.error(f"❌ {res.get('message', 'Đăng nhập thất bại')}")
    with col4:
        if st.button("Register"):
            res = send_request("register", session="", data={"username": username, "password": password})
            if res.get("status") == "102":  # S_REG_OK
                st.success("✅ Đăng ký thành công! Hãy đăng nhập")
            else:
                st.error(f"❌ {res.get('message', 'Đăng ký thất bại')}")
    
    st.stop()

# ----------------------------
# HEADER & LOGOUT
# ----------------------------
col1, col2 = st.columns([3, 1])
with col1:
    st.write(f"👤 **User:** {st.session_state.username}")
with col2:
    if st.button("🚪 Logout"):
        st.session_state.session = ""
        st.session_state.username = ""
        st.session_state.selected_project = None
        st.session_state.user_role = {}
        st.rerun()

st.divider()

# ----------------------------
# TABS
# ----------------------------
tab1, tab2, tab3, tab4 = st.tabs(["📋 Dự án", "✅ Công việc", "👥 Thành viên", "💬 Nhận xét"])

# ============================
# TAB 1: PROJECT MANAGEMENT
# ============================
with tab1:
    st.header("📋 Quản lý dự án")
    
    # 1. PROJECT LIST
    st.subheader("Danh sách dự án của bạn")
    
    if st.button("🔄 Làm mới danh sách", key="refresh_projects"):
        res = send_request("list_projects", st.session_state.session)
        if res.get("status") == "code(ok)":
            projects = res["data"]["projects"]
            if projects:
                for p in projects:
                    with st.container():
                        col_a, col_b, col_c = st.columns([2,2,1])
                        with col_a: st.write(f"**{p['name']}**")
                        with col_b: st.write(f"ID: {p['id']}")
                        with col_c:
                            if st.button("Chọn", key=f"select_{p['id']}"):
                                st.session_state.selected_project = p
                                st.rerun()
            else:
                st.info("Bạn chưa tham gia dự án nào")
        else:
            st.error(res.get("message"))
    
    st.divider()
    
    # 2. PROJECT SEARCH
    st.subheader("🔍 Tìm kiếm dự án")
    col1, col2 = st.columns([3,1])
    with col1:
        search_kw = st.text_input("Nhập từ khóa", key="search_input")
    with col2:
        st.write("")
        st.write("")
        search_btn = st.button("Tìm kiếm", type="primary")
    if search_btn and search_kw:
        res = send_request("search_project", st.session_state.session, {"keyword": search_kw})
        if res.get("status") == "code(ok)":
            projects = res["data"]["projects"]
            if projects:
                st.success(f"Tìm thấy {len(projects)} dự án")
                for p in projects:
                    st.write(f"• **{p['name']}** (ID: {p['id']})")
            else:
                st.warning("Không tìm thấy dự án nào")
        else:
            st.error(res.get("message"))
    
    st.divider()
    
    # 3. CREATE PROJECT
    st.subheader("➕ Tạo dự án mới")
    with st.form("create_project_form"):
        new_project_name = st.text_input("Tên dự án")
        new_project_desc = st.text_area("Mô tả dự án")
        submit_project = st.form_submit_button("Tạo dự án", type="primary")
        if submit_project:
            if new_project_name:
                res = send_request("create_project", st.session_state.session, 
                                   {"name": new_project_name, "description": new_project_desc})
                if res.get("status") == "code(ok)":
                    st.success(f"✅ Tạo dự án '{new_project_name}' thành công!")
                else:
                    st.error(res.get("message", "Tạo dự án thất bại"))
            else:
                st.warning("Vui lòng nhập tên dự án")

# ============================
# TAB 2: TASKS MANAGEMENT
# ============================
with tab2:
    st.header("✅ Quản lý công việc")
    
    if st.session_state.selected_project:
        project = st.session_state.selected_project
        st.info(f"📂 Dự án: **{project['name']}** (ID: {project['id']})")
        
        # 4. LIST TASKS
        st.subheader("Danh sách công việc")
        if st.button("🔄 Tải công việc", key="load_tasks"):
            res = send_request("list_tasks", st.session_state.session, {"project_id": project['id']})
            if res.get("status") == "code(ok)":
                tasks = res["data"]["tasks"]
                if tasks:
                    for task in tasks:
                        with st.expander(f"📌 {task.get('name', 'Unnamed')} - {task.get('status', 'N/A')}"):
                            st.write(f"**ID:** {task['id']}")
                            st.write(f"**Trạng thái:** {task.get('status','N/A')}")
                            st.write(f"**Người thực hiện:** {task.get('assignee','Chưa gán')}")
                            st.write(f"**Mô tả:** {task.get('description','Không có')}")
                            col1, col2 = st.columns(2)
                            with col1:
                                new_status = st.selectbox(
                                    "Thay đổi trạng thái",
                                    ["todo","in_progress","done","blocked"],
                                    key=f"status_{task['id']}"
                                )
                            with col2:
                                st.write("")
                                if st.button("Cập nhật", key=f"update_{task['id']}"):
                                    res2 = send_request("update_task_status", st.session_state.session,
                                                        {"task_id": task['id'], "status": new_status})
                                    if res2.get("status") == "code(ok)":
                                        st.success("✅ Cập nhật thành công!")
                                        st.rerun()
                                    else:
                                        st.error(res2.get("message"))
                else:
                    st.info("Dự án chưa có công việc nào")
            else:
                st.error(res.get("message"))
        
        st.divider()
        
        # 5. CREATE TASK
        st.subheader("➕ Tạo công việc mới")
        with st.form("create_task_form"):
            task_name = st.text_input("Tên công việc")
            task_desc = st.text_area("Mô tả công việc")
            submit_task = st.form_submit_button("Tạo công việc", type="primary")
            if submit_task:
                if task_name:
                    res2 = send_request("create_task", st.session_state.session,
                                        {"project_id": project['id'], "name": task_name, "description": task_desc})
                    if res2.get("status") == "code(ok)":
                        st.success(f"✅ Tạo công việc '{task_name}' thành công!")
                    else:
                        st.error(res2.get("message", "Tạo công việc thất bại"))
                else:
                    st.warning("Vui lòng nhập tên công việc")
        
        st.divider()
        
        # 6. ASSIGN TASK
        st.subheader("👤 Gán công việc")
        with st.form("assign_task_form"):
            col1, col2 = st.columns(2)
            with col1:
                assign_task_id = st.text_input("ID công việc")
            with col2:
                assign_username = st.text_input("Username người nhận")
            submit_assign = st.form_submit_button("Gán công việc", type="primary")
            if submit_assign:
                if assign_task_id and assign_username:
                    res3 = send_request("assign_task", st.session_state.session,
                                        {"task_id": assign_task_id, "username": assign_username})
                    if res3.get("status") == "code(ok)":
                        st.success(f"✅ Đã gán công việc cho {assign_username}")
                    else:
                        st.error(res3.get("message", "Gán công việc thất bại"))
                else:
                    st.warning("Vui lòng nhập đầy đủ thông tin")
    else:
        st.warning("⚠️ Vui lòng chọn dự án từ tab 'Dự án'")

# ============================
# TAB 3: MEMBER MANAGEMENT
# ============================
with tab3:
    st.header("👥 Quản lý thành viên")
    if st.session_state.selected_project:
        project = st.session_state.selected_project
        st.info(f"📂 Dự án: **{project['name']}** (ID: {project['id']})")
        
        # ADD MEMBER
        st.subheader("➕ Thêm thành viên mới")
        with st.form("add_member_form"):
            member_username = st.text_input("Username thành viên")
            member_role = st.selectbox("Vai trò", ["member", "admin", "viewer"])
            submit_member = st.form_submit_button("Thêm thành viên", type="primary")
            if submit_member:
                if member_username:
                    res4 = send_request("add_member", st.session_state.session,
                                        {"project_id": project['id'], "username": member_username, "role": member_role})
                    if res4.get("status") == "code(ok)":
                        st.success(f"✅ Đã thêm {member_username} vào dự án")
                    else:
                        st.error(res4.get("message", "Thêm thành viên thất bại"))
                else:
                    st.warning("Vui lòng nhập username")
        
        st.divider()
        st.subheader("📋 Danh sách thành viên hiện tại")
        st.info("Chức năng xem danh sách thành viên cần API 'list_members'")
    else:
        st.warning("⚠️ Vui lòng chọn dự án từ tab 'Dự án'")

# ============================
# TAB 4: COMMENTS MANAGEMENT
# ============================
with tab4:
    st.header("💬 Nhận xét công việc")
    if st.session_state.selected_project:
        project = st.session_state.selected_project
        st.info(f"📂 Dự án: **{project['name']}** (ID: {project['id']})")
        
        # ADD COMMENT
        st.subheader("✍️ Thêm nhận xét")
        with st.form("comment_task_form"):
            comment_task_id = st.text_input("ID công việc")
            comment_content = st.text_area("Nội dung nhận xét")
            submit_comment = st.form_submit_button("Gửi nhận xét", type="primary")
            if submit_comment:
                if comment_task_id and comment_content:
                    res5 = send_request("comment_task", st.session_state.session,
                                        {"task_id": comment_task_id, "comment": comment_content})
                    if res5.get("status") == "code(ok)":
                        st.success("✅ Đã gửi nhận xét thành công!")
                    else:
                        st.error(res5.get("message", "Gửi nhận xét thất bại"))
                else:
                    st.warning("Vui lòng nhập đầy đủ thông tin")
        
        st.divider()
        st.subheader("📜 Xem nhận xét")
        view_comment_task_id = st.text_input("Nhập ID công việc để xem nhận xét", key="view_comments")
        if st.button("Xem nhận xét"):
            if view_comment_task_id:
                st.info("Chức năng xem nhận xét cần API 'get_comments'")
            else:
                st.warning("Vui lòng nhập ID công việc")
    else:
        st.warning("⚠️ Vui lòng chọn dự án từ tab 'Dự án'")

st.divider()
st.caption("🟦 Client Project Manager | Session: " + st.session_state.session[:20] + "...")
