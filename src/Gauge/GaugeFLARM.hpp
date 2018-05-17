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

#ifndef GAUGE_FLARM_HPP
#define GAUGE_FLARM_HPP

#include "Widget/OverlappedWidget.hpp"
#include "Blackboard/BlackboardListener.hpp"

class LiveBlackboard;
struct FlarmTrafficLook;
struct NMEAInfo;

/**
 * Widget to display a FLARM gauge
 */
class GaugeFLARM final : public OverlappedWidget, NullBlackboardListener {
  LiveBlackboard &blackboard;
  const FlarmTrafficLook &look;

public:
  GaugeFLARM(LiveBlackboard &_blackboard, const FlarmTrafficLook &_look)
    :blackboard(_blackboard), look(_look) {}

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual void Unprepare() override;
  virtual void Show(const PixelRect &rc) override;
  virtual void Hide() override;

private:
  void Update(const NMEAInfo &basic);

  void OnGPSUpdate(const MoreData &basic) override;
};

#endif
