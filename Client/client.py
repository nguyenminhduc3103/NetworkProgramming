import streamlit as st
import socket
import json

# ============================
# CONFIG SERVER
# ============================
SERVER_HOST = "172.31.245.233"
SERVER_PORT = 8080

STATUS_MAP = {
    "101": "Đăng nhập thành công",
    "102": "Đăng ký thành công",
    "103": "Lấy danh sách dự án thành công",
    "104": "Tìm kiếm thành công",
    "105": "Tạo dự án thành công",
    "106": "Thêm thành viên thành công",
    "107": "Lấy danh sách task thành công",
    "108": "Tạo task thành công",
    "109": "Gán task thành công",
    "110": "Cập nhật task thành công",
    "111": "Gửi nhận xét thành công",
    "165": "Tên dự án đã tồn tại",
    "156": "Không tìm thấy người dùng",
    "269": "Lấy danh sách thành viên thành công",
    "509": "Gán Task thất bại",
    "512": "Lỗi hệ thống (Server Error)"
}

TASK_STATUS = {
    "todo": {"label": "📝 Chờ làm", "color": "blue"},
    "in_progress": {"label": "⚡ Đang làm", "color": "orange"},
    "done": {"label": "✅ Hoàn thành", "color": "green"},
    "blocked": {"label": "🚫 Bị chặn", "color": "red"}
}

# ============================
# HELPER FUNCTIONS
# ============================
def send_request(action, session="", data={}):
    print(f"\n=== SENDING REQUEST ===")
    print(f"Action: {action}")
    print(f"Session: {session}")
    print(f"Data: {data}")
    
    request = json.dumps({
        "action": action,
        "session": session,
        "data": data
    }, separators=(',', ':')) + "\r\n"
    
    print(f"Request JSON: {request}")

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.settimeout(20)
        s.connect((SERVER_HOST, SERVER_PORT))
        s.sendall(request.encode('utf-8'))

        chunks = []
        while True:
            chunk = s.recv(8192)
            if not chunk:
                break
            chunks.append(chunk)
            if len(chunk) < 8192:
                break
        
        buffer = b''.join(chunks)
        resp = buffer.decode('utf-8').strip()
        
        print(f"Response: {resp}")
        
        # Handle multiple responses separated by \r\n
        if "\r\n" in resp:
            resp = resp.split("\r\n")[0]
        
        return json.loads(resp)

def show_message(res, success_code):
    status = str(res.get("status"))
    if status == success_code:
        st.toast(STATUS_MAP.get(status, "Thành công!"), icon="✅")
        return True
    else:
        err_msg = STATUS_MAP.get(status, res.get("message", "Lỗi không xác định"))
        st.error(f"Lỗi ({status}): {err_msg}")
        return False

def load_tasks(project_id):
    """Tải danh sách task và tự động cập nhật vào session_state"""
    res = send_request("list_tasks", st.session_state.session, {"project_id": project_id})
    if res.get("status") == "107":
        st.session_state.tasks_list = res["data"].get("tasks", [])
        return True
    return False

def load_project_details(project_id):
    """Tải thông tin chi tiết dự án"""
    res = send_request("project_detail", st.session_state.session, {"project_id": project_id})
    if res.get("status") in ["103", "200"]:  # Success codes
        return res.get("data", {})
    return None

def load_members(project_id):
    """Tải danh sách thành viên"""
    res = send_request("list_members", st.session_state.session, {"project_id": project_id})
    if res.get("status") == "269":
        # Handle both dict and list responses
        data = res.get("data", [])
        if isinstance(data, list):
            st.session_state.members_list = data
        else:
            st.session_state.members_list = []
        return True
    return False

# ============================
# SESSION STATE 
# ============================
if "session" not in st.session_state: st.session_state.session = ""
if "username" not in st.session_state: st.session_state.username = ""
if "selected_project" not in st.session_state: st.session_state.selected_project = None
if "project_details" not in st.session_state: st.session_state.project_details = None
if "projects_list" not in st.session_state: st.session_state.projects_list = [] 
if "selected_task" not in st.session_state: st.session_state.selected_task = None
if "tasks_list" not in st.session_state: st.session_state.tasks_list = []
if "members_list" not in st.session_state: st.session_state.members_list = []
if "view_mode" not in st.session_state: st.session_state.view_mode = "list"
if "show_create_task_form" not in st.session_state: st.session_state.show_create_task_form = False
if "show_add_member_form" not in st.session_state: st.session_state.show_add_member_form = False

