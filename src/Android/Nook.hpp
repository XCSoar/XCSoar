/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_ANDROID_NOOK_HPP
#define XCSOAR_ANDROID_NOOK_HPP

namespace Nook {
  /**
   * initialize USB mode in Nook (must be rooted with USB Kernel)
   */
  void InitUsb();

  /**
   * Enter FastMode to eliminate full refresh of screen
   * requires Nook kernel rooted to support FastMode
   *
   * @return false if the operation has failed, e.g. because the
   * kernel does not support the mode
   */
  bool EnterFastMode();

  /**
   * Exit FastMode to restore full (slow) refresh of screen
   * requires Nook kernel rooted to support FastMode
   */
  void ExitFastMode();

  /**
   * Set Nook regulator's charge rate to 500mA.
   */
  void SetCharge500();

  /**
   * Set Nook regulator's charge rate to 100mA.
   */
  void SetCharge100();
}

#endif
