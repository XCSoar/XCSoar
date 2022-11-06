/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

#include <tchar.h>

struct DialogLook;
class PluggableOperationEnvironment;
namespace UI { class SingleWindow; }
namespace Co { class InvokeTask; }

/**
 * Run the specified coroutine in the I/O thread, and show a modal
 * dialog while it is running.
 *
 * Rethrows exceptions thrown by the coroutine.
 *
 * @return true if the job has finished (may have failed), false if
 * the job was cancelled by the user
 */
bool
ShowCoDialog(UI::SingleWindow &parent, const DialogLook &dialog_look,
             const TCHAR *caption, Co::InvokeTask task,
             PluggableOperationEnvironment *env);
