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

#ifndef GLUE_GAUGE_VARIO_H
#define GLUE_GAUGE_VARIO_H

#include "Form/WindowWidget.hpp"
#include "Blackboard/BlackboardListener.hpp"

struct VarioLook;
struct UnitsLook;
class LiveBlackboard;

/**
 * A variant of GaugeVario which auto-updates its data from the device
 * blackboard.
 */
class GlueGaugeVario
  : public WindowWidget, private NullBlackboardListener {
  LiveBlackboard &blackboard;
  const VarioLook &look;
  const UnitsLook &units_look;

public:
  GlueGaugeVario(LiveBlackboard &_blackboard, const VarioLook &_look,
                 const UnitsLook &_units_look)
    :blackboard(_blackboard), look(_look), units_look(_units_look) {}

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual void Unprepare();
  virtual void Show(const PixelRect &rc);
  virtual void Hide();

private:
  virtual void OnGPSUpdate(const MoreData &basic);
};

#endif
