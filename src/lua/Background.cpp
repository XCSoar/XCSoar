// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Background.hpp"
#include "Util.hxx"
#include "Persistent.hpp"
#include "util/DeleteDisposer.hxx"
#include "util/IntrusiveList.hxx"

extern "C" {
#include <lua.h>
}

#include <cassert>

static constexpr char background_lua_key[] = "xcsoar.background";

class BackgroundLua final
  : public IntrusiveListHook<IntrusiveHookMode::NORMAL>
{
  Lua::StatePtr state;

public:
  explicit BackgroundLua(Lua::StatePtr &&_state) noexcept
    :state(std::move(_state))
  {
    Lua::SetRegistry(state.get(), background_lua_key, Lua::LightUserData(this));
    Lua::SetPersistentCallback(state.get(), PersistentCallback);
  }

  ~BackgroundLua() noexcept {
    Lua::SetRegistry(state.get(), background_lua_key, nullptr);
  }

private:
  void PersistentCallback() noexcept;

  static void PersistentCallback(lua_State *L) noexcept {
    auto *b = (BackgroundLua *)
      Lua::GetRegistryLightUserData(L, background_lua_key);
    if (b != nullptr) {
      assert(b->state.get() == L);
      b->PersistentCallback();
    }
  }
};

namespace Lua {

static IntrusiveList<BackgroundLua> background;

}

inline void
BackgroundLua::PersistentCallback() noexcept
{
  Lua::SetRegistry(state.get(), background_lua_key, nullptr);
  Lua::background.erase_and_dispose(Lua::background.iterator_to(*this),
                                    DeleteDisposer());
}

void
Lua::AddBackground(StatePtr &&state) noexcept
{
  auto *b = new BackgroundLua(std::move(state));
  background.push_front(*b);
}

void
Lua::StopAllBackground() noexcept
{
  background.clear_and_dispose(DeleteDisposer());
}
