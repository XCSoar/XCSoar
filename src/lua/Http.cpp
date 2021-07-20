/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Http.hpp"
#include "Assert.hxx"
#include "Util.hxx"
#include "Class.hxx"
#include "Error.hxx"
#include "net/http/Easy.hxx"
#include "net/http/Adapter.hxx"
#include "net/http/Handler.hxx"

extern "C" {
#include <lauxlib.h>
}

#include <map>
#include <string>

class LuaHttpRequest final : CurlResponseHandler {
  CurlEasy easy;

  CurlResponseHandlerAdapter adapter;

  int status;
  std::multimap<std::string, std::string> response_headers;

  std::string response_body;

  std::exception_ptr error;

public:
  explicit LuaHttpRequest(const char *url)
    :easy(url), adapter(*this)
  {
    adapter.Install(easy);
  }

private:
  int Perform(lua_State *L);

public:
  static int l_new(lua_State *L);
  static int l_perform(lua_State *L);

private:
  /* virtual methods from class CurlResponseHandler */
  void OnHeaders(unsigned _status,
                 std::multimap<std::string, std::string> &&_headers) override {
    status = _status;
    response_headers = std::move(_headers);
  }

  void OnData(ConstBuffer<void> data) override {
    // TODO size check
    response_body.append((const char *)data.data, data.size);
  }

  void OnEnd() override {
  }

  void OnError(std::exception_ptr e) noexcept override {
    error = std::move(e);
  }
};

static constexpr struct luaL_Reg http_request_funcs[] = {
  {"new", LuaHttpRequest::l_new},
  {nullptr, nullptr}
};

static constexpr struct luaL_Reg http_request_methods[] = {
  {"perform", LuaHttpRequest::l_perform},
  {nullptr, nullptr}
};

static constexpr char lua_http_request_class[] = "http.Request";
using LuaHttpRequestClass = Lua::Class<LuaHttpRequest, lua_http_request_class>;

int
LuaHttpRequest::l_new(lua_State *L)
{
  if (lua_gettop(L) != 2)
    return luaL_error(L, "Invalid parameters");

  if (!lua_isstring(L, 2))
    luaL_argerror(L, 2, "URL expected");

  LuaHttpRequestClass::New(L, lua_tostring(L, 2));
  return 1;
}

int
LuaHttpRequest::Perform(lua_State *L)
{
  using namespace Lua;

  easy.Perform();

  // TODO: yield to the main event loop until the request is done

  if (error)
    Lua::Raise(L, std::move(error));

  lua_newtable(L);
  SetTable(L, RelativeStackIndex{-1}, "status", status);

  lua_pushstring(L, "headers");
  lua_newtable(L);
  for (const auto &i : response_headers)
    SetTable(L, RelativeStackIndex{-1}, i.first, i.second);
  lua_settable(L, -3);

  SetTable(L, RelativeStackIndex{-1}, "body", response_body);

  return 1;
}

int
LuaHttpRequest::l_perform(lua_State *L)
{
  auto &request = LuaHttpRequestClass::Cast(L, 1);
  return request.Perform(L);
}

static void
CreateHttpRequestMetatable(lua_State *L)
{
  LuaHttpRequestClass::Register(L);

  /* metatable.__index = http_request_methods */
  luaL_newlib(L, http_request_methods);
  lua_setfield(L, -2, "__index");

  /* pop metatable */
  lua_pop(L, 1);
}

void
Lua::InitHttp(lua_State *L)
{
  const Lua::ScopeCheckStack check_stack(L);

  lua_getglobal(L, "xcsoar");

  /* create the "http" namespace */
  lua_newtable(L);

  luaL_newlib(L, http_request_funcs); // create 'request'
  lua_setfield(L, -2, "Request"); // http.Request = request

  lua_setfield(L, -2, "http"); // xcsoar.http = http
  lua_pop(L, 1); // pop global "xcsoar"

  CreateHttpRequestMetatable(L);
}
