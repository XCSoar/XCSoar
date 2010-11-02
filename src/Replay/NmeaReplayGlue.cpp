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

#include "Replay/NmeaReplayGlue.hpp"

#include "Device/List.hpp"
#include "Device/Descriptor.hpp"
#include "Language.hpp"
#include "Dialogs/Message.hpp"

void
NmeaReplayGlue::on_sentence(const char *line)
{
  DeviceList[0].LineReceived(line);
}

void
NmeaReplayGlue::on_bad_file()
{
  MessageBoxX(_("Could not open NMEA file!"),
              _("Flight replay"), MB_OK | MB_ICONINFORMATION);
}

bool
NmeaReplayGlue::update_time()
{
  return clock.check_update(1000);
}

void
NmeaReplayGlue::reset_time()
{
  clock.reset();
}
