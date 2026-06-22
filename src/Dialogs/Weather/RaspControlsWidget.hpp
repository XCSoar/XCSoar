// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/CursorBarWidget.hpp"
#include "Blackboard/BlackboardListener.hpp"

#include <memory>

/**
 * Map-bottom RASP forecast controls. Row 0 steps forecast time (tap
 * resumes auto advance); row 1 steps the RASP layer (tap opens a list).
 */
class RaspControlsWidget final : public CursorBarWidget,
                                 public NullBlackboardListener
{
  static constexpr unsigned TIME_ROW = 0;
  static constexpr unsigned FIELD_ROW = 1;

  struct Private;
  std::unique_ptr<Private> data;

  void UpdateLabels() noexcept;
  void RefreshRaspOverlay() noexcept;
  void OnStepTime(int delta) noexcept;
  void OnStepField(int delta) noexcept;
  void OnResumeAuto() noexcept;
  void OnPickField() noexcept;

public:
  RaspControlsWidget();
  ~RaspControlsWidget() noexcept override;

  RaspControlsWidget(const RaspControlsWidget &) = delete;
  RaspControlsWidget &operator=(const RaspControlsWidget &) = delete;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;

  /* virtual methods from class BlackboardListener */
  void OnGPSUpdate(const MoreData &basic) override;

  void HandleWeatherOverlayInput(const char *misc) noexcept;
};
