// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Types.hpp"

#include <optional>

enum class UpdateCheckTrigger {
  MANUAL,
  SCHEDULED,
};

/**
 * A build-selected update provider.  The owner calls ConsumeCheckResult()
 * only on the UI thread after its completion notification has fired.
 */
class UpdateBackend {
public:
  virtual ~UpdateBackend() = default;

  [[gnu::const]] virtual UpdateBackendId GetId() const noexcept = 0;
  [[gnu::const]] virtual int GetPriority() const noexcept { return 0; }
  [[gnu::const]] virtual bool
  SupportsCheck(UpdateCheckTrigger trigger) const noexcept = 0;
  virtual void OnAttached() noexcept {}

  virtual void StartCheck(UpdateCheckTrigger trigger) = 0;
  virtual std::optional<UpdateCheckResult> ConsumeCheckResult() = 0;
  virtual void BeginShutdown() noexcept = 0;
};
