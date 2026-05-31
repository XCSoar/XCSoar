// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

#include "PageSettings.hpp"
#include "Usage.hpp"

class DataFieldEnum;
class RaspStore;

namespace MapOverlay {

class RaspControlsModel {
  [[gnu::pure]]
  int GetFieldIndex() const noexcept;

public:
  void SyncFromPageLayout() noexcept;

  void FillTimeChoices(DataFieldEnum &field,
                       const std::shared_ptr<RaspStore> &rasp) const noexcept;

  void SetTime(unsigned minute_of_day) noexcept;

  [[gnu::pure]]
  bool GetTimeAutoAdvance() const noexcept;

  void SetTimeAutoAdvance(bool auto_advance) noexcept;
  void ApplyAutoAdvanceTime() noexcept;
};

} // namespace MapOverlay
