// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/RowFormWidget.hpp"

class FlarmDevice;

class FLARMConfigWidget final : public RowFormWidget {
  enum Controls {
    Baud,
    Priv,
    Thre,
    Range,
    Acft,
    LogInt,
    NoTrack,
  };

  FlarmDevice &device;

  unsigned baud, thre, range, acft, log_int;

  bool priv, notrack;

public:
  FLARMConfigWidget(const DialogLook &look, FlarmDevice &_device)
    :RowFormWidget(look), device(_device) {}

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};
