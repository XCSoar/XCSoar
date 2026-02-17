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

/**
 * Task kind/type as returned by the WeGlide API.
 *
 * @see https://api.weglide.org/docs#/task
 */
enum class TaskKind : uint_least8_t {
  UNKNOWN,

  /** Free flight (FAI triangle) */
  FREE,

  /** Free flight with 4 turnpoints */
  FREE4,

  /** Triangle */
  TRIANGLE,
};

[[gnu::const]]
const char *
ToString(TaskKind kind) noexcept;

/**
 * Parse the "kind" field from a WeGlide task JSON object.
 */
[[gnu::pure]]
TaskKind
ParseTaskKind(std::string_view kind) noexcept;

struct TurnpointInfo {
  std::string name;

  /**
   * Elevation in metres.
   */
  int elevation;

  /**
   * Observation zone radius [m], or -1 if not specified.
   */
  double radius;

  double longitude, latitude;
};

struct TaskInfo {
  uint_least64_t id;

  std::string name;

  std::string user_name;

  /**
   * The total task distance [m].
   */
  double distance;

  TaskKind kind = TaskKind::UNKNOWN;

  /**
   * The scoring ruleset (e.g. "D", "US", "AA").
   */
  std::string ruleset;

  /**
   * The share token for this task.
   */
  std::string token;

  /**
   * Turnpoint information from the point_features array.
   */
  std::vector<TurnpointInfo> turnpoints;
};

/**
 * Download a list of all tasks by the specified user.
 *
 * @see https://api.weglide.org/docs#/task/get_task_by_user_v1_task_get
 */
Co::Task<std::vector<TaskInfo>>
ListTasksByUser(CurlGlobal &curl, const WeGlideSettings &settings,
                uint_least64_t user_id,
                ProgressListener &progress);

/**
 * Download a list of all tasks declared in WeGlide.
 *
 * @see https://api.weglide.org/docs#/task/get_all_declared_tasks_v1_task_declaration_get
 */
Co::Task<std::vector<TaskInfo>>
ListDeclaredTasks(CurlGlobal &curl, const WeGlideSettings &settings,
                  ProgressListener &progress);

/**
 * Download the list of daily competition tasks from WeGlide.
 *
 * @see https://api.weglide.org/docs#/task/daily_competitions_v1_task_competitions_today_get
 */
Co::Task<std::vector<TaskInfo>>
ListDailyCompetitions(CurlGlobal &curl, const WeGlideSettings &settings,
                      ProgressListener &progress);

} // namespace WeGlide
