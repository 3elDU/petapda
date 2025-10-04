#include "screen.h"
#include <gfx/screen.h>
#include <lauxlib.h>

static int set(lua_State *L)
{
    uint16_t x = luaL_checkinteger(L, 1);
    uint16_t y = luaL_checkinteger(L, 2);
    color_t color = (color_t)luaL_checkinteger(L, 3);

    luaL_argcheck(L, x > 0 && x <= EINK_WIDTH, 1, "out of bounds");
    luaL_argcheck(L, y > 0 & y <= EINK_HEIGHT, 2, "out of bounds");

    screen_set(x - 1, y - 1, color);
    return 0;
}
static int get(lua_State *L)
{
    uint16_t x = luaL_checkinteger(L, 1);
    uint16_t y = luaL_checkinteger(L, 2);

    luaL_argcheck(L, x > 0 && x <= EINK_WIDTH, 1, "out of bounds");
    luaL_argcheck(L, y > 0 && y <= EINK_HEIGHT, 2, "out of bounds");

    lua_pushinteger(L, screen_get(x - 1, y - 1));
    return 1;
}

static int fill(lua_State *L)
{
    color_t color = luaL_checkinteger(L, 1);
    screen_fill(color);

    return 0;
}

static int fill_rect(lua_State *L)
{
    uint16_t x = luaL_checkinteger(L, 1);
    uint16_t y = luaL_checkinteger(L, 2);
    uint16_t x1 = luaL_checkinteger(L, 3);
    uint16_t y1 = luaL_checkinteger(L, 4);
    color_t color = (color_t)luaL_checkinteger(L, 5);

    luaL_argcheck(L, x > 0 && x <= EINK_WIDTH, 1, "out of bounds");
    luaL_argcheck(L, y > 0 && y <= EINK_HEIGHT, 2, "out of bounds");
    luaL_argcheck(L, x1 > 0 && x1 <= EINK_WIDTH, 3, "out of bounds");
    luaL_argcheck(L, y1 > 0 && y1 <= EINK_HEIGHT, 4, "out of bounds");

    screen_fill_rect(
        (box_t){x - 1, y - 1, x1 - 1, y1 - 1},
        color);
}

static int outline_rect(lua_State *L)
{
    uint16_t x = luaL_checkinteger(L, 1);
    uint16_t y = luaL_checkinteger(L, 2);
    uint16_t x1 = luaL_checkinteger(L, 3);
    uint16_t y1 = luaL_checkinteger(L, 4);
    color_t color = (color_t)luaL_checkinteger(L, 5);

    luaL_argcheck(L, x > 0 && x <= EINK_WIDTH, 1, "out of bounds");
    luaL_argcheck(L, y > 0 && y <= EINK_HEIGHT, 2, "out of bounds");
    luaL_argcheck(L, x1 > 0 && x1 <= EINK_WIDTH, 3, "out of bounds");
    luaL_argcheck(L, y1 > 0 && y1 <= EINK_HEIGHT, 4, "out of bounds");

    screen_outline_rect(
        (box_t){x - 1, y - 1, x1 - 1, y1 - 1},
        color);
}

static int draw_line(lua_State *L)
{
    uint16_t x = luaL_checkinteger(L, 1);
    uint16_t y = luaL_checkinteger(L, 2);
    uint16_t x1 = luaL_checkinteger(L, 3);
    uint16_t y1 = luaL_checkinteger(L, 4);
    color_t color = (color_t)luaL_checkinteger(L, 5);

    luaL_argcheck(L, x > 0 && x <= EINK_WIDTH, 1, "out of bounds");
    luaL_argcheck(L, y > 0 && y <= EINK_HEIGHT, 2, "out of bounds");
    luaL_argcheck(L, x1 > 0 && x1 <= EINK_WIDTH, 3, "out of bounds");
    luaL_argcheck(L, y1 > 0 && y1 <= EINK_HEIGHT, 4, "out of bounds");

    screen_draw_line(
        (box_t){x - 1, y - 1, x1 - 1, y1 - 1},
        color);
}

static int draw_char(lua_State *L)
{
    uint16_t x = luaL_checkinteger(L, 1);
    uint16_t y = luaL_checkinteger(L, 2);
    char ch = luaL_checkstring(L, 3)[0];
    color_t color = (color_t)luaL_checkinteger(L, 4);

    luaL_argcheck(L, x > 0 && x <= EINK_WIDTH, 1, "out of bounds");
    luaL_argcheck(L, y > 0 && y <= EINK_HEIGHT, 2, "out of bounds");

    box_size_t box = screen_draw_char(x - 1, y - 1, ch, color);

    lua_pushinteger(L, box.w);
    lua_pushinteger(L, box.h);
    return 2;
}

