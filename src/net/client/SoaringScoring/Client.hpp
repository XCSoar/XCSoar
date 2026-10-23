// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Parse.hpp"
#include "co/Task.hxx"

#include <memory>
#include <string>
#include <utility>
#include <vector>

class CurlGlobal;
class OrderedTask;
class ProgressListener;
class Waypoints;
struct TaskBehaviour;

namespace SoaringScoring {

struct FlightTask {
  TaskRow row;
  std::string contest_name;
  std::string upload_url;
};

struct FlightTaskList {
  std::vector<FlightTask> tasks;
  /** Index into #tasks. Zero when the list is empty. */
  unsigned suggested = 0;
};

[[nodiscard]] Co::Task<std::pair<unsigned, std::string>>
PostForm(CurlGlobal &curl, std::string url, std::string body);

[[nodiscard]] Co::Task<std::vector<Entry>>
LoadEntries(CurlGlobal &curl, std::string access_token,
            ProgressListener &progress);

/**
 * Tasks for the pilot's current contest entries, limited to each
 * entry's class. @p handicap selects the suggested row.
 */
[[nodiscard]] Co::Task<FlightTaskList>
LoadFlightTasks(CurlGlobal &curl, std::string access_token,
                unsigned handicap, ProgressListener &progress);

[[nodiscard]] Co::Task<std::unique_ptr<OrderedTask>>
DownloadFlightTask(CurlGlobal &curl, std::string access_token,
                   std::string task_url,
                   const TaskBehaviour &task_behaviour,
                   const Waypoints *waypoints,
                   ProgressListener &progress);

[[nodiscard]] Co::Task<UploadReport>
UploadIgc(CurlGlobal &curl, std::string access_token,
          std::string upload_url, std::string filename,
          std::string igc, ProgressListener &progress);

[[nodiscard]] Co::Task<bool>
RevokeToken(CurlGlobal &curl, std::string access_token);

} // namespace SoaringScoring
