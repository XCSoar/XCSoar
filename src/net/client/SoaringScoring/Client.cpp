// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Client.hpp"
#include "Auth.hpp"
#include "net/client/auth/DeviceFlow.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Task/Deserialiser.hpp"
#include "XML/DataNodeXML.hpp"
#include "XML/Node.hpp"
#include "XML/Parser.hpp"
#include "util/StringAPI.hxx"
#include "io/StringOutputStream.hxx"
#include "lib/curl/CoRequest.hxx"
#include "lib/curl/CoStreamRequest.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Setup.hxx"
#include "lib/curl/Slist.hxx"
#include "net/http/Progress.hpp"

#include <stdexcept>

namespace SoaringScoring {
namespace {

void
RequireOk(unsigned status, std::string_view body)
{
  if (status >= 200 && status < 300)
    return;

  auto message = ErrorMessage(body);
  if (message.empty())
    message = "SoaringScoring request failed";
  throw std::runtime_error(std::move(message));
}

bool
HeaderSafe(std::string_view text) noexcept
{
  if (text.empty())
    return false;
  for (const char ch : text) {
    const bool ok = (ch >= 'a' && ch <= 'z') ||
                    (ch >= 'A' && ch <= 'Z') ||
                    (ch >= '0' && ch <= '9') ||
                    ch == '.' || ch == '_' || ch == '-';
    if (!ok)
      return false;
  }
  return true;
}

Co::Task<std::pair<unsigned, std::string>>
AuthGet(CurlGlobal &curl, std::string url, std::string access_token,
        ProgressListener &progress, long timeout)
{
  CurlEasy easy{url.c_str()};
  Curl::Setup(easy);
  easy.SetTimeout(timeout);

  const std::string authorization =
    "Authorization: Bearer " + access_token;
  CurlSlist headers;
  headers.Append(authorization.c_str());
  easy.SetRequestHeaders(headers.Get());

  const Net::ProgressAdapter progress_adapter{easy, progress};
  StringOutputStream body;
  const auto response =
    co_await Curl::CoStreamRequest(curl, std::move(easy), body);
  co_return std::pair{response.status, body.GetValue()};
}

} // namespace

Co::Task<std::pair<unsigned, std::string>>
PostForm(CurlGlobal &curl, std::string url, std::string body)
{
  CurlEasy easy{url.c_str()};
  Curl::Setup(easy);
  easy.SetTimeout(30);
  easy.SetPost();
  easy.SetRequestBody(body);

  CurlSlist headers;
  headers.Append("Content-Type: application/x-www-form-urlencoded");
  easy.SetRequestHeaders(headers.Get());

  const auto response =
    co_await Curl::CoRequest(curl, std::move(easy));
  co_return std::pair{response.status, std::move(response.body)};
}

Co::Task<std::vector<Entry>>
LoadEntries(CurlGlobal &curl, std::string access_token,
            ProgressListener &progress)
{
  auto response = co_await AuthGet(curl, std::string{kEntriesUrl},
                                   std::move(access_token), progress,
                                   30);
  RequireOk(response.first, response.second);
  co_return ActiveEntries(ParsePilot(response.second).entries);
}

Co::Task<FlightTaskList>
LoadFlightTasks(CurlGlobal &curl, std::string access_token,
                unsigned handicap, ProgressListener &progress)
{
  auto entries = co_await LoadEntries(curl, access_token, progress);

  FlightTaskList list;
  for (const auto &entry : entries) {
    if (entry.contest_id.empty())
      continue;

    std::string url{kOrigin};
    url += "/api/v1/public/contests/";
    url += entry.contest_id;
    url += "/tasks";

    /* A championship list can take several seconds. */
    auto response = co_await AuthGet(curl, std::move(url), access_token,
                                     progress, 120);
    RequireOk(response.first, response.second);
    auto tasks = ParseTaskList(response.second);

    for (auto &row : tasks.tasks) {
      if (!RowMatchesClass(row, entry.class_name))
        continue;
      FlightTask task;
      task.row = std::move(row);
      task.contest_name = entry.contest_name.empty()
                            ? tasks.contest_name
                            : entry.contest_name;
      task.upload_url = entry.upload_url;
      list.tasks.push_back(std::move(task));
    }
  }

  std::vector<TaskRow> rows;
  rows.reserve(list.tasks.size());
  for (const auto &task : list.tasks)
    rows.push_back(task.row);
  list.suggested = SuggestTask(rows, handicap);
  co_return list;
}

Co::Task<std::unique_ptr<OrderedTask>>
DownloadFlightTask(CurlGlobal &curl, std::string access_token,
                   std::string task_url,
                   const TaskBehaviour &task_behaviour,
                   const Waypoints *waypoints,
                   ProgressListener &progress)
{
  auto response = co_await AuthGet(curl, AbsoluteUrl(task_url),
                                   std::move(access_token), progress,
                                   60);
  RequireOk(response.first, response.second);

  const auto xml = XML::ParseString(response.second);
  const ConstDataNodeXML root{xml};
  if (!StringIsEqual(root.GetName(), "Task"))
    throw std::runtime_error("Invalid SoaringScoring task");

  auto task = std::make_unique<OrderedTask>(task_behaviour);
  LoadTask(*task, root, waypoints);
  co_return task;
}

Co::Task<UploadReport>
UploadIgc(CurlGlobal &curl, std::string access_token,
          std::string upload_url, std::string filename,
          std::string igc, ProgressListener &progress)
{
  const auto url = AbsoluteUrl(upload_url);
  CurlEasy easy{url.c_str()};
  Curl::Setup(easy);
  easy.SetTimeout(120);
  easy.SetPost();
  easy.SetRequestBody(igc);

  const std::string authorization =
    "Authorization: Bearer " + access_token;
  const std::string file_header = "X-Igc-Filename: " + filename;
  CurlSlist headers;
  headers.Append(authorization.c_str());
  headers.Append("Content-Type: application/octet-stream");
  if (HeaderSafe(filename))
    headers.Append(file_header.c_str());
  easy.SetRequestHeaders(headers.Get());

  const Net::ProgressAdapter progress_adapter{easy, progress};
  StringOutputStream body;
  const auto response =
    co_await Curl::CoStreamRequest(curl, std::move(easy), body);
  RequireOk(response.status, body.GetValue());
  co_return ParseUploadReport(body.GetValue());
}

Co::Task<bool>
RevokeToken(CurlGlobal &curl, std::string access_token)
{
  auto response = co_await PostForm(
    curl, std::string{kRevokeUrl},
    Net::Client::Auth::FormField("token", access_token));
  RequireOk(response.first, response.second);
  co_return true;
}

} // namespace SoaringScoring
