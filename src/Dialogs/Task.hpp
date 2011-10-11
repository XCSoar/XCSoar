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

#ifndef XCSOAR_DIALOGS_TASK_HPP
#define XCSOAR_DIALOGS_TASK_HPP

class SingleWindow;
class OrderedTask;

void
dlgTaskManagerShowModal(SingleWindow &parent);

bool
dlgTaskPointShowModal(SingleWindow &parent, OrderedTask** task, const unsigned index);

bool
dlgTaskPointType(SingleWindow &parent, OrderedTask** task, const unsigned index);

bool
dlgTaskOptionalStarts(SingleWindow &parent, OrderedTask** task);

bool
dlgTaskPointNew(SingleWindow &parent, OrderedTask** task, const unsigned index);

/**
 * Shows map display zoomed to target point
 * with half dialog popup to manipulate point
 *
 * @param TargetPoint if -1 then goes to active target
 * else goes to TargetPoint by default
 */
void
dlgTargetShowModal(int TargetPoint = -1);

/**
 * Called by #MainWindow when the calculated data got updated, to
 * refresh the #TargetMapWindow.  No-op when the target dialog is not
 * open
 */
void
TargetDialogUpdate();

#endif
