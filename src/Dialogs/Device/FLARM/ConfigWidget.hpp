// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/RowFormWidget.hpp"
#include "util/StaticString.hxx"

class FlarmDevice;
struct FlarmHardware;

class FLARMConfigWidget final : public RowFormWidget {
  enum Controls {
    Baud1,
    Baud2,
    Thre,
    Acft,
    LogInt,
    Priv,
    NoTrack,
  };

  FlarmDevice &device;
  FlarmHardware &hardware;

  unsigned baud, baud1, baud2, thre, acft, log_int;
  unsigned nmeaout, nmeaout1, nmeaout2;
  unsigned nmeaout_raw, nmeaout1_raw, nmeaout2_raw;
  unsigned transponder_type;

  bool priv, notrack;
  bool nmeaout_available, nmeaout1_available, nmeaout2_available;
  bool icaoid_available, transponder_available;

  StaticString<32> icaoid;

public:
  FLARMConfigWidget(const DialogLook &look, FlarmDevice &_device, FlarmHardware &_hardware)
    :RowFormWidget(look), device(_device), hardware(_hardware) {}

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};
