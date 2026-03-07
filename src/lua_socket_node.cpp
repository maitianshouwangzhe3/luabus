/*
** repository: https://github.com/trumanzhao/luabus.git
** trumanzhao, 2017-07-09, trumanzhao@foxmail.com
*/

#include "tools.h"
#include "var_int.h"
#include "lua_socket_node.h"

LUA_EXPORT_CLASS_BEGIN(lua_socket_node)
LUA_EXPORT_METHOD(send)
LUA_EXPORT_METHOD(async_send)
LUA_EXPORT_METHOD(send_forward)
LUA_EXPORT_METHOD(async_send_forward)
LUA_EXPORT_METHOD(close)
LUA_EXPORT_METHOD(set_send_buffer_size)
LUA_EXPORT_METHOD(set_recv_buffer_size)
LUA_EXPORT_METHOD(set_nodelay)
LUA_EXPORT_METHOD(set_protobuf)
LUA_EXPORT_METHOD(call)
LUA_EXPORT_METHOD(forward_target)
LUA_EXPORT_METHOD_AS(forward_by_group<msg_id::forward_master>, "forward_master")
LUA_EXPORT_METHOD_AS(forward_by_group<msg_id::forward_random>, "forward_random")
LUA_EXPORT_METHOD_AS(forward_by_group<msg_id::forward_broadcast>, "forward_broadcast")
LUA_EXPORT_METHOD(forward_hash)
LUA_EXPORT_METHOD(set_send_cache)
LUA_EXPORT_METHOD(set_recv_cache)
LUA_EXPORT_METHOD(set_timeout)
LUA_EXPORT_PROPERTY_AS(m_ip, "ip")
LUA_EXPORT_PROPERTY_READONLY_AS(m_token, "token")
LUA_EXPORT_CLASS_END()

lua_socket_node::lua_socket_node(uint32_t token, lua_State* L, std::shared_ptr<socket_mgr>& mgr, std::shared_ptr<lua_archiver>& ar, std::shared_ptr<socket_router> router)
    : m_token(token), m_lvm(L), m_mgr(mgr), m_archiver(ar), m_router(router) {
    m_mgr->get_remote_ip(m_token, m_ip);

    m_mgr->set_accept_callback(token, [this](uint32_t steam_token) {
        lua_guard g(m_lvm);
        auto stream = new lua_socket_node(steam_token, m_lvm, m_mgr, m_archiver, m_router);
        stream->set_protobuf(m_mgr->is_protobuf());
        lua_call_object_function(m_lvm, nullptr, this, "on_accept", std::tie(), stream);
    });

    m_mgr->set_connect_callback(token, [this](bool ok, const char* reason) {
        if (ok) {
            m_mgr->get_remote_ip(m_token, m_ip);
        }

        lua_guard g(m_lvm);
        lua_call_object_function(m_lvm, nullptr, this, "on_connect", std::tie(), ok ? "ok" : reason);
    });

    m_mgr->set_error_callback(token, [this](const char* err) {
        lua_guard g(m_lvm);
        lua_call_object_function(m_lvm, nullptr, this, "on_error", std::tie(), err);
    });

    m_mgr->set_package_callback(token, [this](char* data, size_t data_len) {
        // on_recv(data, data_len);
        on_dispatch_message(data, data_len);
    });
}

lua_socket_node::~lua_socket_node() {
	close();
}

int lua_socket_node::send(lua_State* L) {
    int top = lua_gettop(L);
    if (top < 1)
        return 0;

    void* data = nullptr;
    size_t data_len = 0;
    data = m_archiver->save(&data_len, L, 1, top);
    if (data == nullptr)
        return 0;

    BYTE msg_id_data[MAX_VARINT_SIZE];
    size_t msg_id_len = encode_u64(msg_id_data, sizeof(msg_id_data), (char)msg_id::remote_call);
    sendv_item items[] = {{msg_id_data, msg_id_len}, {data, data_len}};
    m_mgr->sendv(m_token, items, _countof(items));
    lua_pushinteger(L, data_len);
    return 1;
}

int lua_socket_node::async_send(lua_State* L) {
    int top = lua_gettop(L);
    if (top < 1)
        return 0;

    void* data = nullptr;
    size_t data_len = 0;
    data = m_archiver->save(&data_len, L, 1, top);
    if (data == nullptr)
        return 0;

    BYTE msg_id_data[MAX_VARINT_SIZE];
    size_t msg_id_len = encode_u64(msg_id_data, sizeof(msg_id_data), (char)msg_id::remote_call);
    sendv_item items[] = {{msg_id_data, msg_id_len}, {data, data_len}};
    m_mgr->async_sendv(m_token, items, _countof(items));
    lua_pushinteger(L, data_len);
    return 1;
}

int lua_socket_node::send_forward(lua_State* L) {
    int top = lua_gettop(L);
    if (top < 2)
        return 0;

    size_t data_len = 0;
    void* data = nullptr;
    data_len = luaL_checkinteger(L, 2);
    data = (void*)luaL_checklstring(L, 1, &data_len);
    if (!data)
        return 0;

    m_mgr->send(m_token, data, data_len);
    lua_pushinteger(L, data_len);
    return 1;
}

