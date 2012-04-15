/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifndef XCSOAR_TASK_ACCESSOR_HPP
#define XCSOAR_TASK_ACCESSOR_HPP

struct GeoPoint;
struct ElementStat;
class GlidePolar;

class TaskAccessor {
public:
  gcc_pure
  virtual bool is_ordered() const = 0;

  gcc_pure
  virtual bool is_empty() const = 0;

  gcc_pure
  virtual bool is_finished() const = 0;

  gcc_pure
  virtual bool is_started() const = 0;

  gcc_pure
  virtual GeoPoint random_oz_point(unsigned index, const fixed noise) const = 0;

  gcc_pure
  virtual unsigned size() const = 0;

  gcc_pure
  virtual GeoPoint getActiveTaskPointLocation() const = 0;

  gcc_pure
  virtual bool has_entered(unsigned index) const = 0;

  gcc_pure
  virtual const ElementStat leg_stats() const = 0;

  gcc_pure
  virtual fixed target_height() const = 0;

  gcc_pure
  virtual fixed remaining_alt_difference() const = 0;

  gcc_pure
  virtual GlidePolar get_glide_polar() const =0;

  gcc_pure
  virtual void setActiveTaskPoint(unsigned index) = 0;

  gcc_pure
  virtual unsigned getActiveTaskPointIndex() const = 0;
};

#endif
