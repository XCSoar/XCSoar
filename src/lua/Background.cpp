// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Background.hpp"
#include "Util.hxx"
#include "Persistent.hpp"
#include "util/DeleteDisposer.hxx"

extern "C" {
#include <lua.h>
}

#include <boost/intrusive/list.hpp>

#include <cassert>

static constexpr char background_lua_key[] = "xcsoar.background";

class BackgroundLua final
  : public boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>>
{
  Lua::StatePtr state;

public:
  explicit BackgroundLua(Lua::StatePtr &&_state):state(std::move(_state)) {
    Lua::SetRegistry(state.get(), background_lua_key, Lua::LightUserData(this));
    Lua::SetPersistentCallback(state.get(), PersistentCallback);
  }

  ~BackgroundLua() {
    Lua::SetRegistry(state.get(), background_lua_key, nullptr);
  }

private:
  void PersistentCallback();

  static void PersistentCallback(lua_State *L) {
    auto *b = (BackgroundLua *)
      Lua::GetRegistryLightUserData(L, background_lua_key);
    if (b != nullptr) {
      assert(b->state.get() == L);
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
  Lua::SetRegistry(state.get(), background_lua_key, nullptr);
  Lua::background.erase_and_dispose(Lua::background.iterator_to(*this),
                                    DeleteDisposer());
}

void
Lua::AddBackground(StatePtr &&state)
{
  auto *b = new BackgroundLua(std::move(state));
  background.push_front(*b);
}

void
Lua::StopAllBackground()
{
  background.clear_and_dispose(DeleteDisposer());
}
