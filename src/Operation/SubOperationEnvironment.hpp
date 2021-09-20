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

#pragma once

#include "ProxyOperationEnvironment.hpp"

/**
 * An #OperationEnvironment implementation which translates progress
 * values.
 */
class SubOperationEnvironment : public ProxyOperationEnvironment {
  const unsigned parent_start, parent_range;

  unsigned range = 0;

public:
  SubOperationEnvironment(OperationEnvironment &_other,
                          unsigned _start, unsigned _end) noexcept
    :ProxyOperationEnvironment(_other),
     parent_start(_start), parent_range(_end - _start) {}

  /* virtual methods from class ProgressListener */
  void SetProgressRange(unsigned range) noexcept override;
  void SetProgressPosition(unsigned position) noexcept override;
};
