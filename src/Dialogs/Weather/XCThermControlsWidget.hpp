// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Blackboard/BlackboardListenerRegistration.hpp"
#include "Widget/CursorBarWidget.hpp"
#include "Blackboard/BlackboardListener.hpp"
#include "Weather/xctherm/XCThermControlsModel.hpp"

/**
 * Map-bottom XCTherm cursor bar (layer + forecast time).
 * Orchestration lives in #XCTherm::XCThermControlsModel.
 */
class XCThermControlsWidget final : public CursorBarWidget,
                                    public NullBlackboardListener
{
  static constexpr unsigned LAYER_ROW = 0;
  static constexpr unsigned TIME_ROW = 1;

  XCTherm::XCThermControlsModel model;
  BlackboardListenerRegistration blackboard_listener;

  void UpdateLabels() noexcept;
  void OnStepLayer(int delta) noexcept;
  void OnStepTime(int delta) noexcept;
  void OnResumeLayerAuto() noexcept;
  void OnResumeTimeAuto() noexcept;
  void OnLabelClicked(unsigned row) noexcept;
  void OnRequestDownload() noexcept;

public:
  XCThermControlsWidget();

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;

  /* virtual methods from class BlackboardListener */
  void OnGPSUpdate(const MoreData &basic) override;
};
