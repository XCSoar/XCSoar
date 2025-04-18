// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/RowFormWidget.hpp"

class MapItemListSettingsPanel final : public RowFormWidget {
  enum ControlIndex {
    AddLocation,
    AddArrivalAltitude,
  };

public:
  MapItemListSettingsPanel() noexcept;

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};
