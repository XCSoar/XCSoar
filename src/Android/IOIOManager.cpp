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

#include "Android/IOIOManager.hpp"
#include "Java/String.hpp"
#include "Java/Class.hpp"

IOIOManager::IOIOManager(JNIEnv *env):
  ref_count(0), helper(NULL)
{
  helper = new IOIOHelper(env);
  assert(helper != NULL);
}

IOIOHelper*
IOIOManager::AddClient()
{
  ref_count_mutex.Lock();
  const unsigned temp_ref = ++ref_count;
  ref_count_mutex.Unlock();

  if (temp_ref == 1) {
    helper->open(Java::GetEnv());
  }
  return helper;
}

void
IOIOManager::RemoveClient()
{
  ref_count_mutex.Lock();
  assert(ref_count > 0);
  const unsigned temp_ref = --ref_count;
  ref_count_mutex.Unlock();

  if (temp_ref == 0)
    helper->close();
}
