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

#ifndef LUA_ERROR_HXX
#define LUA_ERROR_HXX

#include <stdexcept>

struct lua_State;

namespace Lua {

class Error : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

/**
 * After a failed call to lua_pcall(), load and pop the Lua error
 * from the stack and store it in an #Error instance.
 */
Error
PopError(lua_State *L);

/**
 * Pushes a representation of the given C++ exception on the stack.
 */
void
Push(lua_State *L, std::exception_ptr e) noexcept;

/**
 * Raise a Lua error (using lua_error()) based on the given C++
 * exception.  This function never returns because lua_error() uses
 * longjmp().
 *
 * Note that this function cannot be `noexcept`, because this would
 * break _Unwind_RaiseException() which is used by Lua to raise
 * errors.
 */
[[noreturn]]
void
Raise(lua_State *L, std::exception_ptr e);

/**
 * Wrapper for Raise() which uses std::current_exception.  As a
 * special case, this supports Lua errors caught by a "catch(...)" and
 * rethrows them as-is.
 */
[[noreturn]]
void
RaiseCurrent(lua_State *L);

}

#endif
