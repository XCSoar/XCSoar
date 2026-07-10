// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Layers.hpp"
#include "system/Path.hpp"
#include "thread/StandbyThread.hpp"
#include "ui/event/Notify.hpp"

#include <exception>
#include <functional>
#include <map>
#include <string>
#include <string_view>

enum class SkySightPreparedDataKind {
  DisplayReady,
  NeedsNetCdfDecode,
};

struct SkySightPreparedData {
  SkySightPreparedDataKind kind = SkySightPreparedDataKind::DisplayReady;
  AllocatedPath source_path;
  AllocatedPath display_path;
  AllocatedPath cleanup_source_path;
  AllocatedPath cleanup_download_path;

  [[nodiscard]] bool IsDisplayReady() const noexcept {
    return kind == SkySightPreparedDataKind::DisplayReady;
  }

  [[nodiscard]] bool NeedsDecode() const noexcept {
    return kind != SkySightPreparedDataKind::DisplayReady;
  }

  [[nodiscard]] Path GetAvailablePath() const noexcept {
    return IsDisplayReady() ? Path{display_path} : Path{source_path};
  }
};

class SkySightFileDecoder final {
public:
  static SkySightPreparedData Prepare(Path path);
  static void InvalidateCache(Path path) noexcept;
  static bool IsNetCdfDecodeAvailable() noexcept;
};

class SkySightFileDecodeJob final : private StandbyThread {
public:
  enum class Status {
    Idle,
    Busy,
    Complete,
    Error,
  };

private:
  UI::Notify notify;
  SkySightPreparedData prepared;
  std::string variable_name;
  std::map<float, SkySight::LegendColor> legend;
  AllocatedPath result_path;
  std::exception_ptr error;
  std::function<void(AllocatedPath)> on_success;
  std::function<void(std::exception_ptr)> on_error;
  Status status = Status::Idle;

public:
  SkySightFileDecodeJob() noexcept;
  ~SkySightFileDecodeJob() noexcept;

  void Start(SkySightPreparedData prepared, std::string variable_name,
             std::map<float, SkySight::LegendColor> legend,
             std::function<void(AllocatedPath)> on_success,
             std::function<void(std::exception_ptr)> on_error);

  void Cancel() noexcept;
  Status GetStatus() noexcept;

private:
  void Tick() noexcept override;
  void OnNotification() noexcept;
};