int lua_socket_node::async_send_forward(lua_State* L) {
    int top = lua_gettop(L);
    if (top < 2)
        return 0;

    size_t data_len = 0;
    void* data = nullptr;
    
    data_len = luaL_checkinteger(L, 2);
    data = (void*)luaL_checklstring(L, 1, &data_len);
    if (data == nullptr)
        return 0;

    m_mgr->async_send(m_token, data, data_len);
    lua_pushinteger(L, data_len);
    return 1;
}

void lua_socket_node::close() {
    if (m_token != 0) {
        m_mgr->close(m_token);
        m_token = 0;
    }
}

void lua_socket_node::on_recv(char* data, size_t data_len) {
    lua_guard g(m_lvm);

    if (!lua_get_object_function(m_lvm, this, "on_recv"))
        return;

    int param_count = m_archiver->load(m_lvm, data, data_len);
    if (param_count == 0)
        return;

    lua_call_function(m_lvm, nullptr, param_count, 0);
}

void lua_socket_node::on_dispatch_message(char* data, size_t data_len) {
    if (m_protobuf) {
        on_pb_recv(data, data_len);
        return;
    }

    uint64_t msg = 0;
    size_t len = decode_u64(&msg, (BYTE*)data, data_len);
    if (len == 0)
        return;
    data += len;
    data_len -= len;

    switch ((msg_id)msg)
    {
    case msg_id::remote_call:
        on_recv(data, data_len);
        break;

    case msg_id::forward_target:
        m_router->forward_target(data, data_len);
        break;

    case msg_id::forward_random:
        m_router->forward_random(data, data_len);
        break;

    case msg_id::forward_master:
        m_router->forward_master(data, data_len);
        break;

    case msg_id::forward_hash:
        m_router->forward_hash(data, data_len);
        break;

    case msg_id::forward_broadcast:
        m_router->forward_broadcast(data, data_len);
        break;

    default:
        break;
    }
}

void lua_socket_node::on_pb_recv(void* data, size_t data_len) {
    auto stream = m_mgr->find_node(m_token);
    if (!stream) {
        return;
    }

    lua_guard g(m_lvm);
    if (!lua_get_object_function(m_lvm, this, "on_pb_recv")) {
        lua_call_object_function(m_lvm, nullptr, this, "on_error", std::tie(), "get on_pb_recv error");
        return;
    }

    const char* msg_data = nullptr;
    int msg_len = 0;
    if (!lua_call_function(m_lvm, nullptr, std::tie(msg_data, msg_len), m_token, data, data_len)) {
        lua_call_object_function(m_lvm, nullptr, this, "on_error", std::tie(), "dispatch_pb_message error");
        return;
    }

    if (!msg_data || msg_len <= 0)
        return;

    m_mgr->async_send(m_token, msg_data, msg_len);
}

int lua_socket_node::call(lua_State* L)
{
    int top = lua_gettop(L);
    if (top < 1)
        return 0;

    BYTE msg_id_data[MAX_VARINT_SIZE];
    size_t msg_id_len = encode_u64(msg_id_data, sizeof(msg_id_data), (char)msg_id::remote_call);

    size_t data_len = 0;
    void* data = nullptr;
    
    if (m_mgr->is_protobuf()) {
        data_len = luaL_checkinteger(L, 2);
        data = (void*)luaL_checklstring(L, 1, &data_len);
        if (data == nullptr)
            return 0;
    } else {
        data = m_archiver->save(&data_len, L, 1, top);
        if (data == nullptr)
            return 0;
    }

    sendv_item items[] = {{msg_id_data, msg_id_len}, {data, data_len}};
    m_mgr->sendv(m_token, items, _countof(items));
    lua_pushinteger(L, data_len);
    return 1;
}

int lua_socket_node::forward_target(lua_State* L)
{
    int top = lua_gettop(L);
    if (top < 2)
        return 0;

    BYTE msg_id_data[MAX_VARINT_SIZE];
    size_t msg_id_len = encode_u64(msg_id_data, sizeof(msg_id_data), (char)msg_id::forward_target);

    uint32_t service_id = (uint32_t)lua_tointeger(L, 1);
    BYTE svr_id_data[MAX_VARINT_SIZE];
    size_t svr_id_len = encode_u64(msg_id_data, sizeof(msg_id_data), service_id);

    size_t data_len = 0;
    void* data = nullptr;
    
    if (m_mgr->is_protobuf()) {
        data_len = luaL_checkinteger(L, 2);
        data = (void*)luaL_checklstring(L, 1, &data_len);
        if (data == nullptr)
            return 0;
    } else {
        data = m_archiver->save(&data_len, L, 2, top);
        if (data == nullptr)
            return 0;
    }

    sendv_item items[] = {{msg_id_data, msg_id_len}, {svr_id_data, svr_id_len}, {data, data_len}};
    m_mgr->sendv(m_token, items, _countof(items));
    lua_pushinteger(L, data_len);
    return 1;
}

