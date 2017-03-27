/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "InputEvent.hpp"
#include "Util.hxx"
#include "Error.hpp"
#include "Catch.hpp"
#include "Associate.hpp"
#include "Persistent.hpp"
#include "Input/InputQueue.hpp"
#include "Input/InputKeys.hpp"
#include "Compiler.h"
#include "Util/StringAPI.hxx"
#include "Util/ConvertString.hpp"
#include "Util.hxx"
#include "Interface.hpp"

#include <map>
#include "Util/tstring.hpp"

extern "C" {
#include <lauxlib.h>
}

class LuaInputEvent;

template<typename T>
class LuaEventRegistry {
  typedef std::multimap<T, LuaInputEvent*> Map;
  Map data;

public:
  bool Fire(T code);
  bool Clear(const T code);

  bool HasHandler(T code) const;

  bool Insert(T code, LuaInputEvent* event) {
    data.insert(std::pair<T, LuaInputEvent*>(code, event));
    return true;
  }

  bool Remove(const LuaInputEvent* code) {
    for (auto it = data.begin(); it != data.end(); ++it) {
      if (it->second == code) {
        data.erase(it);
        return true;
      }
    }
    return false;
  }

};

static LuaEventRegistry<unsigned> event_store_enum;
static LuaEventRegistry<const tstring> event_store_gesture;
static LuaEventRegistry<unsigned> event_store_key;

static const char* event_enum_names[] = {
  "nil",
#include "InputEvents_Char2GCE.cpp"
#include "InputEvents_Char2NE.cpp"
  nullptr
};

class LuaInputEvent final {
  lua_State *const L;

public:
  static constexpr const char *registry_table = "xcsoar.input_events";

  explicit LuaInputEvent(lua_State *_l, int callback_idx):L(_l) {
    auto d = (LuaInputEvent **)lua_newuserdata(L, sizeof(LuaInputEvent **));
    *d = this;

    luaL_setmetatable(L, "xcsoar.input_event");

    Register(callback_idx, -1);

    /* 'this' is left on stack */
  }

  ~LuaInputEvent() {
    Lua::DisassociatePointer(L, registry_table, (void *)this);
  }

  void AttachEnum(unsigned code) {
    if ((code < GCE_COUNT + NE_COUNT) && event_store_enum.Insert(code, this)) {
      Lua::AddPersistent(L, this);
    }
  }

  void AttachGesture(const TCHAR* gesture) {
    if (event_store_gesture.Insert(gesture, this)) {
      Lua::AddPersistent(L, this);
    }
  }

  void AttachKey(unsigned code) {
    if (event_store_key.Insert(code, this)) {
      Lua::AddPersistent(L, this);
    }
  }

  void Cancel() {
    event_store_enum.Remove(this);
    event_store_gesture.Remove(this);
    event_store_key.Remove(this);
    RemovePersistent();
  }

  void OnEvent() {
    if (PushTable()) {
      lua_getfield(L, -1, "callback");
      lua_getfield(L, -2, "input_event");
      if (lua_pcall(L, 1, 0, 0))
        Lua::ThrowError(L, Lua::PopError(L));

      Lua::CheckPersistent(L);
    }
  }

  void RemovePersistent() {
    Lua::RemovePersistent(L, this);
  }

private:
  void Register(int callback_idx, int this_idx) {
    lua_newtable(L);
    --this_idx;

    Lua::SetField(L, -2, "callback", Lua::StackIndex(callback_idx));
    Lua::SetField(L, -2, "input_event", Lua::StackIndex(this_idx));

    Lua::AssociatePointer(L, registry_table, (void *)this, -1);
    lua_pop(L, 1); // pop table
  }

  bool PushTable() {
    Lua::LookupPointer(L, registry_table, (void *)this);
    if (lua_isnil(L, -1)) {
      lua_pop(L, 1); // pop table
      return false;
    }

    return true;
  }

  gcc_pure
  static LuaInputEvent &Check(lua_State *L, int idx) {
    auto d = (LuaInputEvent **)luaL_checkudata(L, idx, "xcsoar.input_event");
    luaL_argcheck(L, d != nullptr, idx, "`xcsoar.input_event' expected");
    return **d;
  }

public:
  static int l_new(lua_State *L) {
    if (lua_gettop(L) != 2)
      return luaL_error(L, "Invalid parameters");

    if (!lua_isfunction(L, 2))
      return luaL_argerror(L, 1, "function expected");

    const char *name = lua_tostring(L, 1);
    if (name == nullptr)
      return luaL_error(L, "Invalid parameters");

    else if (StringIsEqual(name, "gesture_", 8)) {
      // scan for gesture
      const UTF8ToWideConverter gesture(name+8);
      if (gesture.IsValid()) {
        auto *input_event = new LuaInputEvent(L, 2);
        input_event->AttachGesture(gesture);
        return 1;
      }
    } else if (StringIsEqual(name, "key_", 4)) {
        // scan for key code
        const UTF8ToWideConverter keycode(name+4);
        if (keycode.IsValid()) {
          const unsigned code = ParseKeyCode(keycode);
          auto *input_event = new LuaInputEvent(L, 2);
          input_event->AttachKey(code);
          return 1;
        }
    } else {
      // scan for other enums
      const unsigned code = luaL_checkoption(L, 1, NULL, event_enum_names);
      if (code) {
        auto *input_event = new LuaInputEvent(L, 2);
        input_event->AttachEnum(code-1);
        return 1;
      }
    }
    return luaL_argerror(L, 1, "invalid event");
  }

