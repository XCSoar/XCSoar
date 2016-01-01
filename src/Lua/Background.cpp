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

#include "Background.hpp"
#include "Util.hpp"
#include "Persistent.hpp"
#include "Util/DeleteDisposer.hxx"

extern "C" {
#include <lua.h>
}

#include <boost/intrusive/list.hpp>

#include <assert.h>

static constexpr char background_lua_key[] = "xcsoar.background";

class BackgroundLua final
  : public boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>>
{
  lua_State *const L;

public:
  explicit BackgroundLua(lua_State *_L):L(_L) {
    Lua::SetRegistry(L, background_lua_key, Lua::LightUserData(this));
    Lua::SetPersistentCallback(L, PersistentCallback);
  }

  ~BackgroundLua() {
    Lua::SetRegistry(L, background_lua_key, nullptr);
    lua_close(L);
  }

private:
  void PersistentCallback();

  static void PersistentCallback(lua_State *L) {
    auto *b = (BackgroundLua *)
      Lua::GetRegistryLightUserData(L, background_lua_key);
    if (b != nullptr) {
      assert(b->L == L);
      b->PersistentCallback();
    }
  }
};

namespace Lua {
  static boost::intrusive::list<BackgroundLua,
                                boost::intrusive::constant_time_size<false>> background;
}

void
BackgroundLua::PersistentCallback()
{
  Lua::SetRegistry(L, background_lua_key, nullptr);
  Lua::background.erase_and_dispose(Lua::background.iterator_to(*this),
                                    DeleteDisposer());
}

void
Lua::AddBackground(lua_State *L)
{
  auto *b = new BackgroundLua(L);
  background.push_front(*b);
}

void
Lua::StopAllBackground()
{
  background.clear_and_dispose(DeleteDisposer());
}
