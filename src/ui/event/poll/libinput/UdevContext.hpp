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

#ifndef XCSOAR_EVENT_UDEVCONTEXT_HPP
#define XCSOAR_EVENT_UDEVCONTEXT_HPP

static constexpr const char *UDEV_DEFAULT_SEAT = "seat0";

struct udev;

/**
 * Helper class for initialisation and (thread safe) access to the udev context.
 */
class UdevContext {
  struct udev *ud;

  explicit UdevContext(struct udev *_ud):ud(_ud) {}

public:
  UdevContext():ud(nullptr) {}
  UdevContext(const UdevContext &);

  UdevContext(UdevContext &&other)
    :ud(other.ud) {
    other.ud = nullptr;
  }

  UdevContext &operator=(const UdevContext &);
  ~UdevContext();

  struct udev *Get() {
    return ud;
  }

  static UdevContext NewRef();
};

#endif
