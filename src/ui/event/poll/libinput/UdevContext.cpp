/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "UdevContext.hpp"
#include "thread/Mutex.hxx"

#include <libudev.h>

#include <cassert>

static UdevContext *udev_root_context = nullptr;
static Mutex udev_context_mutex;

UdevContext::UdevContext(const UdevContext &other)
{
  std::lock_guard<Mutex> lock(udev_context_mutex);
  if (other.ud) {
    ud = udev_ref(other.ud);
    assert(ud);
  } else {
    ud = nullptr;
  }
}

UdevContext::~UdevContext()
{
  std::lock_guard<Mutex> lock(udev_context_mutex);
  if (nullptr != ud)
    udev_unref(ud);
}

UdevContext &
UdevContext::operator=(const UdevContext &other)
{
  if (this != &other) {
    std::lock_guard<Mutex> lock(udev_context_mutex);
    struct udev *new_ud = (nullptr == other.ud) ? udev_ref(other.ud) : nullptr;
    assert(nullptr == other.ud || nullptr != new_ud);
    if (ud != nullptr)
      udev_unref(ud);
    ud = new_ud;
  }

  return *this;
}

UdevContext
UdevContext::NewRef()
{
  {
    std::lock_guard<Mutex> lock(udev_context_mutex);
    if (nullptr == udev_root_context) {
      udev_root_context = new UdevContext(udev_new());
      assert(udev_root_context);
    }
  }

  return UdevContext(*udev_root_context);
}
