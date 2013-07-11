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

#ifndef XCSOAR_INFOBOX_CONTENT_THERMAL_HPP
#define XCSOAR_INFOBOX_CONTENT_THERMAL_HPP

#include "InfoBoxes/Content/Base.hpp"
#include "Gauge/ThermalAssistantRenderer.hpp"

void
UpdateInfoBoxVario(InfoBoxData &data);

void
UpdateInfoBoxVarioNetto(InfoBoxData &data);

void
UpdateInfoBoxThermal30s(InfoBoxData &data);

void
UpdateInfoBoxThermalLastAvg(InfoBoxData &data);

void
UpdateInfoBoxThermalLastGain(InfoBoxData &data);

void
UpdateInfoBoxThermalLastTime(InfoBoxData &data);

void
UpdateInfoBoxThermalAllAvg(InfoBoxData &data);

void
UpdateInfoBoxThermalAvg(InfoBoxData &data);

void
UpdateInfoBoxThermalGain(InfoBoxData &data);

void
UpdateInfoBoxThermalRatio(InfoBoxData &data);

void
UpdateInfoBoxVarioDistance(InfoBoxData &data);

void
UpdateInfoBoxNextLegEqThermal(InfoBoxData &data);

void
UpdateInfoBoxCircleDiameter(InfoBoxData &data);

class InfoBoxContentThermalAssistant: public InfoBoxContent
{
  ThermalAssistantRenderer renderer;

public:
  InfoBoxContentThermalAssistant();

  virtual void Update(InfoBoxData &data) override;
  virtual void OnCustomPaint(Canvas &canvas, const PixelRect &rc) override;
};

#endif
