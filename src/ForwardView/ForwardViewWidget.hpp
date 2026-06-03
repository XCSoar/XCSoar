// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/WindowWidget.hpp"
#include "Blackboard/BlackboardListener.hpp"
#include "ui/event/PeriodicTimer.hpp"

struct DataComponents;
struct MapSettings;

class ForwardViewWidget : public WindowWidget,
                          private NullBlackboardListener {
  const DataComponents &data_components;

  UI::PeriodicTimer smooth_timer{[this]{ OnSmoothTimer(); }};

  void Update(const MoreData &basic, const DerivedInfo &calculated,
              const MapSettings &settings) noexcept;

  void OnSmoothTimer() noexcept;

public:
  explicit ForwardViewWidget(const DataComponents &data_components) noexcept
    :data_components(data_components) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;

private:
  void OnCalculatedUpdate(const MoreData &basic,
                          const DerivedInfo &calculated) override;
};