static int measure_char(lua_State *L)
{
    const char ch = luaL_checkstring(L, 1)[0];

    box_size_t box = screen_measure_char(ch);

    lua_pushinteger(L, box.w);
    lua_pushinteger(L, box.h);
    return 2;
}

static int draw_text(lua_State *L)
{
    uint16_t x = luaL_checkinteger(L, 1);
    uint16_t y = luaL_checkinteger(L, 2);
    const char *str = luaL_checkstring(L, 3);
    color_t color = (color_t)luaL_checkinteger(L, 4);
    uint16_t width = 0;

    luaL_argcheck(L, x > 0 && x <= EINK_WIDTH, 1, "out of bounds");
    luaL_argcheck(L, y > 0 && y <= EINK_HEIGHT, 2, "out of bounds");

    if (lua_gettop(L) == 5)
    {
        width = luaL_checkinteger(L, 5);
        luaL_argcheck(L, width <= EINK_WIDTH, 5, "out of bounds");
    }

    box_size_t box;

    if (width)
    {
        box = screen_draw_text_fit(x, y, str, color, width);
    }
    else
    {
        box = screen_draw_text(x - 1, y - 1, str, color);
    }

    lua_pushinteger(L, box.w);
    lua_pushinteger(L, box.h);
    return 2;
}

static int measure_text(lua_State *L)
{
    const char *str = luaL_checkstring(L, 1);
    uint16_t width = 0;

    if (lua_gettop(L) == 2)
    {
        width = luaL_checkinteger(L, 2);
        luaL_argcheck(L, width < EINK_WIDTH, 2, "out of bounds");
    }

    box_size_t box = width ? screen_measure_text_fit(str, width) : screen_measure_text(str);

    lua_pushinteger(L, box.w);
    lua_pushinteger(L, box.h);
    return 2;
}

static int refresh(lua_State *L)
{
    screen_full_refresh();
    return 0;
}
static int fast_refresh(lua_State *L)
{
    screen_fast_refresh();
    return 0;
}
static int partial_refresh(lua_State *L)
{
    uint16_t x, y, x1, y1;
    bool area = false;

    if (lua_gettop(L) == 4)
    {
        area = true;

        x = luaL_checkinteger(L, 1);
        y = luaL_checkinteger(L, 2);
        x1 = luaL_checkinteger(L, 3);
        y1 = luaL_checkinteger(L, 4);

        luaL_argcheck(L, x > 0 && x <= EINK_WIDTH, 1, "out of bounds");
        luaL_argcheck(L, y > 0 && y <= EINK_HEIGHT, 2, "out of bounds");
        luaL_argcheck(L, x1 > 0 && x1 <= EINK_WIDTH, 3, "out of bounds");
        luaL_argcheck(L, y1 > 0 && y1 <= EINK_HEIGHT, 4, "out of bounds");
    }

    if (area)
    {
        screen_partial_refresh_area((box_t){x - 1, y - 1, x1 - 1, y1 - 1});
    }
    else
    {
        screen_partial_refresh();
    }
}

static const luaL_Reg screenlib[] = {
    {"set", set},
    {"get", get},
    {"fill", fill},
    {"fill_rect", fill_rect},
    {"outline_rect", outline_rect},
    {"draw_line", draw_line},
    {"draw_char", draw_char},
    {"draw_text", draw_text},
    {"measure_char", measure_char},
    {"measure_text", measure_text},
    {"refresh", refresh},
    {"fast_refresh", fast_refresh},
    {"partial_refresh", partial_refresh},
    {NULL, NULL},
};

static int luaopen_screen(lua_State *L)
{
    luaL_newlib(L, screenlib);

    lua_pushinteger(L, EINK_WIDTH);
    lua_setfield(L, -2, "width");

    lua_pushinteger(L, EINK_HEIGHT);
    lua_setfield(L, -2, "height");

    return 1;
}

static int luaopen_colors(lua_State *L)
{
    lua_createtable(L, 0, 2);

    lua_pushinteger(L, C_BLACK);
    lua_setfield(L, -2, "black");

    lua_pushinteger(L, C_WHITE);
    lua_setfield(L, -2, "white");

    return 1;
}

void screen_lua_loadlib(lua_State *L)
{
    luaL_requiref(L, "screen", luaopen_screen, 1);
    luaL_requiref(L, "colors", luaopen_colors, 1);
    lua_pop(L, 2);
}