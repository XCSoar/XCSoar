// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/RowFormWidget.hpp"
#include "Device/Driver/CAI302/Protocol.hpp"

class CAI302UnitsEditor final : public RowFormWidget {
  enum Controls {
    VarioUnit,
    AltitudeUnit,
    TemperatureUnit,
    PressureUnit,
    DistanceUnit,
    SpeedUnit,
    SinkTone,
  };

  CAI302::Pilot data;

public:
  CAI302UnitsEditor(const DialogLook &look, const CAI302::Pilot &_data)
    :RowFormWidget(look), data(_data) {}

  const CAI302::Pilot &GetData() const {
    return data;
  }

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};
