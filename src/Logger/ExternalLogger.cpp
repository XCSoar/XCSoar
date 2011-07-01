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

#include "Logger/ExternalLogger.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "Device/device.hpp"
#include "Device/Declaration.hpp"
#include "Device/Descriptor.hpp"
#include "Device/List.hpp"
#include "Profile/DeclarationConfig.hpp"

static bool DeclaredToDevice = false;

bool
ExternalLogger::IsDeclared()
{
  return DeclaredToDevice;
}

static bool
DeviceDeclare(DeviceDescriptor *dev, const Declaration &declaration)
{
  if (!dev->IsLogger())
    return false;

  if (MessageBoxX(_("Declare task?"),
                  dev->GetDisplayName(), MB_YESNO| MB_ICONQUESTION) == IDYES) {
    if (dev->Declare(declaration)) {
      MessageBoxX(_("Task declared!"),
                  dev->GetDisplayName(), MB_OK| MB_ICONINFORMATION);
      DeclaredToDevice = true;
    } else {
      MessageBoxX(_("Error occured,\nTask NOT declared!"),
                  dev->GetDisplayName(), MB_OK| MB_ICONERROR);
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

  Declaration decl(&task);
  Profile::GetDeclarationConfig(decl);

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
