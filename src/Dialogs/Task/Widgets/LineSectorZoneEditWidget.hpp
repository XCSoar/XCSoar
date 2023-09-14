// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ObservationZoneEditWidget.hpp"

class LineSectorZone;

class LineSectorZoneEditWidget : public ObservationZoneEditWidget {
  const bool length_editable;

public:
  LineSectorZoneEditWidget(LineSectorZone &oz, bool length_editable) noexcept;

protected:
  const LineSectorZone &GetObject() const noexcept {
    return (const LineSectorZone &)ObservationZoneEditWidget::GetObject();
  }

  LineSectorZone &GetObject() noexcept {
    return (LineSectorZone &)ObservationZoneEditWidget::GetObject();
  }

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};
