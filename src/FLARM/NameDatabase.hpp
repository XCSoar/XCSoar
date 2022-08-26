/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#include "util/StaticString.hxx"
#include "util/StaticArray.hxx"
#include "FlarmId.hpp"

#include <cassert>
#include <tchar.h>

class FlarmNameDatabase {
public:
  struct Record {
    FlarmId id;
    StaticString<21> name;

    Record() = default;
    Record(FlarmId _id, const TCHAR *_name) noexcept
      :id(_id), name(_name) {}
  };

private:
  using Array = StaticArray<Record, 200>;
  using iterator = Array::iterator;

  Array data;

public:
  using const_iterator = Array::const_iterator;

  [[gnu::pure]]
  const_iterator begin() const noexcept {
    return data.begin();
  }

  [[gnu::pure]]
  const_iterator end() const noexcept {
    return data.end();
  }

  [[gnu::pure]]
  const TCHAR *Get(FlarmId id) const noexcept;

  [[gnu::pure]]
  FlarmId Get(const TCHAR *name) const noexcept;

  /**
   * Look up all records with the specified name.
   *
   * @param max the maximum size of the given buffer
   * @return the number of items copied to the given buffer
   */
  unsigned Get(const TCHAR *name,
               FlarmId *buffer, unsigned max) const noexcept;

  bool Set(FlarmId id, const TCHAR *name) noexcept;

protected:
  [[gnu::pure]]
  int Find(FlarmId id) const noexcept;

  [[gnu::pure]]
  int Find(const TCHAR *name) const noexcept;
};
