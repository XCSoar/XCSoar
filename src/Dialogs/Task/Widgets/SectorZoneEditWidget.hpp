// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ObservationZoneEditWidget.hpp"

class SectorZone;

class SectorZoneEditWidget : public ObservationZoneEditWidget {
public:
  explicit SectorZoneEditWidget(SectorZone &_oz) noexcept;

protected:
  const SectorZone &GetObject() const noexcept {
    return (const SectorZone &)ObservationZoneEditWidget::GetObject();
  }

  SectorZone &GetObject() noexcept {
    return (SectorZone &)ObservationZoneEditWidget::GetObject();
  }

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};
