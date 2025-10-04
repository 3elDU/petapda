#include "joystick.h"
#include <dev/joystick.h>
#include <lauxlib.h>

static int read(lua_State *L)
{
    joy_state_t j = joystick_read();

    lua_pushnumber(L, j.x);
    lua_pushnumber(L, j.y);
    lua_pushboolean(L, j.pressed);
    return 3;
}
static int get_x(lua_State *L)
{
    joy_state_t j = joystick_read();

    lua_pushnumber(L, j.x);
    return 1;
}
static int get_y(lua_State *L)
{
    joy_state_t j = joystick_read();

    lua_pushnumber(L, j.y);
    return 1;
}
static int pressed(lua_State *L)
{
    joy_state_t j = joystick_read();

    lua_pushboolean(L, j.pressed);
    return 1;
}

static const luaL_Reg joysticklib[] = {
    {"read", read},
    {"pressed", pressed},
    {"get_x", get_x},
    {"get_y", get_y},
    {NULL, NULL},
};

static int luaopen_joystick(lua_State *L)
{
    luaL_newlib(L, joysticklib);
    return 1;
}

void joystick_lua_loadlib(lua_State *L)
{
    luaL_requiref(L, "joystick", luaopen_joystick, 1);
    lua_pop(L, 1);
}