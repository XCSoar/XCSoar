// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ObservationZoneEditWidget.hpp"
#include "Engine/Task/ObservationZones/KeyholeZone.hpp"

class KeyholeZoneEditWidget : public ObservationZoneEditWidget {
public:
  explicit KeyholeZoneEditWidget(KeyholeZone &_oz) noexcept;

protected:
  const KeyholeZone &GetObject() const noexcept {
    return (const KeyholeZone &)ObservationZoneEditWidget::GetObject();
  }

  KeyholeZone &GetObject() noexcept {
    return (KeyholeZone &)ObservationZoneEditWidget::GetObject();
  }

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};
