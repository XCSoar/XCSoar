// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Catch.hpp"
#include "Util.hxx"

extern "C" {
#include <lua.h>
}

#include <utility>

static constexpr char catch_callback[] = "xcsoar.catch_callback";

void
Lua::SetCatchCallback(lua_State *L, CatchCallback callback)
{
  Lua::SetRegistry(L, catch_callback,
                   Lua::LightUserData((void *)callback));
}

[[gnu::pure]]
static Lua::CatchCallback
GetCatchCallback(lua_State *L)
{
  return (Lua::CatchCallback)
    Lua::GetRegistryLightUserData(L, catch_callback);
}

void
Lua::ThrowError(lua_State *L, Error &&error)
{
  auto callback = GetCatchCallback(L);
  if (callback != nullptr)
    callback(std::move(error));
}
