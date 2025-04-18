// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Path;

namespace Lua {

/**
 * Load, compile and run the specified file.  If the script is
 * "persistent" as determined by Lua::IsPersistent(), move it to
 * background it using Lua::AddBackground().
 *
 * Throws std::runtime_error on error.
 */
void
StartFile(Path path);

}
