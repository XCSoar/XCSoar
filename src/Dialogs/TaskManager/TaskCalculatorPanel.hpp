/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_TASK_CALCULATOR_PANEL_HPP
#define XCSOAR_TASK_CALCULATOR_PANEL_HPP

#include "DataField/Base.hpp"

class Window;
class SingleWindow;
class WndForm;
class TabBarControl;
class WndButton;
class WndOwnerDrawFrame;
class Canvas;

class pnlTaskCalculator
{
public:
  /**
   * creates the control from its XML file and does any init work
   * @param parent
   * @param wf
   * @param _task_modified tells calc whether to display warning
   * @return Window* that points to the control created
   */
  static Window* Load(SingleWindow &parent, TabBarControl* wTabBar,
                      WndForm* wf, bool* _task_modified);

  /**
   * copies values from form to ordered_task
   * @return true
   */
  static bool OnTabPreHide();

  /**
   * copies values from ordered_task to form
   * @param EventType 0 = Mouse Click, 1 = up/dn/left/right key
   * @return true
   */
  static bool OnTabPreShow();

  /**
   * draws colored warning message if task has been modified
   */
  static void OnWarningPaint(WndOwnerDrawFrame *Sender, Canvas &canvas);
  static void OnTargetClicked(WndButton &Sender);

  static void OnTimerNotify(WndForm &Sender);

  static void OnMacCreadyData(DataField *Sender,
      DataField::DataAccessKind_t Mode);

  static void OnCruiseEfficiencyData(DataField *Sender,
      DataField::DataAccessKind_t Mode);
};

#endif