# ============================
# AUTHENTICATION UI
# ============================
if not st.session_state.session:
    st.markdown("### 🔐 Đăng nhập hệ thống quản lý dự án")
    
    with st.container():
        user_input = st.text_input("Username", placeholder="Nhập tên đăng nhập...")
        pass_input = st.text_input("Password", type="password", placeholder="Nhập mật khẩu...")
        
        col1, col2 = st.columns(2)
        if col1.button("🚀 Đăng nhập", use_container_width=True, type="primary"):
            if user_input and pass_input:
                res = send_request("login", data={"username": user_input, "password": pass_input})
                if res.get("status") == "101":
                    st.session_state.session = res["data"]["session"]
                    st.session_state.username = user_input
                    st.rerun()
                else: 
                    show_message(res, "101")
            else:
                st.warning("Vui lòng điền đầy đủ thông tin!")
                
        if col2.button("📝 Đăng ký", use_container_width=True):
            if user_input and pass_input:
                res = send_request("register", data={"username": user_input, "password": pass_input})
                show_message(res, "102")
            else:
                st.warning("Vui lòng điền đầy đủ thông tin!")
    st.stop()

# ============================
# SIDEBAR
# ============================
with st.sidebar:
    st.title(f"👤 {st.session_state.username}")
    
    if st.session_state.selected_project:
        st.success(f"*Dự án hiện tại:*")
        st.info(f"📂 {st.session_state.selected_project['project_name']}")
        
        # Quick stats
        if st.session_state.project_details:
            details = st.session_state.project_details
            st.metric("📋 Tasks", len(st.session_state.tasks_list))
            st.metric("👥 Thành viên", len(st.session_state.members_list))
        
        if st.button("🔄 Đổi dự án", use_container_width=True):
            st.session_state.selected_project = None
            st.session_state.selected_task = None
            st.session_state.tasks_list = []
            st.session_state.members_list = []
            st.session_state.project_details = None
            st.rerun()
    
    st.divider()
    
    if st.button("🚪 Đăng xuất", use_container_width=True):
        st.session_state.clear()
        st.rerun()

# ============================
# MAIN INTERFACE
# ============================

# Nếu chưa chọn dự án -> Hiển thị danh sách dự án
if not st.session_state.selected_project:
    st.title("📂 Quản lý Dự án")
    
    # Search & Refresh
    col_search, col_btn1, col_btn2 = st.columns([3, 1, 1])
    with col_search:
        search_kw = st.text_input("🔍 Tìm kiếm dự án", placeholder="Nhập từ khóa...")
    with col_btn1:
        st.write("##")
        if st.button("🔍 Tìm", use_container_width=True):
            action = "search_project" if search_kw else "list_projects"
            data = {"keyword": search_kw} if search_kw else {}
            res = send_request(action, st.session_state.session, data)
            if res.get("status") in ["103", "104"]:
                st.session_state.projects_list = res.get("data", [])
    with col_btn2:
        st.write("##")
        if st.button("🔄 Tải lại", use_container_width=True):
            res = send_request("list_projects", st.session_state.session, {})
            if res.get("status") == "103":
                st.session_state.projects_list = res.get("data", [])

    st.divider()

    # Display projects
    if not st.session_state.projects_list:
        st.info("💡 Chưa có dữ liệu. Nhấn 'Tải lại' hoặc tạo dự án mới.")
    else:
        cols = st.columns(2)
        for idx, p in enumerate(st.session_state.projects_list):
            with cols[idx % 2]:
                with st.container(border=True):
                    st.markdown(f"### 📁 {p.get('project_name')}")
                    st.caption(f"ID: {p.get('project_id')}")
                    if st.button("➡️ Mở dự án", key=f"open_{p.get('project_id')}", use_container_width=True):
                        st.session_state.selected_project = p
                        # Tự động tải thông tin chi tiết
                        with st.spinner("Đang tải thông tin dự án..."):
                            st.session_state.project_details = load_project_details(p['project_id'])
                            load_tasks(p['project_id'])
                            load_members(p['project_id'])
                        st.rerun()

    # Create new project
    st.divider()
    with st.expander("➕ *Tạo dự án mới*"):
        with st.form("new_project_form"):
            pname = st.text_input("Tên dự án")
            pdesc = st.text_area("Mô tả")
            if st.form_submit_button("✨ Tạo dự án", use_container_width=True):
                if pname:
                    res = send_request("create_project", st.session_state.session, {"project_name": pname, "description": pdesc})
                    if show_message(res, "105"):
                        # Refresh project list
                        res2 = send_request("list_projects", st.session_state.session, {})
                        if res2.get("status") == "103":
                            st.session_state.projects_list = res2.get("data", [])
                        st.rerun()
                else:
                    st.warning("Vui lòng nhập tên dự án!")