  static int l_gc(lua_State *L) {
    auto &input_event = Check(L, 1);
    input_event.Cancel();
    delete &input_event;
    return 0;
  }

  static int l_cancel(lua_State *L) {
    auto &input_event = Check(L, 1);
    input_event.Cancel();
    return 0;
  }

  static int l_clear(lua_State *L) {

    if (lua_gettop(L) != 1)
      return luaL_error(L, "Invalid parameters");

    const char *name = lua_tostring(L, 1);
    if (name == nullptr)
      return luaL_error(L, "Invalid parameters");

    else if (StringIsEqual(name, "gesture_", 8)) {
      const UTF8ToWideConverter gesture(name+8);
      if (gesture.IsValid()) {
        event_store_gesture.Clear(tstring(gesture));
        return 1;
      }
    } else if (StringIsEqual(name, "key_", 4)) {
      // scan for key code
      const UTF8ToWideConverter keycode(name+4);
      if (keycode.IsValid()) {
        const unsigned code = ParseKeyCode(keycode);
        event_store_key.Clear(code);
        return 1;
      }
    } else {
      // scan for other enums
      const unsigned code = luaL_checkoption(L, 1, NULL, event_enum_names);
      if (code) {
        event_store_enum.Clear(code);
        return 1;
      }
    }
    return luaL_argerror(L, 1, "invalid event");
  }

};

static constexpr struct luaL_Reg input_event_funcs[] = {
  {"new", LuaInputEvent::l_new},
  {"clear", LuaInputEvent::l_clear},
  {nullptr, nullptr}
};

static constexpr struct luaL_Reg input_event_methods[] = {
  {"cancel", LuaInputEvent::l_cancel},
  {nullptr, nullptr}
};


static void
CreateInputEventMetatable(lua_State *L)
{
  luaL_newmetatable(L, "xcsoar.input_event");

  /* metatable.__index = input_event_methods */
  luaL_newlib(L, input_event_methods);
  lua_setfield(L, -2, "__index");

  /* metatable.__gc = l_gc */
  lua_pushcfunction(L, LuaInputEvent::l_gc);
  lua_setfield(L, -2, "__gc");

  /* pop metatable */
  lua_pop(L, 1);
}

void
Lua::InitInputEvent(lua_State *L)
{
#ifndef NDEBUG
  const int old_top = lua_gettop(L);
#endif

  lua_getglobal(L, "xcsoar");

  luaL_newlib(L, input_event_funcs); // create 'input_event'

  lua_setfield(L, -2, "input_event"); // xcsoar.input_event = input_event
  lua_pop(L, 1); // pop global "xcsoar"

  assert(lua_gettop(L) == old_top);

  CreateInputEventMetatable(L);

  assert(lua_gettop(L) == old_top);

  Lua::InitAssociatePointer(L, LuaInputEvent::registry_table);

  assert(lua_gettop(L) == old_top);
}


template<typename T>
bool LuaEventRegistry<T>::Fire(T event) {
  bool retval = false;
  auto r = data.equal_range(event);
  for (auto it = r.first; it != r.second; ) {
    // save used here because OnEvent may cause removal (cancel) and hence invalidate the iterator
    auto save = it; ++save;
    it->second->OnEvent();
    retval = true;
    it = save;
  }
  return retval;
}

template<typename T>
bool LuaEventRegistry<T>::Clear(const T event) {
  bool retval = false;
  auto r = data.equal_range(event);
  for (auto it = r.first; it != r.second; ) {
    // save used here because clearing invalidates the iterator
    auto save = it; ++save;
    it->second->RemovePersistent();
    data.erase(it);
    retval = true;
    it = save;
  }
  return retval;
}

template<typename T>
bool LuaEventRegistry<T>::HasHandler(T event) const {
  return data.count(event)>0;
}

bool Lua::FireGlideComputerEvent(unsigned event) {
  return event_store_enum.Fire(event);
}

bool Lua::FireNMEAEvent(unsigned event) {
  return event_store_enum.Fire(event + GCE_COUNT);
}

bool Lua::FireGesture(const TCHAR* gesture) {
  return event_store_gesture.Fire(gesture);
}

bool Lua::IsGesture(const TCHAR* gesture) {
  return event_store_gesture.HasHandler(gesture);
}

bool Lua::FireKey(unsigned key) {
  return event_store_key.Fire(key);
}
