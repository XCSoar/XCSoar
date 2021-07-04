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

#include "ToStream.hpp"
#include "Request.hxx"
#include "OutputStreamHandler.hxx"
#include "Progress.hpp"
#include "Operation/Operation.hpp"
#include "util/ScopeExit.hxx"

#include <cassert>

namespace Net {

namespace {

class DownloadToStreamHandler final : public OutputStreamCurlResponseHandler {
public:
  using OutputStreamCurlResponseHandler::OutputStreamCurlResponseHandler;

  /* virtual methods from class CurlResponseHandler */
  void OnHeaders(unsigned status,
                 std::multimap<std::string, std::string> &&headers) override {
  }
};

} // anonymous namespace

void
DownloadToStream(CurlGlobal &curl, CurlEasy easy,
                 OutputStream &out,
                 OperationEnvironment &env)
{
  assert(easy);

  const Net::ProgressAdapter progress(easy, env);

  DownloadToStreamHandler handler(out);
  CurlRequest request(curl, std::move(easy), handler);
  AtScopeExit(&request) { request.StopIndirect(); };

  env.SetCancelHandler([&]{
    request.StopIndirect();
    handler.Cancel();
  });

  AtScopeExit(&env) { env.SetCancelHandler({}); };

  request.StartIndirect();
  handler.Wait();
}

void
DownloadToStream(CurlGlobal &curl, const char *url,
                 const char *username, const char *password,
                 OutputStream &out,
                 OperationEnvironment &env)
{
  assert(url != nullptr);

  CurlEasy easy;
  easy.SetURL(url);
  easy.SetFailOnError();

  if (username != nullptr)
    easy.SetOption(CURLOPT_USERNAME, username);
  if (password != nullptr)
    easy.SetOption(CURLOPT_PASSWORD, password);

  DownloadToStream(curl, std::move(easy), out, env);
}

} // namespace Net
