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

#ifndef XCSOAR_TASK_CLOSE_PANEL_HPP
#define XCSOAR_TASK_CLOSE_PANEL_HPP

class Window;
class SingleWindow;
class WndForm;
class WndButton;
class TabBarControl;
class OrderedTask;

class pnlTaskManagerClose
{
public:
  /**
   * creates the control from its XML file and does any init work
   * @param parent
   * @param task_modified Sets to True if changes it.
   * @param wf
   * @return Window* that points to the control created
   */
  static Window* Load(SingleWindow &parent, TabBarControl* wTabBar,
                      WndForm* wf, OrderedTask** task, bool* _task_modified);

  static void OnCloseClicked(WndButton &Sender);
  static void OnRevertClicked(WndButton &Sender);

  /**
   * callback
   * sets status and buttons per task edit status
   * Closes immediately if flying and task not changed and mouse click
   * @param EventType 0 = Mouse Click, 1 = up/dn/left/right key
   * @return True
   */
  static bool OnTabPreShow();

  static bool OnTabClick();

  /**
   * Called when the Close tab is displayed and clicked a 2nd time
   * Acts as if the user clicked the Fly button
   */
  static void OnTabReClick();

};

#endif