template <msg_id forward_method>
int lua_socket_node::forward_by_group(lua_State* L)
{
    int top = lua_gettop(L);
    if (top < 2)
        return 0;

    static_assert(forward_method == msg_id::forward_master || forward_method == msg_id::forward_random ||
        forward_method == msg_id::forward_broadcast, "Unexpected forward method !");

    BYTE msg_id_data[MAX_VARINT_SIZE];
    size_t msg_id_len = encode_u64(msg_id_data, sizeof(msg_id_data), (char)forward_method);

    uint8_t group_id = (uint8_t)lua_tointeger(L, 1);
    BYTE group_id_data[MAX_VARINT_SIZE];
    size_t group_id_len = encode_u64(group_id_data, sizeof(group_id_data), group_id);

    size_t data_len = 0;
    void* data = nullptr;
    
    if (m_mgr->is_protobuf()) {
        data_len = luaL_checkinteger(L, 2);
        data = (void*)luaL_checklstring(L, 1, &data_len);
        if (data == nullptr)
            return 0;
    } else {
        data = m_archiver->save(&data_len, L, 2, top);
        if (data == nullptr)
            return 0;
    }

    sendv_item items[] = {{msg_id_data, msg_id_len}, {group_id_data, group_id_len}, {data, data_len}};
    m_mgr->sendv(m_token, items, _countof(items));
    lua_pushinteger(L, data_len);
    return 1;
}

// BKDR Hash
static uint32_t string_hash(const char* str)
{
    uint32_t seed = 131; // 31 131 1313 13131 131313 etc..
    uint32_t hash = 0;
    while (*str)
    {
        hash = hash * seed + (*str++);
    }
    return (hash & 0x7FFFFFFF);
}

int lua_socket_node::forward_hash(lua_State* L)
{
    int top = lua_gettop(L);
    if (top < 3)
        return 0;

    BYTE msg_id_data[MAX_VARINT_SIZE];
    size_t msg_id_len = encode_u64(msg_id_data, sizeof(msg_id_data), (char)msg_id::forward_hash);

    uint8_t group_id = (uint8_t)lua_tointeger(L, 1);
    BYTE group_id_data[MAX_VARINT_SIZE];
    size_t group_id_len = encode_u64(group_id_data, sizeof(group_id_data), group_id);

    int type = lua_type(L, 2);
    uint32_t hash_key = 0;
    if (type == LUA_TNUMBER)
    {
        hash_key = (uint32_t)lua_tointeger(L, 2);
    }
    else if (type == LUA_TSTRING)
    {
        const char* str = lua_tostring(L, 2);
        if (str == nullptr)
            return 0;
        hash_key = string_hash(str);
    }
    else
    {
        // unexpected hash key
        return 0;
    }

    BYTE hash_data[MAX_VARINT_SIZE];
    size_t hash_len = encode_u64(hash_data, sizeof(hash_data), hash_key);

    size_t data_len = 0;
    void* data = nullptr;
    
    if (m_mgr->is_protobuf()) {
        data_len = luaL_checkinteger(L, 2);
        data = (void*)luaL_checklstring(L, 1, &data_len);
        if (data == nullptr)
            return 0;
    } else {
        data = m_archiver->save(&data_len, L, 2, top);
        if (data == nullptr)
            return 0;
    }

    sendv_item items[] = {{msg_id_data, msg_id_len}, {group_id_data, group_id_len}, {hash_data, hash_len}, {data, data_len}};
    m_mgr->sendv(m_token, items, _countof(items));
    lua_pushinteger(L, data_len);
    return 1;
}

// void lua_socket_node::on_recv(char* data, size_t data_len)
// {
//     uint64_t msg = 0;
//     size_t len = decode_u64(&msg, (BYTE*)data, data_len);
//     if (len == 0)
//         return;
//     data += len;
//     data_len -= len;

//     switch ((msg_id)msg)
//     {
//     case msg_id::remote_call: {
//         lua_guard g(m_lvm);

//         if (!lua_get_object_function(m_lvm, this, "on_recv"))
//             return;

//         int param_count = m_archiver->load(m_lvm, data, data_len);
//         if (param_count == 0)
//             return;

//         lua_call_function(m_lvm, nullptr, param_count, 0);
//     }
//         break;

//     case msg_id::forward_target:
//         m_router->forward_target(data, data_len);
//         break;

//     case msg_id::forward_random:
//         m_router->forward_random(data, data_len);
//         break;

//     case msg_id::forward_master:
//         m_router->forward_master(data, data_len);
//         break;

//     case msg_id::forward_hash:
//         m_router->forward_hash(data, data_len);
//         break;

//     case msg_id::forward_broadcast:
//         m_router->forward_broadcast(data, data_len);
//         break;

//     default:
//         break;
//     }
// }
