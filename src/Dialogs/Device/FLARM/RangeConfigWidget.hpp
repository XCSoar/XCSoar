// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/RowFormWidget.hpp"

class FlarmDevice;
struct FlarmHardware;

class FLARMRangeConfigWidget final : public RowFormWidget {
  enum Controls {
    Range,
    VRange,
    PCASRange,
    PCASVRange,
    ADSBRange,
    ADSBVrange,
  };

  FlarmDevice &device;
  FlarmHardware &hardware;

  unsigned range, vrange, pcas_range, pcas_vrange, adsb_range, adsb_vrange;

public:
  FLARMRangeConfigWidget(const DialogLook &look, FlarmDevice &_device, FlarmHardware &_hardware)
    :RowFormWidget(look), device(_device), hardware(_hardware) {}

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};
