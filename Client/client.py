import streamlit as st
import socket
import json

# ============================
# CONFIG SERVER
# ============================
SERVER_HOST = "127.0.0.1"
SERVER_PORT = 8080

# Cập nhật mapping dựa trên log thực tế của bạn
STATUS_MAP = {
    "101": "Đăng nhập thành công",
    "102": "Đăng ký thành công",
    "103": "Lấy danh sách dự án thành công",
    "104": "Tìm kiếm thành công",
    "105": "Tạo dự án thành công",
    "106": "Thêm thành viên thành công",
    "107": "Lấy danh sách task thành công",
    "108": "Tạo task thành công",
    "110": "Cập nhật task thành công",
    "111": "Gửi nhận xét thành công",
    "165": "Tên dự án đã tồn tại",
    "156": "Không tìm thấy người dùng",
    "509": "Gán Task thất bại",
    "512": "Lỗi hệ thống (Server Error)"
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
    
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(5)
            s.connect((SERVER_HOST, SERVER_PORT))
            s.sendall(request.encode())
            
            buffer = ""
            while True:
                chunk = s.recv(4096).decode('utf-8')
                if not chunk: break
                buffer += chunk
                if "\r\n" in buffer:
                    resp_str = buffer.split("\r\n")[0]
                    return json.loads(resp_str)
    except Exception as e:
        return {"status": "error", "message": str(e)}
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

# ============================
# SESSION STATE (Quan trọng để không mất dữ liệu)
# ============================
if "session" not in st.session_state: st.session_state.session = ""
if "username" not in st.session_state: st.session_state.username = ""
if "selected_project" not in st.session_state: st.session_state.selected_project = None
if "projects_list" not in st.session_state: st.session_state.projects_list = [] # Lưu danh sách project tại đây
if "selected_task" not in st.session_state: st.session_state.selected_task = None
if "tasks_list" not in st.session_state: st.session_state.tasks_list = []

# ============================
# AUTHENTICATION UI
# ============================
if not st.session_state.session:
    st.subheader("🔐 Xác thực hệ thống")
    user_input = st.text_input("Username")
    pass_input = st.text_input("Password", type="password")
    
    col1, col2 = st.columns(2)
    if col1.button("Đăng nhập", use_container_width=True):
        res = send_request("login", data={"username": user_input, "password": pass_input})
        if res.get("status") == "101":
            st.session_state.session = res["data"]["session"]
            st.session_state.username = user_input
            st.rerun()
        else: show_message(res, "101")
            
    if col2.button("Đăng ký", use_container_width=True):
        res = send_request("register", data={"username": user_input, "password": pass_input})
        show_message(res, "102")
    st.stop()

# ============================
# MAIN INTERFACE
# ============================
st.sidebar.title(f"👤 {st.session_state.username}")
if st.sidebar.button("Đăng xuất"):
    st.session_state.session = ""
    st.rerun()

if st.session_state.selected_project:
    st.sidebar.success(f"Dự án: **{st.session_state.selected_project['project_name']}**")

tab1, tab2, tab3, tab4 = st.tabs(["📂 Dự án", "📋 Công việc", "👥 Thành viên", "💬 Nhận xét"])

# ============================
# TAB 1: DỰ ÁN
# ============================
with tab1:
    col_a, col_b = st.columns([2, 1])
    with col_a:
        search_kw = st.text_input("Tìm kiếm dự án...")
    with col_b:
        st.write("##") # Căn chỉnh
        if st.button("Tìm kiếm / Làm mới"):
            action = "search_project" if search_kw else "list_projects"
            data = {"keyword": search_kw} if search_kw else {}
            res = send_request(action, st.session_state.session, data)
            if res.get("status") in ["103", "104"]:
                st.session_state.projects_list = res.get("data", [])
            else: show_message(res, "103")

    # Hiển thị danh sách dự án từ session_state
    st.divider()
    if not st.session_state.projects_list:
        st.info("Chưa có dữ liệu dự án. Hãy nhấn 'Làm mới'.")
    
    for p in st.session_state.projects_list:
        # Sử dụng đúng key từ log của bạn: project_id, project_name
        p_id = p.get('project_id')
        p_name = p.get('project_name')
        
        with st.container(border=True):
            c1, c2, c3 = st.columns([3, 2, 1])
            c1.write(f"**{p_name}**")
            c2.write(f"ID: `{p_id}`")
            if c3.button("Chọn", key=f"btn_sel_{p_id}"):
                # Lưu thông tin project đã chọn vào session
                st.session_state.selected_project = p
                st.rerun()

    st.subheader("➕ Tạo dự án mới")
    with st.expander("Mở form tạo dự án"):
        pname = st.text_input("Tên dự án mới")
        pdesc = st.text_area("Mô tả dự án")
        if st.button("Xác nhận tạo"):
            res = send_request("create_project", st.session_state.session, {"name": pname, "description": pdesc})
            show_message(res, "105")

# ============================
# TAB 2: QUẢN LÝ CÔNG VIỆC
# ============================
with tab2:
    if not st.session_state.selected_project:
        st.warning("⚠️ Vui lòng chọn dự án ở tab 'Dự án' trước!")
    else:
        prj = st.session_state.selected_project
        st.subheader(f"📋 Danh sách Task: {prj['project_name']}")

        # 1. Nút làm mới danh sách Task
        if st.button("🔄 Tải lại danh sách Task"):
            res = send_request("list_tasks", st.session_state.session, {"project_id": prj['project_id']})
            if res.get("status") == "107":
                st.session_state.tasks_list = res["data"].get("tasks", [])
                st.toast("Đã cập nhật danh sách task!")
            else:
                show_message(res, "107")

        # 2. Hiển thị danh sách Task để chọn
        if not st.session_state.tasks_list:
            st.info("Dự án này chưa có task nào hoặc bạn chưa nhấn 'Tải lại'.")
        else:
            for t in st.session_state.tasks_list:
                t_id = t.get('task_id')
                t_name = t.get('name', f"Task #{t_id}")
                t_status = t.get('status', 'N/A')
                
                # Tạo khung hiển thị task
                with st.container(border=True):
                    c1, c2, c3 = st.columns([3, 1, 1])
                    c1.write(f"**{t_name}** (ID: `{t_id}`)")
                    c2.info(f"Trạng thái: {t_status}")
                    if c3.button("Chọn Task", key=f"sel_task_{t_id}"):
                        st.session_state.selected_task = t
                        st.rerun()

        st.divider()

        # 3. Khu vực xử lý Task đã chọn
        if st.session_state.selected_task:
            task = st.session_state.selected_task
            st.success(f"🎯 Đang xử lý Task: **{task.get('name')}** (ID: {task.get('task_id')})")
            
            # Chia cột cho các hành động
            action_col1, action_col2 = st.columns(2)

            with action_col1:
                st.markdown("#### 👤 Gán nhân sự & Trạng thái")
                # Form cập nhật trạng thái
                new_status = st.selectbox("Đổi trạng thái", 
                                          ["todo", "in_progress", "done", "blocked"],
                                          key="status_select")
                if st.button("Cập nhật trạng thái"):
                    res = send_request("update_task_status", st.session_state.session, 
                                      {"task_id": task['task_id'], "status": new_status})
                    show_message(res, "110")

                # Form gán user
                assign_user = st.text_input("Username người nhận", placeholder="Nhập username...")
                if st.button("Xác nhận gán việc"):
                    res = send_request("assign_task", st.session_state.session, 
                                      {"task_id": task['task_id'], "username": assign_user})
                    show_message(res, "109")

            with action_col2:
                st.markdown("#### 💬 Nhận xét (Comment)")
                comment_text = st.text_area("Nội dung nhận xét", placeholder="Viết gì đó...")
                if st.button("Gửi nhận xét"):
                    if comment_text:
                        res = send_request("comment_task", st.session_state.session, 
                                          {"task_id": task['task_id'], "comment": comment_text})
                        if show_message(res, "111"):
                            st.session_state.comment_text = "" # Clear text
                    else:
                        st.warning("Vui lòng nhập nội dung!")

            if st.button("❌ Bỏ chọn Task"):
                st.session_state.selected_task = None
                st.rerun()
        
        st.divider()
        # 4. Form tạo task mới (luôn hiển thị cuối tab)
        with st.expander("➕ Tạo Task mới cho dự án này"):
            with st.form("new_task_form"):
                new_t_name = st.text_input("Tên Task")
                new_t_desc = st.text_area("Mô tả Task")
                if st.form_submit_button("Tạo Task"):
                    res = send_request("create_task", st.session_state.session, 
                                      {"project_id": prj['project_id'], "name": new_t_name, "description": new_t_desc})
                    show_message(res, "108")

# ============================
# TAB 4: NHẬN XÉT (Sửa theo log 111)
# ============================
with tab4:
    if st.session_state.selected_project:
        if st.session_state.selected_task:
            st.subheader("✍️ Gửi nhận xét vào Task")
            with st.form("comment_form"):
                comment_content = st.text_area("Nội dung nhận xét")
                if st.form_submit_button("Gửi Comment"):
                    res = send_request("comment_task", st.session_state.session, 
                                    {"task_id": st.session_state.selected_task['task_id'], "comment": comment_content})
                    show_message(res, "111")
    else:
        st.warning("Vui lòng chọn dự án")