// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/RowFormWidget.hpp"

class LXDevice;
class OperationEnvironment;

class NanoConfigWidget final : public RowFormWidget {
  enum Controls {
    BAUDRATE,
    //NMEARATE,
    AUTOOFF,
    OFFFIN,
    //NEARDIS,
    ALWRUN,
    NMEA,
    //ACCELL,
    RECINT,
  };

  LXDevice &device;

public:
  NanoConfigWidget(const DialogLook &look, LXDevice &_device)
    :RowFormWidget(look), device(_device) {}

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

protected:
  bool SaveSettingBoolean(const char *name, unsigned idx,
                          OperationEnvironment &env);
  bool SaveSettingInteger(const char *name, unsigned idx,
                          OperationEnvironment &env);
  bool SaveSettingEnum(const char *name, unsigned idx,
                          OperationEnvironment &env);
};
