// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

struct lua_State;
class Path;

namespace Lua {

/**
 * Load, compile and run the specified file.
 *
 * Throws std::runtime_error on error.
 */
void
RunFile(lua_State *L, Path path);

}
