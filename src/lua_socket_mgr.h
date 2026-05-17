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
    void set_package_type(int type) { m_package_type = (package_type)type; }
    int get_package_type() const { return (int)m_package_type; }

private:
    lua_State* m_lvm = nullptr;
    std::shared_ptr<socket_mgr> m_mgr;
    std::shared_ptr<lua_archiver> m_archiver;
    std::shared_ptr<socket_router> m_router;
    package_type m_package_type = package_type::lua_message;
public:
    DECLARE_LUA_CLASS(lua_socket_mgr)
};

