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

#ifndef ABSTRACT_REPLAY_HPP
#define ABSTRACT_REPLAY_HPP

#include "Math/fixed.hpp"

class AbstractReplay 
{
public:
  fixed time_scale;

protected:
  bool enabled;

public:
  AbstractReplay(): time_scale(fixed_one), enabled(false) {}
  virtual bool Update() = 0;
  virtual void Stop() = 0;
  virtual void Start() = 0;

  bool IsEnabled() const {
    return enabled;
  }

protected:
  virtual bool UpdateTime() = 0;
  virtual void ResetTime() = 0;
};

#endif
