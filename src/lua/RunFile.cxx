// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "RunFile.hxx"
#include "Error.hxx"
#include "system/OpenPathFile.hpp"
#include "system/Path.hpp"
#include "util/ScopeExit.hxx"

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

extern "C" {
#include <lauxlib.h>
}

/**
 * Skip an optional UTF-8 BOM and a leading shebang line, matching
 * luaL_loadfile() preprocessing.
 */
static std::string_view
SkipLuaFileHeader(std::string_view data) noexcept
{
	static constexpr char utf8_bom[] = {
		'\xef', '\xbb', '\xbf'
	};
	if (data.size() >= 3 &&
	    data[0] == utf8_bom[0] &&
	    data[1] == utf8_bom[1] &&
	    data[2] == utf8_bom[2])
		data.remove_prefix(3);

	if (!data.empty() && data.front() == '#') {
		/* luaL_loadfile() skips the shebang and then inserts a
		   newline so Lua line numbers still match the file. */
		const auto nl = data.find('\n');
		if (nl == std::string_view::npos)
			data = {};
		else
			data.remove_prefix(nl);
	}

	return data;
}

void
Lua::RunFile(lua_State *L, Path path)
{
	FILE *file = OpenPathFile(path, "rb");
	if (file == nullptr)
		throw std::runtime_error("Failed to open Lua file");

	AtScopeExit(file) { fclose(file); };

	if (fseek(file, 0, SEEK_END) != 0)
		throw std::runtime_error("Failed to seek Lua file");

	const long size = ftell(file);
	if (size < 0)
		throw std::runtime_error("Failed to size Lua file");

	if (fseek(file, 0, SEEK_SET) != 0)
		throw std::runtime_error("Failed to rewind Lua file");

	std::vector<char> buffer(static_cast<std::size_t>(size));
	if (size > 0 &&
	    fread(buffer.data(), 1, buffer.size(), file) != buffer.size())
		throw std::runtime_error("Failed to read Lua file");

	const std::string_view chunk =
		SkipLuaFileHeader(std::string_view(buffer.data(),
						   buffer.size()));

	const std::string chunk_name = "@" + std::string(path.c_str());
	if (luaL_loadbuffer(L, chunk.data(), chunk.size(),
			    chunk_name.c_str()) ||
	    lua_pcall(L, 0, 0, 0))
		throw PopError(L);
}
