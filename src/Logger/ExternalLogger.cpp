/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Logger/ExternalLogger.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Dialogs/Message.hpp"
#include "Language.hpp"
#include "Device/device.hpp"
#include "Device/Descriptor.hpp"
#include "Device/List.hpp"

static bool DeclaredToDevice = false;

bool
ExternalLogger::IsDeclared()
{
  return DeclaredToDevice;
}

static bool
DeviceDeclare(DeviceDescriptor *dev, const Declaration &decl)
{
  if (!devIsLogger(*dev))
    return false;

  if (MessageBoxX(_("Declare Task?"),
                  dev->GetName(), MB_YESNO| MB_ICONQUESTION) == IDYES) {
    if (devDeclare(*dev, &decl)) {
      MessageBoxX(_("Task Declared!"),
                  dev->GetName(), MB_OK| MB_ICONINFORMATION);
      DeclaredToDevice = true;
    } else {
      MessageBoxX(_("Error occured,\nTask NOT Declared!"),
                  dev->GetName(), MB_OK| MB_ICONERROR);
    }
  }

  return true;
}

void
ExternalLogger::Declare(const OrderedTask& task)
{
  DeclaredToDevice = false;
  bool found_logger = false;

  // don't do anything if task is not valid
  if (!task.check_task())
    return;

  const Declaration decl(&task);

  for (unsigned i = 0; i < NUMDEV; ++i)
    if (DeviceDeclare(&DeviceList[i], decl))
      found_logger = true;

  if (!found_logger)
    MessageBoxX(_("No logger connected"),
                _("Declare task"), MB_OK | MB_ICONINFORMATION);
}

/**
 * Checks whether a Task is declared to the Logger.
 * If so, asks whether to invalidate the declaration.
 * @return True if a Task is NOT declared to the Logger, False otherwise
 */
bool
ExternalLogger::CheckDeclaration(void)
{
  // if (Task is not declared) -> return true;
  if (!IsDeclared())
    return true;

  if (MessageBoxX(_("OK to invalidate declaration?"),
                  _("Task declared"),
     MB_YESNO| MB_ICONQUESTION) == IDYES){
    DeclaredToDevice = false;
    return true;
  }

  return false;
}
