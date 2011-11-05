/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#ifndef DLGINFOBOXACCESS_HPP
#define DLGINFOBOXACCESS_HPP

#include "Dialogs/Internal.hpp"
#include "DataField/Enum.hpp"

class dlgInfoBoxAccess
{
public:
  dlgInfoBoxAccess();

  /**
   * @returns True if validated, False if window shall remain open
   */
  static bool OnClose();

  static void dlgInfoBoxAccessShowModal(SingleWindow &parent, const int id);

  static WndForm* GetWindowForm();


  /**
   * creates the control from its XML file and does any init work
   * @param parent
   * @param wf
   * @return Window* that points to the control created
   */
  static Window* pnlCloseLoad(SingleWindow &parent, TabBarControl* wTabBar,
                              WndForm* wf);

  static void pnlCloseOnCloseClicked(WndButton &Sender);
  static bool pnlCloseOnTabClick();
  /**
   * callback
   */
  static void pnlCloseOnTabReClick();
  static bool pnlCloseOnTabPreShow();
};

#endif /* DLGINFOBOXACCESS_H_ */
