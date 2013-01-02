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

#include "Widget/ContainerWidget.hpp"
#include "Form/ActionListener.hpp"
#include "Blackboard/BlackboardListener.hpp"
#include "Compiler.h"

class WndButton;
class WndSymbolButton;
class FlarmTrafficControl;

class TrafficWidget : public ContainerWidget,
#ifndef GNAV
                      private ActionListener,
#endif
                      private NullBlackboardListener {
#ifndef GNAV
  enum Action {
    CLOSE,
    ZOOM_IN,
    ZOOM_OUT,
    PREVIOUS_ITEM,
    NEXT_ITEM,
    DETAILS,
  };

  WndSymbolButton *zoom_in_button, *zoom_out_button;
  WndSymbolButton *previous_item_button, *next_item_button;
  WndButton *details_button;
  WndButton *close_button;
#endif

  FlarmTrafficControl *view;

protected:
  void UpdateLayout();
  void UpdateButtons();

public:
  void Update();
  void OpenDetails();
  void ZoomIn();
  void ZoomOut();
  void PreviousTarget();
  void NextTarget();
  void SwitchData();

  gcc_pure
  bool GetAutoZoom() const;
  void SetAutoZoom(bool value);
  void ToggleAutoZoom();

  gcc_pure
  bool GetNorthUp() const;
  void SetNorthUp(bool value);
  void ToggleNorthUp();

  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) gcc_override;
  virtual void Unprepare() gcc_override;
  virtual void Show(const PixelRect &rc) gcc_override;
  virtual void Hide() gcc_override;
  virtual void Move(const PixelRect &rc) gcc_override;
  virtual bool SetFocus() gcc_override;

private:
#ifndef GNAV
  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) gcc_override;
#endif

  /* virtual methods from class BlackboardListener */
  virtual void OnGPSUpdate(const MoreData &basic) gcc_override;
};

#endif
