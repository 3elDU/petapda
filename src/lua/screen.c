#include "screen.h"
#include <gfx/screen.h>
#include <lauxlib.h>

static int fill(lua_State *L)
{
    screen_clear();
    return 0;
}

static int l_screen_set_cursor(lua_State *L)
{
    uint16_t x = (uint16_t)luaL_checkinteger(L, 1) - 1;
    uint16_t y = (uint16_t)luaL_checkinteger(L, 2) - 1;

    luaL_argcheck(L, x < SCREEN_COLUMNS, 1, "out of bounds");
    luaL_argcheck(L, y < SCREEN_LINES, 2, "out of bounds");

    screen_move_cursor(x, y);

    return 0;
}

static int refresh(lua_State *L)
{
    screen_render();
    return 0;
}

static int set_color(lua_State *L)
{
    uint16_t bg = luaL_checkinteger(L, 1);
    uint16_t fg = luaL_checkinteger(L, 2);
    screen_set_color(bg, fg);
}

static int draw_text(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    screen_print_text(text);

    return 0;
}

static int l_screen_get_width(lua_State *L)
{
    lua_pushinteger(L, SCREEN_COLUMNS);
    return 1;
}
static int l_screen_get_height(lua_State *L)
{
    lua_pushinteger(L, SCREEN_LINES);
    return 1;
}

static const luaL_Reg screenlib[] = {
    {"fill", fill},
    {"set_cursor", l_screen_set_cursor},
    {"set_color", set_color},
    {"draw_text", draw_text},
    {"refresh", refresh},
    {"get_width", l_screen_get_width},
    {"get_height", l_screen_get_height},
    {NULL, NULL},
};

static int luaopen_screen(lua_State *L)
{
    luaL_newlib(L, screenlib);
    return 1;
}

void screen_lua_loadlib(lua_State *L)
{
    luaL_requiref(L, "screen", luaopen_screen, 1);
    lua_pop(L, 1);
}