else:
    # ĐÃ CHỌN DỰ ÁN -> Hiển thị tabs quản lý
    prj = st.session_state.selected_project
    st.title(f"📂 {prj['project_name']}")
    
    tab1, tab2, tab3, tab4 = st.tabs(["ℹ️ Thông tin", "📋 Công việc", "👥 Thành viên", "💬 Nhận xét"])
    
    # ============================
    # TAB 0: THÔNG TIN DỰ ÁN
    # ============================
    with tab1:
        col_header, col_refresh = st.columns([4, 1])
        with col_header:
            st.markdown("### 📊 Chi tiết dự án")
        with col_refresh:
            if st.button("🔄 Làm mới", key="refresh_project_detail", use_container_width=True):
                with st.spinner("Đang tải..."):
                    st.session_state.project_details = load_project_details(prj['project_id'])
                    load_tasks(prj['project_id'])
                    load_members(prj['project_id'])
                st.rerun()
        
        st.divider()
        
        # Display project details
        if st.session_state.project_details:
            details = st.session_state.project_details
            
            # Basic info
            with st.container(border=True):
                st.markdown("#### 📝 Thông tin cơ bản")
                col1, col2 = st.columns(2)
                with col1:
                    st.markdown(f"*Tên dự án:* {details.get('project_name', prj['project_name'])}")
                    st.markdown(f"*ID dự án:* {details.get('project_id', prj['project_id'])}")
                with col2:
                    st.markdown(f"*Trạng thái:* {details.get('status', 'Active')}")
                    st.markdown(f"*Người tạo:* {details.get('created_by', 'N/A')}")
                
                if details.get('description'):
                    st.markdown("*Mô tả:*")
                    st.info(details.get('description'))
                
                if details.get('created_at'):
                    st.caption(f"🗓️ Ngày tạo: {details.get('created_at')}")
        
        else:
            st.info("Đang tải thông tin dự án...")
        
        # Statistics
        st.divider()
        st.markdown("#### 📈 Thống kê")
        
        col1, col2, col3, col4 = st.columns(4)
        
        # Task statistics
        total_tasks = len(st.session_state.tasks_list)
        tasks_by_status = {"todo": 0, "in_progress": 0, "done": 0, "blocked": 0}
        for task in st.session_state.tasks_list:
            status = task.get('status', 'todo')
            if status in tasks_by_status:
                tasks_by_status[status] += 1
        
        with col1:
            st.metric("📋 Tổng Task", total_tasks)
        with col2:
            st.metric("✅ Hoàn thành", tasks_by_status['done'])
        with col3:
            st.metric("⚡ Đang làm", tasks_by_status['in_progress'])
        with col4:
            st.metric("👥 Thành viên", len(st.session_state.members_list))
        
        # Progress bar
        if total_tasks > 0:
            progress = tasks_by_status['done'] / total_tasks
            st.progress(progress, text=f"Tiến độ hoàn thành: {progress*100:.1f}%")
        
        # Member list summary
        st.divider()
        st.markdown("#### 👥 Danh sách thành viên")
        
        if st.session_state.members_list:
            # Group by role
            members_by_role = {"PM": [], "DEV": [], "MEMBER": []}
            for mem in st.session_state.members_list:
                role = mem.get('role', 'MEMBER')
                if role in members_by_role:
                    members_by_role[role].append(mem)
            
            col1, col2, col3 = st.columns(3)
            with col1:
                st.markdown("**🎯 Project Manager**")
                if members_by_role['PM']:
                    for mem in members_by_role['PM']:
                        st.markdown(f"- {mem.get('username')}")
                else:
                    st.caption("Chưa có")
            
            with col2:
                st.markdown("**💻 Developer**")
                if members_by_role['DEV']:
                    for mem in members_by_role['DEV']:
                        st.markdown(f"- {mem.get('username')}")
                else:
                    st.caption("Chưa có")
            
            with col3:
                st.markdown("**👤 Member**")
                if members_by_role['MEMBER']:
                    for mem in members_by_role['MEMBER']:
                        st.markdown(f"- {mem.get('username')}")
                else:
                    st.caption("Chưa có")
        else:
            st.info("Chưa có thành viên nào trong dự án")
    
    # ============================
    # TAB 1: CÔNG VIỆC
    # ============================
    with tab2:
        # Header actions
        col1, col2, col3, col4 = st.columns([2, 1, 1, 1])
        with col1:
            st.markdown("### Danh sách Task")
        with col2:
            view_btn = st.segmented_control(
                "Hiển thị",
                options=["📋 Danh sách", "📊 Kanban"],
                default="📋 Danh sách",
                label_visibility="collapsed"
            )
        with col3:
            if st.button("🔄 Làm mới", key="refresh_tasks", use_container_width=True):
                load_tasks(prj['project_id'])
                st.rerun()
        with col4:
            if st.button("➕ Task mới", use_container_width=True, type="primary"):
                st.session_state.show_create_task_form = True
        
        st.divider()
        
        # Create task modal
        if st.session_state.show_create_task_form:
            with st.form("quick_create_task"):
                st.markdown("#### ➕ Tạo Task mới")
                new_t_name = st.text_input("Tên task")
                new_t_desc = st.text_area("Mô tả")
                col_submit, col_cancel = st.columns(2)
                submitted = col_submit.form_submit_button("✅ Tạo", use_container_width=True)
                cancelled = col_cancel.form_submit_button("❌ Hủy", use_container_width=True)
            
            # Process form submission
            if submitted:
                if new_t_name:
                    try:
                        res = send_request("create_task", st.session_state.session, 
                                          {"project_id": prj['project_id'], "task_name": new_t_name, "description": new_t_desc})
                        if show_message(res, "108"):
                            st.session_state.show_create_task_form = False
                            load_tasks(prj['project_id'])
                            st.rerun()
                    except Exception as e:
                        st.error(f"Lỗi: {str(e)}")
                else:
                    st.warning("Vui lòng nhập tên task!")
            
            if cancelled:
                st.session_state.show_create_task_form = False
                st.rerun()
        
        # Display tasks
        if not st.session_state.tasks_list:
            st.info("📭 Dự án chưa có task nào. Hãy tạo task đầu tiên!")
        else:
            # Kanban view
            if view_btn == "📊 Kanban":
                # Group tasks by status
                grouped_tasks = {
                    "todo": [],
                    "in_progress": [],
                    "done": [],
                    "blocked": []
                }
                for task in st.session_state.tasks_list:
                    status = task.get('status', 'todo')
                    grouped_tasks[status].append(task)
                
                kanban_cols = st.columns(4)
                for idx, (status_key, status_info) in enumerate(TASK_STATUS.items()):
                    with kanban_cols[idx]:
                        st.markdown(f"*{status_info['label']}* ({len(grouped_tasks[status_key])})")
                        for task in grouped_tasks[status_key]:
                            with st.container(border=True):
                                st.markdown(f"*{task.get('task_name')}*")
                                st.caption(f"ID: {task.get('task_id')}")
                                assignee = task.get('assigned_to_username') or task.get('assigned_to')
                                if assignee:
                                    st.caption(f"👤 {assignee}")
                                if st.button("👁️ Xem", key=f"view_{task.get('task_id')}", use_container_width=True):
                                    st.session_state.selected_task = task
                                    st.rerun()
            
            # List view (default)
            else:
                for task in st.session_state.tasks_list:
                    status = task.get('status', 'todo')
                    status_info = TASK_STATUS.get(status, {"label": status, "color": "gray"})
                    
                    with st.container(border=True):
                        col_info, col_status, col_action = st.columns([4, 2, 1])
                        
                        with col_info:
                            st.markdown(f"*{task.get('task_name')}*")
                            st.caption(f"ID: {task.get('task_id')}")
                            assignee = task.get('assigned_to_username') or task.get('assigned_to')
                            if assignee:
                                st.caption(f"👤 {assignee}")
                        
                        with col_status:
                            if status_info['color'] == "green":
                                st.success(status_info['label'])
                            elif status_info['color'] == "orange":
                                st.warning(status_info['label'])
                            elif status_info['color'] == "red":
                                st.error(status_info['label'])
                            else:
                                st.info(status_info['label'])
                        
                        with col_action:
                            if st.button("📝 Chi tiết", key=f"detail_{task.get('task_id')}", use_container_width=True):
                                st.session_state.selected_task = task
                                st.rerun()
        
        # Task detail modal
        if st.session_state.selected_task:
            task = st.session_state.selected_task
            st.divider()
            
            with st.container(border=True):
                # Header
                col_title, col_close = st.columns([5, 1])
                with col_title:
                    st.markdown(f"### 📝 {task.get('task_name')}")
                with col_close:
                    if st.button("❌", key="close_task"):
                        st.session_state.selected_task = None
                        st.rerun()
                
                st.caption(f"Task ID: {task.get('task_id')}")
                assignee = task.get('assigned_to_username') or task.get('assigned_to')
                if assignee:
                    st.caption(f"👤 Giao cho: {assignee}")
                
                # Show comments if available
                comments = task.get('comments', [])
                st.markdown("#### 💬 Nhận xét của task")
                if comments:
                    for cmt in comments:
                        with st.container(border=True):
                            st.markdown(f"**{cmt.get('username', 'Ẩn danh')}**  ")
                            st.caption(f"🕒 {cmt.get('created_at', '')}")
                            st.write(cmt.get('comment', ''))
                else:
                    st.info("Chưa có nhận xét nào cho task này.")

                # Task details in columns
                col_left, col_right = st.columns(2)
                
                with col_left:
                    st.markdown("#### 📊 Trạng thái")
                    current_status = task.get('status', 'todo')
                    status_options = list(TASK_STATUS.keys())
                    current_idx = status_options.index(current_status) if current_status in status_options else 0
                    
                    new_status = st.selectbox(
                        "Chọn trạng thái mới",
                        options=status_options,
                        index=current_idx,
                        format_func=lambda x: TASK_STATUS[x]['label'],
                        key=f"status_select_{task.get('task_id')}"
                    )
                    
                    if st.button("💾 Cập nhật trạng thái", use_container_width=True):
                        res = send_request("update_task", st.session_state.session, 
                                          {"task_id": task['task_id'], "status": new_status})
                        if show_message(res, "110"):
                            load_tasks(prj['project_id'])
                            st.session_state.selected_task = None
                            st.rerun()
                
                with col_right:
                    st.markdown("#### 👤 Gán công việc")
                    assign_user = st.text_input(
                        "Username người thực hiện",
                        value=task.get('assigned_to', ''),
                        placeholder="Nhập username...",
                        key=f"assign_{task.get('task_id')}"
                    )
                    
                    if st.button("✅ Gán task", use_container_width=True):
                        if assign_user:
                            res = send_request("assign_task", st.session_state.session, 
                                              {"task_id": task['task_id'], "assigned_to": assign_user})
                            if show_message(res, "109"):
                                load_tasks(prj['project_id'])
                                st.rerun()
                        else:
                            st.warning("Vui lòng nhập username!")
                
                # Comment section
                st.divider()
                st.markdown("#### 💬 Nhận xét")
                with st.form(f"comment_form_{task.get('task_id')}"):
                    comment_text = st.text_area("Viết nhận xét...", placeholder="Thêm nhận xét của bạn về task này")
                    send_cmt = st.form_submit_button("📤 Gửi nhận xét", use_container_width=True)
                    if send_cmt:
                        if comment_text:
                            with st.spinner("Đang gửi nhận xét..."):
                                res = send_request("comment_task", st.session_state.session, {"task_id": task['task_id'], "comment": comment_text})
                                import time; time.sleep(0.5)
                            if show_message(res, "111"):
                                st.success("Gửi nhận xét thành công!")
                                st.rerun()
                        else:
                            st.warning("Vui lòng nhập nội dung nhận xét!")
    
    # ============================
    # TAB 2: THÀNH VIÊN
    # ============================
    with tab3:
        col_header, col_refresh, col_add = st.columns([3, 1, 1])
        with col_header:
            st.markdown("### Danh sách thành viên")
        with col_refresh:
            if st.button("🔄 Làm mới", key="refresh_members", use_container_width=True):
                load_members(prj['project_id'])
                st.rerun()
        with col_add:
            if st.button("➕ Thêm", key="add_member_btn", use_container_width=True, type="primary"):
                st.session_state.show_add_member_form = True
        
        st.divider()
        
        # Add member form (persist with session_state)
        if st.session_state.show_add_member_form:
            with st.form("add_member_form"):
                st.markdown("#### ➕ Thêm thành viên mới")
                new_mem_user = st.text_input("Username")
                new_mem_role = st.selectbox("Vai trò", ["MEMBER", "DEV", "PM"])
                
                col_submit, col_cancel = st.columns(2)
                submitted = col_submit.form_submit_button("✅ Thêm", use_container_width=True)
                cancelled = col_cancel.form_submit_button("❌ Hủy", use_container_width=True)

            if submitted:
                if new_mem_user:
                    res = send_request("add_member", st.session_state.session, {
                        "project_id": prj['project_id'],
                        "username": new_mem_user,
                        "role": new_mem_role
                    })
                    if show_message(res, "106"):
                        st.session_state.show_add_member_form = False
                        load_members(prj['project_id'])
                        st.rerun()
                else:
                    st.error("Vui lòng nhập Username")

            if cancelled:
                st.session_state.show_add_member_form = False
                st.rerun()
        
        # Display members
        members = st.session_state.get("members_list", [])
        if not members:
            st.info("👥 Chưa có thành viên nào. Nhấn 'Làm mới' hoặc thêm thành viên mới.")
        else:
            for mem in members:
                with st.container(border=True):
                    col_user, col_role, col_action = st.columns([3, 2, 1])
                    
                    with col_user:
                        st.markdown(f"**👤 {mem.get('username')}**")
                        st.caption(f"ID: {mem.get('user_id')}")
                    
                    with col_role:
                        role = mem.get('role', 'MEMBER')
                        if role == 'PM':
                            st.success(f"🎯 {role}")
                        elif role == 'DEV':
                            st.info(f"💻 {role}")
                        else:
                            st.warning(f"👥 {role}")
                    
                    with col_action:
                        with st.popover("⚙️"):
                            new_role = st.selectbox(
                                "Đổi quyền",
                                ["PM", "DEV", "MEMBER"],
                                key=f"role_select_{mem.get('user_id')}"
                            )
                            if st.button("💾 Lưu", key=f"save_role_{mem.get('user_id')}", use_container_width=True):
                                res = send_request("update_member", st.session_state.session, 
                                                 {"project_id": prj['project_id'], 
                                                  "user_id": mem.get('user_id'), 
                                                  "role": new_role})
                                if show_message(res, "112"):
                                    load_members(prj['project_id'])
                                    st.rerun()
    
    # ============================
    # TAB 3: NHẬN XÉT
    # ============================
    with tab4:
        st.markdown("### 💬 Nhận xét & Thảo luận")
        
        if not st.session_state.selected_task:
            st.info("👈 Vui lòng chọn một task từ tab 'Công việc' để xem và thêm nhận xét")
        else:
            task = st.session_state.selected_task
            st.success(f"Task đang xem: *{task.get('task_name')}*")
            
            with st.form("comment_dedicated_form"):
                comment_content = st.text_area(
                    "Nhận xét của bạn",
                    placeholder="Viết nhận xét, đề xuất hoặc câu hỏi về task này...",
                    height=150
                )
                if st.form_submit_button("📤 Gửi nhận xét", use_container_width=True, type="primary"):
                    if comment_content:
                        res = send_request("comment_task", st.session_state.session, 
                                          {"task_id": task['task_id'], "comment": comment_content})
                        if show_message(res, "111"):
                            st.rerun()
                    else:
                        st.warning("Vui lòng nhập nội dung nhận xét!")