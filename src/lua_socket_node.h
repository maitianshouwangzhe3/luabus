/*
** repository: https://github.com/trumanzhao/luabus.git
** trumanzhao, 2017-07-09, trumanzhao@foxmail.com
*/

#pragma once
#include <memory>

#include "socket_mgr.h"
#include "luna.h"
#include "lua_archiver.h"
#include "socket_router.h"

struct lua_socket_node final {
    lua_socket_node(uint32_t token, lua_State* L, std::shared_ptr<socket_mgr>& mgr, std::shared_ptr<lua_archiver>& ar, std::shared_ptr<socket_router> router);
    ~lua_socket_node();

    int send(lua_State* L);
    int async_send(lua_State* L);
    int send_forward(lua_State* L);
    int async_send_forward(lua_State* L);
    void close();
    void set_send_buffer_size(size_t size) { m_mgr->set_send_buffer_size(m_token, size); }
    void set_recv_buffer_size(size_t size) { m_mgr->set_recv_buffer_size(m_token, size); }
    void set_nodelay(bool flag) { m_mgr->set_nodelay(m_token, flag); }

    int call(lua_State* L);
    int forward(lua_State* L);
    int forward_target(lua_State* L);

    template <msg_id forward_method>
    int forward_by_group(lua_State* L);

    int forward_hash(lua_State* L);
    void set_send_cache(size_t size) { m_mgr->set_send_cache(m_token, size); }
    void set_recv_cache(size_t size) { m_mgr->set_recv_cache(m_token, size); }
    void set_timeout(int duration) { m_mgr->set_timeout(m_token, duration); }

    void set_protobuf(bool flag) { m_protobuf = flag; }

private:
    void on_dispatch_message(char* data, size_t data_len);
    void on_recv(char* data, size_t data_len);
    void on_pb_recv(void* data, size_t data_len);
    
    uint32_t m_token = 0;
    lua_State* m_lvm = nullptr;
    bool m_protobuf = false;
    std::string m_ip;
    std::shared_ptr<socket_mgr> m_mgr;
    std::shared_ptr<lua_archiver> m_archiver;
    std::shared_ptr<socket_router> m_router;

public:
    DECLARE_LUA_CLASS(lua_socket_node)
};

