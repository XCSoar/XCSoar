/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_Nano_CONFIG_WIDGET_HPP
#define XCSOAR_Nano_CONFIG_WIDGET_HPP

#include "Widget/RowFormWidget.hpp"

class LXDevice;
class OperationEnvironment;

namespace LX1600 {
  enum class Setting : uint8_t;
  class SettingsMap;
}

class LX16xxConfigWidget : public RowFormWidget {
  enum Controls {
    VARIO_AVG_TIME,
    VARIO_RANGE,
    VARIO_FILTER,
    TE_LEVEL,
    TE_FILTER,
    SMART_VARIO_FILTER,
  };

  LXDevice &device;

public:
  LX16xxConfigWidget(const DialogLook &look, LXDevice &_device)
    :RowFormWidget(look), device(_device) {}

  /* virtual methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);

protected:
  bool SaveFixedSetting(LX1600::Setting key, unsigned idx,
                        LX1600::SettingsMap &settings);
  bool SaveUnsignedSetting(LX1600::Setting key, unsigned idx,
                           LX1600::SettingsMap &settings);
  bool SaveFixedEnumSetting(LX1600::Setting key, unsigned idx,
                            LX1600::SettingsMap &settings, unsigned factor);
};

#endif
