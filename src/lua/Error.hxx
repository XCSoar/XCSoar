// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

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
