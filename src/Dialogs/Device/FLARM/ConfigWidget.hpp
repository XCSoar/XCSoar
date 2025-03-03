// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/RowFormWidget.hpp"

class FlarmDevice;
struct FlarmHardware;

class FLARMConfigWidget final : public RowFormWidget {
  enum Controls {
    Baud,
    Priv,
    Thre,
    Range,
    VRange,
    PCASRange,
    PCASVRange,
    ADSBRange,
    ADSBVrange,
    Acft,
    LogInt,
    NoTrack,
  };

  FlarmDevice &device;
  FlarmHardware &hardware;

  unsigned baud, thre, range, vrange, pcas_range, pcas_vrange, adsb_range, adsb_vrange, acft, log_int;

  bool priv, notrack;

public:
  FLARMConfigWidget(const DialogLook &look, FlarmDevice &_device, FlarmHardware &_hardware)
    :RowFormWidget(look), device(_device), hardware(_hardware) {}

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};
