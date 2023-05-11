// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DownloadTask.hpp"
#include "Error.hpp"
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
#include "lib/fmt/RuntimeError.hxx"
#include "io/StringOutputStream.hxx"
#include "util/ConvertString.hpp"

#include <boost/json.hpp>

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

  StringOutputStream sos;
  const auto response =
    co_await Curl::CoStreamRequest(curl, std::move(easy), sos);

  if (const auto i = response.headers.find("content-type"sv);
      i != response.headers.end() && i->second == "application/json"sv) {
    /* on error, WeGlide returns a JSON document, and if a user does
       not have a declared task (or if the user does not exist), it
       returns a JSON "null" value */
    if (response.status == 200)
      co_return nullptr;

    throw ResponseToException(response.status,
                              boost::json::parse(sos.GetValue()));
  }

  if (response.status != 200)
    throw FmtRuntimeError("WeGlide status {}", response.status);

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
