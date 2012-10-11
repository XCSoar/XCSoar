/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifndef XCSOAR_BIG_THERMAL_ASSISTANT_WIDGET_HPP
#define XCSOAR_BIG_THERMAL_ASSISTANT_WIDGET_HPP

#include "Form/OverlappedWidget.hpp"
#include "Blackboard/BlackboardListener.hpp"

struct AttitudeState;
class LiveBlackboard;
struct ThermalAssistantLook;

class BigThermalAssistantWidget
  : public OverlappedWidget, private NullBlackboardListener {
  LiveBlackboard &blackboard;
  const ThermalAssistantLook &look;

public:
  BigThermalAssistantWidget(LiveBlackboard &_blackboard,
                            const ThermalAssistantLook &_look)
    :blackboard(_blackboard), look(_look) {}

  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) gcc_override;
  virtual void Unprepare() gcc_override;
  virtual void Show(const PixelRect &rc) gcc_override;
  virtual void Hide() gcc_override;
  virtual bool SetFocus() gcc_override;

private:
  void Update(const AttitudeState &attitude, const DerivedInfo &calculated);

  /* virtual methods from class BlackboardListener */
  virtual void OnCalculatedUpdate(const MoreData &basic,
                                  const DerivedInfo &calculated) gcc_override;
};

#endif
