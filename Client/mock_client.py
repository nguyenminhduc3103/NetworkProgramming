import streamlit as st
import json

st.title("🟦 Client Project Manager (MOCK VERSION)")

# ====================================
# MOCK DATA
# ====================================

MOCK_PROJECTS = [
    {"id": 1, "name": "AI Research"},
    {"id": 2, "name": "Web Development"},
    {"id": 3, "name": "Mobile App"},
]

def mock_send_cmd(obj):
    action = obj.get("action")
    session = obj.get("session")
    data = obj.get("data", {})

    # ---------------- LOGIN ----------------
    if action == "login":
        if data["username"] == "teo" and data["password"] == "123":
            return {
                "status": "code(ok)",
                "message": "Login successful",
                "data": {"session": "ABC123"}
            }
        else:
            return {"status": "error", "message": "Sai thông tin đăng nhập"}

    # ---------------- LIST PROJECTS ----------------
    if action == "list_projects":
        return {
            "status": "code(ok)",
            "data": {"projects": MOCK_PROJECTS}
        }

    # ---------------- SEARCH PROJECT ----------------
    if action == "search_project":
        kw = data.get("keyword", "").lower()
        results = [p for p in MOCK_PROJECTS if kw in p["name"].lower()]
        return {
            "status": "code(ok)",
            "data": {"projects": results}
        }

    return {"status": "error", "message": "Unknown action"}

# ====================================
# LOGIN UI
# ====================================

if "session" not in st.session_state:
    st.session_state.session = ""

st.subheader("🔐 Đăng nhập (Mock)")

username = st.text_input("Username")
password = st.text_input("Password", type="password")

if st.button("Login (mock)"):
    req = {
        "action": "login",
        "session": "",
        "data": {"username": username, "password": password}
    }
    res = mock_send_cmd(req)
    st.write(res)

    if res.get("status") == "code(ok)":
        st.session_state.session = res["data"]["session"]
        st.success("Đăng nhập thành công!")

st.write("---")

if st.session_state.session == "":
    st.warning("Hãy đăng nhập trước (mock)")
    st.stop()


# ====================================
# LIST PROJECTS
# ====================================

st.header("📋 Danh sách dự án (Mock)")

if st.button("Lấy danh sách dự án"):
    req = {"action": "list_projects", "session": st.session_state.session, "data": {}}
    res = mock_send_cmd(req)

    if res.get("status") == "code(ok)":
        for p in res["data"]["projects"]:
            st.write(f"**ID:** {p['id']} — **Tên:** {p['name']}")
    else:
        st.error(res.get("message"))

st.write("---")

# ====================================
# SEARCH PROJECT
# ====================================

st.header("🔍 Tìm kiếm dự án (Mock)")

kw = st.text_input("Từ khóa tìm kiếm")

if st.button("Tìm"):
    req = {
        "action": "search_project",
        "session": st.session_state.session,
        "data": {"keyword": kw}
    }
    res = mock_send_cmd(req)

    if res.get("status") == "code(ok)":
        for p in res["data"]["projects"]:
            st.write(f"**ID:** {p['id']} — **Tên:** {p['name']}")
    else:
        st.error(res.get("message"))
