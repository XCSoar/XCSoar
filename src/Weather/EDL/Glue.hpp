// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Blackboard/BlackboardListener.hpp"
#include "Weather/Features.hpp"
#include "time/BrokenDateTime.hpp"

#ifdef HAVE_EDL
#include "Weather/EDL/DownloadGlue.hpp"
#endif

namespace EDL {

/**
 * Keeps the shared EDL forecast hour aligned with GPS time while an
 * overlay is being maintained, and applies download results on the UI
 * thread.
 */
class Glue final
#ifdef HAVE_EDL
  : public NullBlackboardListener,
    private DownloadListener
#else
  : public NullBlackboardListener
#endif
{
public:
  void OnGPSUpdate(const MoreData &basic) override;

#ifdef HAVE_EDL
  void AttachDownloadGlue(DownloadGlue &download_glue) noexcept;
  void DetachDownloadGlue() noexcept;

  void RequestOverlayRefresh() noexcept;
  void RequestPrecacheDay(BrokenDateTime day) noexcept;

private:
  void OnDownloadFinished(const DownloadNotification &notification) noexcept override;

  DownloadGlue *download_glue = nullptr;
#endif
};

#ifdef HAVE_EDL
void RequestOverlayRefresh() noexcept;
void RequestPrecacheDay(BrokenDateTime day) noexcept;
#endif

} // namespace EDL
