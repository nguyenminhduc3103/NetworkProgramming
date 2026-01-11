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
    request = json.dumps({
        "action": action,
        "session": session,
        "data": data
    }, separators=(',', ':')) + "\r\n"

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.settimeout(20)
        s.connect((SERVER_HOST, SERVER_PORT))
        s.sendall(request.encode('utf-8'))

        buffer = b""
        while True:
            chunk = s.recv(4096)
            if not chunk:
                break
            buffer += chunk
            if b"\r\n" in buffer:
                resp_bytes = buffer.split(b"\r\n")[0]
                print(resp_bytes)
                print(" ")
                return json.loads(resp_bytes.decode("utf-8"))
    return {"status": "error", "message": "No response"}

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

def get_task_detail(session, task_id):
    """Lấy chi tiết task từ server"""
    req = {"action": "get_task_detail", "session": session, "data": {"task_id": task_id}}
    res = send_request(req["action"], req["session"], req["data"])
    return res

# ============================
# SESSION STATE 
# ============================
if "session" not in st.session_state: st.session_state.session = ""
if "username" not in st.session_state: st.session_state.username = ""
if "selected_project" not in st.session_state: st.session_state.selected_project = None
if "projects_list" not in st.session_state: st.session_state.projects_list = [] 
if "selected_task" not in st.session_state: st.session_state.selected_task = None
if "tasks_list" not in st.session_state: st.session_state.tasks_list = []
if "members_list" not in st.session_state: st.session_state.members_list = []
if "view_mode" not in st.session_state: st.session_state.view_mode = "list"
if "task_detail" not in st.session_state: st.session_state.task_detail = None

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
        if st.button("🔄 Đổi dự án", use_container_width=True):
            st.session_state.selected_project = None
            st.session_state.selected_task = None
            st.session_state.tasks_list = []
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
    
    # Search & Refresh - SỬA LẠI PHẦN NÀY
    col_search, col_btn1, col_btn2 = st.columns([3, 1, 1])
    with col_search:
        search_kw = st.text_input("🔍 Tìm kiếm dự án", placeholder="Nhập từ khóa...", label_visibility="collapsed")
    with col_btn1:
        # Bỏ st.write("##") và dùng button trực tiếp
        if st.button("🔍 Tìm", use_container_width=True):
            action = "search_project" if search_kw else "list_projects"
            data = {"keyword": search_kw} if search_kw else {}
            res = send_request(action, st.session_state.session, data)
            if res.get("status") in ["103", "104"]:
                st.session_state.projects_list = res.get("data", [])
    with col_btn2:
        # Bỏ st.write("##") và dùng button trực tiếp
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
                        # Tự động tải tasks khi chọn dự án
                        load_tasks(p['project_id'])
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
    
    tab1, tab2, tab3 = st.tabs(["📋 Công việc", "👥 Thành viên", "📄 Chi tiết Task"])
    
    # ============================
    # TAB 1: CÔNG VIỆC
    # ============================
    with tab1:
        col1, col3, col4 = st.columns([2,1,1])
        with col1:
            st.markdown("### Danh sách Task")
        with col3:
            if st.button("🔄 Làm mới", use_container_width=True):
                load_tasks(prj['project_id'])
                st.rerun()
        with col4:
            if st.button("➕ Task mới", use_container_width=True, type="primary"):
                st.session_state.show_create_task = True

        st.divider()

        # nếu chưa có state thì init
        if "show_create_task" not in st.session_state:
            st.session_state.show_create_task = False

        if st.session_state.show_create_task:
            with st.form("quick_create_task"):
                st.markdown("#### ➕ Tạo Task mới")

                new_t_name = st.text_input("Tên task")
                new_t_desc = st.text_area("Mô tả")

                submit = st.form_submit_button("✅ Tạo", use_container_width=True)

            if submit:
                res = send_request(
                    "create_task",
                    st.session_state.session,
                    {
                        "project_id": prj['project_id'],
                        "task_name": new_t_name,
                        "description": new_t_desc
                    }
                )

                if show_message(res, "108"):
                    st.session_state.show_create_task = False
                    load_tasks(prj['project_id'])
                    st.rerun()

        # Display tasks
        if not st.session_state.tasks_list:
            st.info("📭 Dự án chưa có task nào. Hãy tạo task đầu tiên!")
        else:
            for task in st.session_state.tasks_list:
                status = task.get('status', 'todo')
                status_info = TASK_STATUS.get(status, {"label": status, "color": "gray"})
                
                with st.container(border=True):
                    col_info, col_status, col_action = st.columns([4, 2, 1])
                    
                    with col_info:
                        st.markdown(f"*{task.get('task_name')}*")
                        st.caption(f"ID: {task.get('task_id')}")
                        if task.get('assigned_to'):
                            st.caption(f"👤 {task.get('assigned_to')}")
                    
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
                            # Tải chi tiết task từ server
                            detail_res = get_task_detail(st.session_state.session, task['task_id'])
                            print(detail_res)
                            if detail_res.get("status") == "112":  # Giả sử 113 là mã thành công
                                st.session_state.task_detail = detail_res.get("data", {})
                            st.rerun()
        
        # Task detail modal
        if st.session_state.selected_task:
            task = st.session_state.selected_task
            st.divider()
            
            with st.container(border=True):
                # Header
                col_title, col_close = st.columns([5, 1])
                with col_title:
                    st.markdown(f"### 📝 {task.get('name')}")
                with col_close:
                    if st.button("❌", key="close_task"):
                        st.session_state.selected_task = None
                        st.rerun()
                
                st.caption(f"Task ID: {task.get('task_id')}")
                
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
                    
                    if st.button("💾 Cập nhật trạng thái", use_container_width=True, key="update_status_btn"):
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
                    
                    if st.button("✅ Gán task", use_container_width=True, key="assign_task_btn"):
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
                    if st.form_submit_button("📤 Gửi nhận xét", use_container_width=True):
                        if comment_text:
                            res = send_request("comment_task", st.session_state.session, 
                                              {"task_id": task['task_id'], "comment": comment_text})
                            if show_message(res, "111"):
                                st.rerun()
                        else:
                            st.warning("Vui lòng nhập nội dung nhận xét!")
    
    # ============================
    # TAB 2: THÀNH VIÊN
    # ============================
    with tab2:
        col_header, col_refresh, col_add = st.columns([3, 1, 1])
        with col_header:
            st.markdown("### Danh sách thành viên")

        with col_refresh:
            if st.button("🔄 Làm mới", key="refresh_members", use_container_width=True):
                res = send_request("list_members", st.session_state.session, {"project_id": prj['project_id']})
                if res.get("status") == "269":
                    st.session_state.members_list = res.get("data", [])
                    st.toast("Đã cập nhật danh sách thành viên")

        with col_add:
            if st.button("➕ Thêm", key="add_member_btn", use_container_width=True, type="primary"):
                st.session_state.show_add_member = True

        st.divider()

        # init state
        if "show_add_member" not in st.session_state:
            st.session_state.show_add_member = False

        # Add member form
        if st.session_state.show_add_member:
            with st.form("add_member_form"):
                st.markdown("#### ➕ Thêm thành viên mới")

                new_mem_user = st.text_input("Username")
                new_mem_role = st.selectbox("Vai trò", ["MEMBER", "DEV", "PM"])

                col_submit, col_cancel = st.columns(2)

                submit = col_submit.form_submit_button("✅ Thêm", use_container_width=True)
                cancel = col_cancel.form_submit_button("❌ Hủy", use_container_width=True)

            if cancel:
                st.session_state.show_add_member = False
                st.rerun()

            if submit:
                if not new_mem_user:
                    st.error("Vui lòng nhập Username")
                else:
                    res = send_request(
                        "add_member",
                        st.session_state.session,
                        {
                            "project_id": prj['project_id'],
                            "username": new_mem_user,
                            "role": new_mem_role
                        }
                    )

                    if show_message(res, "106"):
                        # Refresh members list
                        res2 = send_request("list_members", st.session_state.session, {"project_id": prj['project_id']})
                        if res2.get("status") == "269":
                            st.session_state.members_list = res2.get("data", [])

                        st.session_state.show_add_member = False
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
                        st.markdown(f"👤 {mem.get('username')}")
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
                                    # Refresh members
                                    res2 = send_request("list_members", st.session_state.session, {"project_id": prj['project_id']})
                                    if res2.get("status") == "269":
                                        st.session_state.members_list = res2.get("data", [])
                                    st.rerun()
    
    # ============================
    # TAB 3: CHI TIẾT TASK
    # ============================
    with tab3:
        if not st.session_state.selected_task:
            st.info("👈 Vui lòng chọn một task từ tab 'Công việc' để xem chi tiết")
        else:
            # Lấy dữ liệu từ session_state
            detail = st.session_state.task_detail or {}
            print(detail)
            task = st.session_state.selected_task
            # 1. Header: Sử dụng 'task_name' từ response
            col_title, col_close = st.columns([5, 1])
            with col_title:
                task_display_name = task.get('task_name', 'Không tiêu đề')
                st.markdown(f"### 📝 {task_display_name}")
            with col_close:
                if st.button("❌ Đóng", key="close_detail"):
                    st.session_state.selected_task = None
                    st.session_state.task_detail = None
                    st.rerun()
            
            st.caption(f"Task ID: {detail.get('task_id')} | Project ID: {detail.get('project_id')}")
            st.divider()
            
            # 2. Thông tin chi tiết
            col_left, col_right = st.columns(2)
            
            with col_left:
                st.markdown("#### 📊 Thông tin chung")
                st.markdown(f"*Tên task:* {detail.get('task_name')}")
                st.markdown(f"*Mô tả:* {detail.get('description') or 'Chưa có mô tả'}")
                
                # Áp dụng màu sắc cho status nếu bạn có dict TASK_STATUS
                status_raw = detail.get('status', 'todo')
                st.markdown(f"*Trạng thái:* {status_raw.upper()}")
                
                # Hiển thị tên người thực hiện (assigned_user) thay vì ID
                st.markdown(f"*Người thực hiện:* {detail.get('assigned_user', 'Chưa gán')}")
                
            with col_right:
                st.markdown("#### 📅 Thông tin khác")
                # Vì server hiện tại chưa trả về created_at của task (chỉ có của comment), 
                # nên ta tạm để N/A hoặc bổ sung sau
                st.markdown(f"*Ngày tạo:* {detail.get('created_at', 'N/A')}")
                st.markdown(f"*Deadline:* {detail.get('deadline', 'Chưa có')}")
            
            st.divider()
            
            # 3. Comments section
            st.markdown("#### 💬 Nhận xét")
            comments = detail.get('comments', [])
            
            if not comments:
                st.info("Chưa có nhận xét nào cho task này")
            else:
                for comment in comments:
                    with st.container(border=True):
                        col_user, col_time = st.columns([3, 1.5])
                        with col_user:
                            st.markdown(f"👤 {comment.get('username')}")
                        with col_time:
                            # Map key 'created_at' từ JSON
                            st.caption(f"🕒 {comment.get('created_at')}")
                        st.markdown(comment.get('comment', ''))
            
            st.divider()
            
            # 4. Form thêm nhận xét
            with st.form("add_comment_detail", clear_on_submit=True):
                comment_text = st.text_area(
                    "Thêm nhận xét mới",
                    placeholder="Viết nhận xét, đề xuất hoặc câu hỏi về task này...",
                    height=100
                )
                if st.form_submit_button("📤 Gửi nhận xét", use_container_width=True, type="primary"):
                    if comment_text:
                        # Gửi request lên server
                        res = send_request("comment_task", st.session_state.session, 
                                        {"task_id": detail.get('task_id'), "comment": comment_text})
                        
                        # Giả sử "112" là code thành công của server C bạn vừa viết
                        if res and res.get("status") == "112":
                            st.success("Đã thêm nhận xét!")
                            # Refresh dữ liệu chi tiết task
                            detail_res = get_task_detail(st.session_state.session, detail.get('task_id'))
                            if detail_res and detail_res.get("status") == "112":
                                st.session_state.task_detail = detail_res.get("data", {})
                            st.rerun()
                        else:
                            st.error("Không thể gửi nhận xét")
                    else:
                        st.warning("Vui lòng nhập nội dung nhận xét!")