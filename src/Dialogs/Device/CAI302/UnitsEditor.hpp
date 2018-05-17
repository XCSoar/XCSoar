/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_CAI302_UNITS_EDITOR_HPP
#define XCSOAR_CAI302_UNITS_EDITOR_HPP

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
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual bool Save(bool &changed) override;
};

#endif
