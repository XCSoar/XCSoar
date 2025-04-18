// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "Error.hxx"
#include "util/Exception.hxx"
#include "util/Compiler.h"

extern "C" {
#include <lua.h>
}

#include <cassert>

namespace Lua {

Error
PopError(lua_State *L)
{
  Error e(lua_tostring(L, -1));
  lua_pop(L, 1);
  return e;
}

void
Push(lua_State *L, std::exception_ptr e) noexcept
{
	assert(e);

	lua_pushstring(L, GetFullMessage(e).c_str());
}

void
Raise(lua_State *L, std::exception_ptr e)
{
	Push(L, std::move(e));
	lua_error(L);

	/* this is unreachable because lua_error() never returns, but
	   the C header doesn't declare it that way */
	gcc_unreachable();
}

void
RaiseCurrent(lua_State *L)
{
	auto e = std::current_exception();
	if (e)
		Raise(L, std::move(e));
	else
		throw;
}

} // namespace Lua
