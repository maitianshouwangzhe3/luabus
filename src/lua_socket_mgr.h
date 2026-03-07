/*
** repository: https://github.com/trumanzhao/luabus.git
** trumanzhao, 2017-07-09, trumanzhao@foxmail.com
*/

#pragma once
#include <memory>
#include "socket_mgr.h"
#include "luna.h"
#include "lua_archiver.h"

class socket_router;

struct lua_socket_mgr final {
public:
    ~lua_socket_mgr();
    bool setup(lua_State* L, int max_fd);
    int wait(int ms) { return m_mgr->wait(ms); }
    int listen(lua_State* L);
    int connect(lua_State* L);
    void set_package_size(size_t size);
    void set_lz_threshold(size_t size);
    int router(lua_State* L);
    void set_master(uint8_t group_idx, uint32_t token);
    void set_protobuf(bool flag) { m_mgr->set_protobuf(flag); }
    bool is_protobuf() const { return m_mgr->is_protobuf(); }

private:
    lua_State* m_lvm = nullptr;
    std::shared_ptr<socket_mgr> m_mgr;
    std::shared_ptr<lua_archiver> m_archiver;
    std::shared_ptr<socket_router> m_router;

public:
    DECLARE_LUA_CLASS(lua_socket_mgr)
};

