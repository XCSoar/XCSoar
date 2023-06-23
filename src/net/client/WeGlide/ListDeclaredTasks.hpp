// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <string>
#include <vector>

class CurlGlobal;
struct WeGlideSettings;
class ProgressListener;
namespace Co { template<typename T> class Task; }

namespace WeGlide {

struct TaskInfo {
  uint_least64_t id;

  std::string name;

  std::string user_name;

  /**
   * The total task distance [m].
   */
  double distance;
};

/**
 * Download a list of all tasks declared in WeGlide.
 *
 * @see https://api.weglide.org/docs#/task/get_all_declared_tasks_v1_task_declaration_get
 */
Co::Task<std::vector<TaskInfo>>
ListDeclaredTasks(CurlGlobal &curl, const WeGlideSettings &settings,
                  ProgressListener &progress);

} // namespace WeGlide
