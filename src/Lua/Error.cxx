/*
 * Copyright 2016-2019 Max Kellermann <max.kellermann@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Error.hxx"
#include "Util/Exception.hxx"
#include "Util/Compiler.h"

extern "C" {
#include <lua.h>
}

#include <assert.h>

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
