// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Key.hpp"

#include "Input/InputKeys.hpp"
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"
#include "util/StringAPI.hxx"

#ifdef ENABLE_SDL
#include <SDL_events.h>
#endif

extern "C" {
#include <lauxlib.h>
}

/**
 * Queue one key press (down, then up).
 *
 * shortcut: returns before the UI handles the key, and a count is
 * queued at once. Upgrade when a script must wait for the handler.
 */
static int
l_key_press(lua_State *L)
{
  const int argc = lua_gettop(L);
  if (argc < 1 || argc > 2)
    return luaL_error(L, "Invalid parameters");

  const char *name = luaL_checkstring(L, 1);
  unsigned count = 1;
  if (argc == 2) {
    const lua_Integer raw = luaL_checkinteger(L, 2);
    if (raw < 1 || raw > 10000)
      return luaL_argerror(L, 2, "count must be 1..10000");
    count = static_cast<unsigned>(raw);
  }

  if (StringIsEqual(name, "ENTER"))
    name = "RETURN";
  else if (StringIsEqual(name, "ESC"))
    name = "ESCAPE";

  const unsigned code = ParseKeyCode(name);
  if (code == 0)
    return luaL_argerror(L, 1, "unknown key");

#if !defined(ENABLE_SDL) && !defined(ANDROID) && !defined(USE_POLL_EVENT)
  (void)count;
  (void)code;
  return luaL_error(L, "key injection is not available");
#else
#if !defined(ENABLE_SDL)
  if (UI::event_queue == nullptr)
    return luaL_error(L, "UI not ready");
#endif

  for (unsigned i = 0; i < count; ++i) {
#ifdef ENABLE_SDL
    SDL_Event down{};
    down.type = SDL_KEYDOWN;
    down.key.state = SDL_PRESSED;
    down.key.keysym.sym = static_cast<SDL_Keycode>(code);
    SDL_Event up{};
    up.type = SDL_KEYUP;
    up.key.state = SDL_RELEASED;
    up.key.keysym.sym = static_cast<SDL_Keycode>(code);
    if (::SDL_PushEvent(&down) < 0 || ::SDL_PushEvent(&up) < 0)
      return luaL_error(L, "key injection failed");
#else
    UI::Event down{};
    down.type = UI::Event::KEY_DOWN;
    down.param = code;
    UI::Event up{};
    up.type = UI::Event::KEY_UP;
    up.param = code;
    UI::event_queue->Inject(down);
    UI::event_queue->Inject(up);
#endif
  }
  return 0;
#endif
}

static constexpr struct luaL_Reg key_funcs[] = {
  {"press", l_key_press},
  {nullptr, nullptr}
};

void
Lua::InitKey(lua_State *L)
{
  lua_getglobal(L, "xcsoar");
  lua_newtable(L);
  luaL_setfuncs(L, key_funcs, 0);
  lua_setfield(L, -2, "key");
  lua_pop(L, 1);
}
