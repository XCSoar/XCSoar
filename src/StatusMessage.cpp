/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "StatusMessage.hpp"
#include "Util/StringAPI.hpp"

static constexpr StatusMessage default_status_messages[] = {
#include "Status_defaults.cpp"
  { nullptr }
};

StatusMessageList::StatusMessageList()
{
  // DEFAULT - 0 is loaded as default, and assumed to exist
  StatusMessage &first = list.append();
  first.key = _T("DEFAULT");
  first.visible = true;
  first.sound = _T("IDR_WAV_DRIP");
  first.delay_ms = 2500; // 2.5 s

  // Load up other defaults - allow overwrite in config file
  const StatusMessage *src = &default_status_messages[0];
  while (src->key != NULL)
    list.append(*src++);
}

const StatusMessage *
StatusMessageList::Find(const TCHAR *key) const
{
  for (int i = list.size() - 1; i > 0; i--)
    if (StringIsEqual(key, list[i].key))
      return &list[i];

  return NULL;
}
