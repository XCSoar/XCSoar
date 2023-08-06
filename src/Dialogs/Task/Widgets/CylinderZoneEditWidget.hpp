// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ObservationZoneEditWidget.hpp"

class CylinderZone;

class CylinderZoneEditWidget : public ObservationZoneEditWidget {
  const bool radius_editable;

public:
  CylinderZoneEditWidget(CylinderZone &oz, bool _length_editable) noexcept;

protected:
  const CylinderZone &GetObject() const noexcept {
    return (const CylinderZone &)ObservationZoneEditWidget::GetObject();
  }

  CylinderZone &GetObject() noexcept {
    return (CylinderZone &)ObservationZoneEditWidget::GetObject();
  }

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};
