// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

enum class SkySightPreparedDataKind {
  DisplayReady,
  NeedsNetCdfDecode,
};

struct SkySightPreparedData {
  SkySightPreparedDataKind kind = SkySightPreparedDataKind::DisplayReady;
  AllocatedPath source_path;
  AllocatedPath display_path;

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
};