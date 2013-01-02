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

#ifndef XCSOAR_TRAFFIC_WIDGET_HPP
#define XCSOAR_TRAFFIC_WIDGET_HPP

#include "Form/WindowWidget.hpp"
#include "Blackboard/BlackboardListener.hpp"

class FlarmTrafficControl2;

class TrafficWidget : public WindowWidget, private NullBlackboardListener {
  FlarmTrafficControl2 *view;

  //CheckBox *auto_zoom, *north_up;

public:
  void Update();
  void OpenDetails();
  void ZoomIn();
  void ZoomOut();
  void PreviousTarget();
  void NextTarget();
  void SwitchData();

  void SetAutoZoom(bool value);
  void ToggleAutoZoom();

  void SetNorthUp(bool value);
  void ToggleNorthUp();

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual void Unprepare();
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
  virtual bool SetFocus();

private:
  virtual void OnGPSUpdate(const MoreData &basic);
};

#endif
