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

#include "DownloadTask.hpp"
#include "Settings.hpp"
#include "Task/Ordered/OrderedTask.hpp"
#include "Task/Deserialiser.hpp"
#include "XML/DataNodeXML.hpp"
#include "XML/Node.hpp"
#include "XML/Parser.hpp"
#include "net/http/Progress.hpp"
#include "lib/curl/CoStreamRequest.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Setup.hxx"
#include "io/StringOutputStream.hxx"
#include "util/ConvertString.hpp"

#include <cassert>

using std::string_view_literals::operator""sv;

namespace WeGlide {

Co::Task<std::unique_ptr<OrderedTask>>
DownloadDeclaredTask(CurlGlobal &curl, const WeGlideSettings &settings,
                     const TaskBehaviour &task_behaviour,
                     const Waypoints *waypoints,
                     ProgressListener &progress)
{
  assert(settings.pilot_id != 0);

  char url[256];
  snprintf(url, sizeof(url), "%s/task/declaration/%u?cup=false&tsk=true",
           settings.default_url, settings.pilot_id);

  CurlEasy easy{url};
  Curl::Setup(easy);
  const Net::ProgressAdapter progress_adapter{easy, progress};
  easy.SetFailOnError();

  StringOutputStream sos;
  const auto response =
    co_await Curl::CoStreamRequest(curl, std::move(easy), sos);

  if (const auto i = response.headers.find("content-type"sv);
      i != response.headers.end() && i->second == "application/json"sv)
    /* on error, WeGlide returns a JSON document, and if a user does
       not have a declared task (or if the user does not exist), it
       returns a JSON "null" value */
    co_return nullptr;

  /* XCSoar task files are returned with
     "Content-Type:application/octet-stream", and we could verify
     that, but it's not the correct MIME type, and may change
     eventually, so let's just ignore the Content-Type for now and
     hope the XML parser catches syntax errors */

  const auto xml_node = XML::ParseString(UTF8ToWideConverter{sos.GetValue().c_str()});
  const ConstDataNodeXML data_node{xml_node};

  auto task = std::make_unique<OrderedTask>(task_behaviour);
  LoadTask(*task, data_node, waypoints);
  co_return task;
}

} // namespace WeGlide
