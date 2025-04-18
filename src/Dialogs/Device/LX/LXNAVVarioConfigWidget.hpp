// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/RowFormWidget.hpp"

class LXDevice;

class LXNAVVarioConfigWidget final : public RowFormWidget {
  enum Controls {
    BRGPS,
    BRPDA
  };

  LXDevice &device;

  unsigned brgps, brpda;

public:
  LXNAVVarioConfigWidget(const DialogLook &look, LXDevice &_device)
    :RowFormWidget(look), device(_device) {}

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};
