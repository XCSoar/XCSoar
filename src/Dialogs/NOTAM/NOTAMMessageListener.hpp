// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef HAVE_HTTP

#include "NOTAM/NOTAMGlue.hpp"

/**
 * Shows user-facing NOTAM fetch status messages on the main thread.
 */
class NOTAMMessageListener final : public NOTAMListener {
  NOTAMGlue &glue;

public:
  explicit NOTAMMessageListener(NOTAMGlue &_glue) noexcept;
  ~NOTAMMessageListener() noexcept;

  void OnNOTAMsUpdated() noexcept override;
  void OnNOTAMsLoadComplete(NOTAMLoadNotification notification) noexcept override;
};

void InstallNOTAMMessageListener(NOTAMGlue &glue) noexcept;
void DeinitNOTAMMessageListener() noexcept;

#endif // HAVE_HTTP
