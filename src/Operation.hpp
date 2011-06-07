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

#ifndef XCSOAR_OPERATION_HPP
#define XCSOAR_OPERATION_HPP

#include "Util/NonCopyable.hpp"

/**
 * An environment a complex operation runs in.  The operation may run
 * in a separate thread, and this class provides a bridge to the
 * calling thread.
 */
class OperationEnvironment : private NonCopyable {
public:
  /**
   * Sleep for a fixed amount of time.  May return earlier if an event
   * occurs.
   */
  void Sleep(unsigned ms);
};

#endif
