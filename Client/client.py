import streamlit as st
import socket
import json

# ============================
# CONFIG SERVER
# ============================
SERVER_HOST = "172.31.245.233"
SERVER_PORT = 8080

# ============================
# STATUS CODE MAPPING
# ============================
STATUS_MAP = {
    # Auth
    "131": "Lỗi dữ liệu đăng nhập", "141": "Không có quyền truy cập", "151": "Không tìm thấy người dùng", "161": "Tài khoản bị khóa",
    "132": "Dữ liệu đăng ký không hợp lệ", "162": "Username đã tồn tại",
    # Project
    "133": "Session không hợp lệ", "143": "Không có quyền xem dự án", "153": "Không tìm thấy dự án",
    "135": "Thiếu thông tin tạo dự án", "165": "Tên dự án đã tồn tại",
    "136": "Thiếu thông tin thành viên", "146": "Chỉ Admin/PM mới có quyền thêm thành viên", "156": "User không tồn tại", "166": "User đã là thành viên",
    # Task
    "138": "Thiếu thông tin task", "148": "Chỉ PM mới được tạo task", "168": "Tên task bị trùng",
    "139": "Dữ liệu gán task lỗi", "149": "Chỉ PM mới được gán task", "159": "User không thuộc dự án này",
    "140": "Dữ liệu cập nhật lỗi", "150": "Chỉ thành viên dự án mới được cập nhật",
    "151_msg": "Chỉ PM hoặc người thực hiện mới được bình luận", # Trùng mã 151 của auth nên đặt tên khác
    # Server
    "501": "Server Auth lỗi", "503": "Server Project lỗi", "507": "Server Task lỗi"
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
                chunk = s.recv(4096).decode()
                if not chunk: break
                buffer += chunk
                if "\r\n" in buffer:
                    resp_str, _ = buffer.split("\r\n", 1)
                    return json.loads(resp_str)
    except Exception as e:
        return {"status": "error", "message": str(e)}
    return {"status": "error", "message": "No response from server"}

def show_message(res, success_code, success_msg):
    """Hàm hiển thị thông báo dựa trên status code"""
    status = str(res.get("status"))
    if status == success_code:
        st.success(success_msg)
        return True
    else:
        err_msg = STATUS_MAP.get(status, res.get("message", "Lỗi không xác định"))
        st.error(f"❌ Lỗi ({status}): {err_msg}")
        return False

# ============================
# SESSION STATE
# ============================
if "session" not in st.session_state: st.session_state.session = ""
if "username" not in st.session_state: st.session_state.username = ""
if "selected_project" not in st.session_state: st.session_state.selected_project = None

# ============================
# STREAMLIT UI
# ============================
st.title("🟦 Project Manager Professional")

# ----------------------------
# LOGIN / REGISTER
# ----------------------------
if st.session_state.session == "":
    st.subheader("🔐 Xác thực hệ thống")
    col1, col2 = st.columns(2)
    with col1: username = st.text_input("Username")
    with col2: password = st.text_input("Password", type="password")
    
    btn_col1, btn_col2 = st.columns(2)
    with btn_col1:
        if st.button("Đăng nhập", use_container_width=True):
            res = send_request("login", data={"username": username, "password": password})
            if res.get("status") == "101":
                st.session_state.session = res["data"]["session"]
                st.session_state.username = username
                st.success("✅ Chào mừng quay trở lại!")
                st.rerun()
            else:
                show_message(res, "101", "")
                
    with btn_col2:
        if st.button("Đăng ký tài khoản", use_container_width=True):
            res = send_request("register", data={"username": username, "password": password})
            show_message(res, "102", "✅ Đăng ký thành công! Mời bạn đăng nhập.")
    st.stop()

# ----------------------------
# LOGOUT & HEADER
# ----------------------------
col_user, col_logout = st.columns([3, 1])
col_user.write(f"👤 **User:** {st.session_state.username}")
if col_logout.button("🚪 Đăng xuất"):
    st.session_state.session = ""
    st.rerun()

st.divider()
tab1, tab2, tab3, tab4 = st.tabs(["📋 Dự án", "✅ Công việc", "👥 Thành viên", "💬 Nhận xét"])

# ============================
# TAB 1: DỰ ÁN (103, 104, 105)
# ============================
with tab1:
    st.header("📋 Quản lý dự án")
    
    if st.button("🔄 Làm mới danh sách"):
        res = send_request("list_projects", st.session_state.session)
        if res.get("status") == "103":
            projects = res["data"]["projects"]
            if not projects: st.info("Bạn chưa tham gia dự án nào")
            for p in projects:
                with st.container(border=True):
                    c1, c2, c3 = st.columns([3,2,1])
                    c1.write(f"**{p['name']}**")
                    c2.write(f"ID: `{p['id']}`")
                    if c3.button("Chọn", key=f"sel_{p['id']}"):
                        st.session_state.selected_project = p
                        st.rerun()
        else:
            show_message(res, "103", "")

    st.subheader("🔍 Tìm kiếm")
    skw = st.text_input("Nhập tên dự án...")
    if st.button("Tìm"):
        res = send_request("search_project", st.session_state.session, {"keyword": skw})
        if res.get("status") == "104":
            st.write(res["data"]["projects"])
        else: show_message(res, "104", "")

    st.subheader("➕ Tạo dự án")
    with st.form("create_prj"):
        pname = st.text_input("Tên dự án")
        pdesc = st.text_area("Mô tả")
        if st.form_submit_button("Xác nhận tạo"):
            res = send_request("create_project", st.session_state.session, {"name": pname, "description": pdesc})
            show_message(res, "105", "✅ Đã tạo dự án mới!")

# ============================
# TAB 2: CÔNG VIỆC (107, 108, 109, 110)
# ============================
with tab2:
    if not st.session_state.selected_project:
        st.warning("Vui lòng chọn dự án ở tab 'Dự án'")
    else:
        prj = st.session_state.selected_project
        st.info(f"📂 Đang xem: **{prj['name']}**")

        if st.button("🔄 Tải danh sách công việc"):
            res = send_request("list_tasks", st.session_state.session, {"project_id": prj['id']})
            if res.get("status") == "107":
                for t in res["data"]["tasks"]:
                    with st.expander(f"📌 {t['name']} ({t['status']})"):
                        st.write(f"ID: `{t['id']}` | Gán cho: {t.get('assignee','--')}")
                        new_s = st.selectbox("Cập nhật trạng thái", ["todo", "in_progress", "done", "blocked"], key=f"s_{t['id']}")
                        if st.button("Lưu trạng thái", key=f"btn_{t['id']}"):
                            res2 = send_request("update_task_status", st.session_state.session, {"task_id": t['id'], "status": new_s})
                            if show_message(res2, "110", "✅ Đã cập nhật!"): st.rerun()
            else: show_message(res, "107", "")

        st.subheader("➕ Tạo Task mới")
        with st.form("new_task"):
            tname = st.text_input("Tên công việc")
            tdesc = st.text_area("Mô tả")
            if st.form_submit_button("Thêm Task"):
                res = send_request("create_task", st.session_state.session, {"project_id": prj['id'], "name": tname, "description": tdesc})
                show_message(res, "108", "✅ Thêm công việc thành công!")

        st.subheader("👤 Gán nhân sự")
        with st.form("assign_task"):
            tid = st.text_input("ID công việc")
            tuser = st.text_input("Username người nhận")
            if st.form_submit_button("Gán việc"):
                res = send_request("assign_task", st.session_state.session, {"task_id": tid, "username": tuser})
                show_message(res, "109", f"✅ Đã gán task cho {tuser}")

# ============================
# TAB 3: THÀNH VIÊN (106)
# ============================
with tab3:
    if st.session_state.selected_project:
        prj = st.session_state.selected_project
        st.subheader("➕ Thêm thành viên vào nhóm")
        with st.form("add_mem"):
            mname = st.text_input("Username")
            mrole = st.selectbox("Vai trò", ["member", "admin", "viewer"])
            if st.form_submit_button("Mời vào dự án"):
                res = send_request("add_member", st.session_state.session, 
                                   {"project_id": prj['id'], "username": mname, "role": mrole})
                show_message(res, "106", f"✅ Đã thêm {mname} làm {mrole}")
    else:
        st.warning("Vui lòng chọn dự án")

# ============================
# TAB 4: NHẬN XÉT (111)
# ============================
with tab4:
    if st.session_state.selected_project:
        st.subheader("✍️ Gửi nhận xét")
        with st.form("comment_frm"):
            ctid = st.text_input("ID công việc")
            cmsg = st.text_area("Nội dung")
            if st.form_submit_button("Gửi"):
                res = send_request("comment_task", st.session_state.session, {"task_id": ctid, "comment": cmsg})
                show_message(res, "111", "✅ Đã gửi nhận xét!")
    else:
        st.warning("Vui lòng chọn dự án")

st.divider()
st.caption(f"Session: {st.session_state.session[:15]}... | UI v2.0")