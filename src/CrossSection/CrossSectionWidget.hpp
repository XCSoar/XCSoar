// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/WindowWidget.hpp"
#include "Blackboard/BlackboardListener.hpp"

struct DataComponents;
struct MapSettings;

class CrossSectionWidget : public WindowWidget,
                           private NullBlackboardListener {
  const DataComponents &data_components;

  void Update(const MoreData &basic, const DerivedInfo &calculated,
              const MapSettings &settings) noexcept;

public:
  explicit CrossSectionWidget(const DataComponents &_data_components) noexcept
    :data_components(_data_components) {}

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;

private:
  /* virtual methods from class BlackboardListener */
  virtual void OnCalculatedUpdate(const MoreData &basic,
                                  const DerivedInfo &calculated) override;
};
