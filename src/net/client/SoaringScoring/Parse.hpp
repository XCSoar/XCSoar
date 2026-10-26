// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace SoaringScoring {

enum class ContestWhen : uint8_t {
  PAST,
  CURRENT,
  FUTURE,
  UNKNOWN,
};

struct Entry {
  std::string contest_id;
  std::string contest_name;
  std::string class_name;
  std::string upload_url;
  ContestWhen when = ContestWhen::UNKNOWN;
};

struct Pilot {
  std::string name;
  std::vector<Entry> entries;
};

/** One published task row from the task distribution list. */
struct TaskRow {
  std::string display_label;
  std::string class_name;
  std::string date;
  std::string task_kind;
  std::string xcsoar_tsk_url;
  double nominal_distance_km = 0;
  double handicap = 0;
  bool official = false;
  bool has_distance = false;
  bool has_handicap = false;
};

struct TaskList {
  std::string contest_name;
  std::vector<TaskRow> tasks;
};

struct UploadReport {
  bool validation_ok = false;
  std::vector<std::string> issues;
};

/** Prefix a relative API path with the SoaringScoring origin. */
[[nodiscard]] std::string
AbsoluteUrl(std::string_view url);

/**
 * Message from a public-API error body, or empty when the body is
 * not that shape. Does not throw.
 */
[[nodiscard]] std::string
ErrorMessage(std::string_view json) noexcept;

[[nodiscard]] Pilot
ParsePilot(std::string_view json);

[[nodiscard]] TaskList
ParseTaskList(std::string_view json);

[[nodiscard]] UploadReport
ParseUploadReport(std::string_view json);

/** Current entries, or future ones when nothing is current. */
[[nodiscard]] std::vector<Entry>
ActiveEntries(const std::vector<Entry> &entries);

[[nodiscard]] bool
RowMatchesClass(const TaskRow &row,
                std::string_view class_name) noexcept;

/**
 * Contest handicap index, where 100 is standard.
 * A value below 5 is a factor (0.91 is 91). Larger values are
 * already an index.
 */
[[nodiscard]] unsigned
HandicapIndex(double value) noexcept;

/**
 * Handicap row for @p handicap, otherwise the first official row.
 * @p handicap 0 does not match a distance-handicap row.
 */
[[nodiscard]] unsigned
SuggestTask(std::span<const TaskRow> rows,
            unsigned handicap) noexcept;

} // namespace SoaringScoring
