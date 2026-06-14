// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/CursorBarWidget.hpp"
#include "Blackboard/BlackboardListener.hpp"
#include "Weather/EDL/DownloadGlue.hpp"
#include "Weather/Features.hpp"

#include <memory>

/**
 * Map-bottom EDL forecast controls: forecast hour and pressure level
 * steppers. Auto advance is resumed by tapping the centre label.
 */
class EdlControlsWidget final : public CursorBarWidget,
                               public NullBlackboardListener,
                               private EDL::DownloadListener
{
  static constexpr unsigned FORECAST_ROW = 0;
  static constexpr unsigned LEVEL_ROW = 1;

  struct Private;
  std::unique_ptr<Private> data;

  void UpdateLabels() noexcept;
  void RefreshEdlOverlay() noexcept;
  void OnStepForecast(int delta) noexcept;
  void OnStepLevel(int delta) noexcept;
  void OnResumeAuto() noexcept;
  void UnregisterEdlDownloadListener() noexcept;

public:
  EdlControlsWidget();
  ~EdlControlsWidget() noexcept override;

  EdlControlsWidget(const EdlControlsWidget &) = delete;
  EdlControlsWidget &operator=(const EdlControlsWidget &) = delete;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;

  /* virtual methods from class BlackboardListener */
  void OnGPSUpdate(const MoreData &basic) override;
  void OnDownloadFinished(const EDL::DownloadNotification &) noexcept override;

  void HandleWeatherOverlayInput(const char *misc) noexcept;
};
