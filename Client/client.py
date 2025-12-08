import streamlit as st
import subprocess
import json

# ============================
# client_stdin
# ============================

if "client" not in st.session_state:
    st.session_state.client = subprocess.Popen(
        ["F:/LTM-2025.1/Prj/Client/mock_stdin.exe"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        text=True,
        bufsize=1    
    )

client = st.session_state.client


def send_cmd(cmd):
    print(cmd)
    try:
        client.stdin.write(cmd + "\n")
        client.stdin.flush()
        resp = client.stdout.readline().strip()
        print("RAW:", resp)
        return json.loads(resp)
    except Exception as e:
        return {"status": "error", "message": str(e)}


# ============================
#  session state
# ============================
if "session" not in st.session_state:
    st.session_state.session = ""
if "username" not in st.session_state:
    st.session_state.username = ""
if "selected_project" not in st.session_state:
    st.session_state.selected_project = None
if "user_role" not in st.session_state:
    st.session_state.user_role = {}  # {project_id: role}


# ============================
# Streamlit UI
# ============================

st.title("🟦 Client Project Manager")

# ============================
# 🔐 LOGIN
# ============================

if st.session_state.session == "":
    st.subheader("🔐 Đăng nhập")
    
    col1, col2 = st.columns(2)
    with col1:
        username = st.text_input("Username")
    with col2:
        password = st.text_input("Password", type="password")
    
    if st.button("Login", type="primary"):
        req = json.dumps({
            "action": "login",
            "session": "",
            "data": {"username": username, "password": password}
        },separators=(',', ':'))
        res = send_cmd(req)
        
        if res.get("status") == "code(ok)":
            st.session_state.session = res["data"]["session"]
            st.session_state.username = username
            st.success("✅ Đăng nhập thành công!")
            st.rerun()
        else:
            st.error(f"❌ {res.get('message', 'Đăng nhập thất bại')}")
    
    st.stop()


# ============================
# HEADER -USER INFO & LOGOUT
# ============================

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


# ============================
# TABS - FUNCTIONALITYs
# ============================

tab1, tab2, tab3, tab4 = st.tabs(["📋 Dự án", "✅ Công việc", "👥 Thành viên", "💬 Nhận xét"])


# ============================
# TAB 1: PROJECT MANAGEMENT
# ============================

with tab1:
    st.header("📋 Quản lý dự án")
    
    # 1. PROJECT LIST
    st.subheader("Danh sách dự án của bạn")
    
    col1, col2 = st.columns([1, 3])
    with col1:
        if st.button("🔄 Làm mới danh sách", key="refresh_projects"):
            req = json.dumps({
                "action": "list_projects",
                "session": st.session_state.session,
                "data": {}
            },separators=(',', ':'))
            res = send_cmd(req)
            
            if res.get("status") == "code(ok)":
                projects = res["data"]["projects"]
                if projects:
                    for p in projects:
                        with st.container():
                            col_a, col_b, col_c = st.columns([2, 2, 1])
                            with col_a:
                                st.write(f"**{p['name']}**")
                            with col_b:
                                st.write(f"ID: {p['id']}")
                            with col_c:
                                if st.button("Chọn", key=f"select_{p['id']}"):
                                    st.session_state.selected_project = p
                                    st.rerun()
                else:
                    st.info("Bạn chưa tham gia dự án nào")
            else:
                st.error(res.get("message"))
    #token k hop le
    st.divider()
    
    # 2. PROJECT SEARCH
    st.subheader("🔍 Tìm kiếm dự án")
    
    col1, col2 = st.columns([3, 1])
    with col1:
        search_kw = st.text_input("Nhập từ khóa", key="search_input")
    with col2:
        st.write("")
        st.write("")
        search_btn = st.button("Tìm kiếm", type="primary")
    
    if search_btn and search_kw:
        req = json.dumps({
            "action": "search_project",
            "session": st.session_state.session,
            "data": {"keyword": search_kw}
        },separators=(',', ':'))
        res = send_cmd(req)
        
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
    #token k hop le
    st.divider()
    
    # 5. CREATE PROJECT
    st.subheader("➕ Tạo dự án mới")
    
    with st.form("create_project_form"):
        new_project_name = st.text_input("Tên dự án")
        new_project_desc = st.text_area("Mô tả dự án")
        
        submit_project = st.form_submit_button("Tạo dự án", type="primary")
        
        if submit_project:
            if new_project_name:
                req = json.dumps({
                    "action": "create_project",
                    "session": st.session_state.session,
                    "data": {
                        "name": new_project_name,
                        "description": new_project_desc
                    }
                },separators=(',', ':'))
                res = send_cmd(req)
                
                if res.get("status") == "code(ok)":
                    st.success(f"✅ Tạo dự án '{new_project_name}' thành công!")
                else:
                    st.error(f"❌ {res.get('message', 'Tạo dự án thất bại')}")
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
        
        # 3. LIST TASKS
        st.subheader("Danh sách công việc")
        
        if st.button("🔄 Tải công việc", key="load_tasks"):
            req = json.dumps({
                "action": "list_tasks",
                "session": st.session_state.session,
                "data": {"project_id": project['id']}
            },separators=(',', ':'))
            res = send_cmd(req)
            
            if res.get("status") == "code(ok)":
                tasks = res["data"]["tasks"]
                if tasks:
                    for task in tasks:
                        with st.expander(f"📌 {task.get('name', 'Unnamed Task')} - {task.get('status', 'N/A')}"):
                            st.write(f"**ID:** {task['id']}")
                            st.write(f"**Trạng thái:** {task.get('status', 'N/A')}")
                            st.write(f"**Người thực hiện:** {task.get('assignee', 'Chưa gán')}")
                            st.write(f"**Mô tả:** {task.get('description', 'Không có mô tả')}")
                            
                            # 4. UPDATE TASK STATUS
                            st.write("---")
                            col1, col2 = st.columns(2)
                            with col1:
                                new_status = st.selectbox(
                                    "Thay đổi trạng thái",
                                    ["todo", "in_progress", "done", "blocked"],
                                    key=f"status_{task['id']}"
                                )
                            with col2:
                                st.write("")
                                if st.button("Cập nhật", key=f"update_{task['id']}"):
                                    req = json.dumps({
                                        "action": "update_task_status",
                                        "session": st.session_state.session,
                                        "data": {
                                            "task_id": task['id'],
                                            "status": new_status
                                        }
                                    },separators=(',', ':'))
                                    res = send_cmd(req)
                                    
                                    if res.get("status") == "code(ok)":
                                        st.success("✅ Cập nhật thành công!")
                                        st.rerun()
                                    else:
                                        st.error(res.get("message"))
                else:
                    st.info("Dự án chưa có công việc nào")
            else:
                st.error(res.get("message"))
        
        st.divider()
        
        # 7. CREATE TASK
        st.subheader("➕ Tạo công việc mới")
        
        with st.form("create_task_form"):
            task_name = st.text_input("Tên công việc")
            task_desc = st.text_area("Mô tả công việc")
            
            submit_task = st.form_submit_button("Tạo công việc", type="primary")
            
            if submit_task:
                if task_name:
                    req = json.dumps({
                        "action": "create_task",
                        "session": st.session_state.session,
                        "data": {
                            "project_id": project['id'],
                            "name": task_name,
                            "description": task_desc
                        }
                    },separators=(',', ':'))
                    res = send_cmd(req)
                    
                    if res.get("status") == "code(ok)":
                        st.success(f"✅ Tạo công việc '{task_name}' thành công!")
                    else:
                        st.error(f"❌ {res.get('message', 'Tạo công việc thất bại')}")
                else:
                    st.warning("Vui lòng nhập tên công việc")
        
        st.divider()
        
        # 8. ASSIGN TASK
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
                    req = json.dumps({
                        "action": "assign_task",
                        "session": st.session_state.session,
                        "data": {
                            "task_id": assign_task_id,
                            "username": assign_username
                        }
                    },separators=(',', ':'))
                    res = send_cmd(req)
                    
                    if res.get("status") == "code(ok)":
                        st.success(f"✅ Đã gán công việc cho {assign_username}")
                    else:
                        st.error(f"❌ {res.get('message', 'Gán công việc thất bại')}")
                else:
                    st.warning("Vui lòng nhập đầy đủ thông tin")
        
    else:
        st.warning("⚠️ Vui lòng chọn một dự án từ tab 'Dự án'")


# ============================
# TAB 3: MEMBER MANAGEMENT
# ============================

with tab3:
    st.header("👥 Quản lý thành viên")
    
    if st.session_state.selected_project:
        project = st.session_state.selected_project
        st.info(f"📂 Dự án: **{project['name']}** (ID: {project['id']})")
        
        # 6. ADD MEMBER
        st.subheader("➕ Thêm thành viên mới")
        
        st.caption("⚠️ Chức năng này yêu cầu quyền admin/owner")
        
        with st.form("add_member_form"):
            member_username = st.text_input("Username thành viên")
            member_role = st.selectbox(
                "Vai trò",
                ["member", "admin", "viewer"],
                help="member: Thành viên thường | admin: Quản trị viên | viewer: Chỉ xem"
            )
            
            submit_member = st.form_submit_button("Thêm thành viên", type="primary")
            
            if submit_member:
                if member_username:
                    req = json.dumps({
                        "action": "add_member",
                        "session": st.session_state.session,
                        "data": {
                            "project_id": project['id'],
                            "username": member_username,
                            "role": member_role
                        }
                    },separators=(',', ':'))
                    res = send_cmd(req)
                    
                    if res.get("status") == "code(ok)":
                        st.success(f"✅ Đã thêm {member_username} vào dự án")
                    else:
                        st.error(f"❌ {res.get('message', 'Thêm thành viên thất bại')}")
                else:
                    st.warning("Vui lòng nhập username")
        
        st.divider()
        
        # Hiển thị danh sách thành viên (nếu có API)
        st.subheader("📋 Danh sách thành viên hiện tại")
        st.info("Chức năng xem danh sách thành viên cần API 'list_members'")
        
    else:
        st.warning("⚠️ Vui lòng chọn một dự án từ tab 'Dự án'")


# ============================
# TAB 4: COMMENT MANAGEMENT
# ============================

with tab4:
    st.header("💬 Nhận xét công việc")
    
    if st.session_state.selected_project:
        project = st.session_state.selected_project
        st.info(f"📂 Dự án: **{project['name']}** (ID: {project['id']})")
        
        # 9. ADD COMMENT
        st.subheader("✍️ Thêm nhận xét")
        
        with st.form("comment_task_form"):
            comment_task_id = st.text_input("ID công việc")
            comment_content = st.text_area(
                "Nội dung nhận xét",
                height=150,
                placeholder="Nhập nhận xét của bạn về công việc này..."
            )
            
            submit_comment = st.form_submit_button("Gửi nhận xét", type="primary")
            
            if submit_comment:
                if comment_task_id and comment_content:
                    req = json.dumps({
                        "action": "comment_task",
                        "session": st.session_state.session,
                        "data": {
                            "task_id": comment_task_id,
                            "comment": comment_content
                        }
                    },separators=(',', ':'))
                    res = send_cmd(req)
                    
                    if res.get("status") == "code(ok)":
                        st.success("✅ Đã gửi nhận xét thành công!")
                    else:
                        st.error(f"❌ {res.get('message', 'Gửi nhận xét thất bại')}")
                else:
                    st.warning("Vui lòng nhập đầy đủ thông tin")
        
        st.divider()
        
        # 10. VIEW COMMENTS
        st.subheader("📜 Xem nhận xét")
        view_comment_task_id = st.text_input("Nhập ID công việc để xem nhận xét", key="view_comments")
        
        if st.button("Xem nhận xét"):
            if view_comment_task_id:
                st.info("Chức năng xem nhận xét cần API 'get_comments'")
            else:
                st.warning("Vui lòng nhập ID công việc")
        
    else:
        st.warning("⚠️ Vui lòng chọn một dự án từ tab 'Dự án'")


# ============================
# FOOTER
# ============================

st.divider()
st.caption("🟦 Client Project Manager | Session: " + st.session_state.session[:20] + "...")