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

#ifndef XCSOAR_FLARM_NAME_DATABASE_HPP
#define XCSOAR_FLARM_NAME_DATABASE_HPP

#include "Util/StaticString.hxx"
#include "Util/StaticArray.hxx"
#include "FlarmId.hpp"
#include "Compiler.h"

#include <assert.h>
#include <tchar.h>

class FlarmNameDatabase {
public:
  struct Record {
    FlarmId id;
    StaticString<21> name;

    Record() = default;
    Record(FlarmId _id, const TCHAR *_name)
      :id(_id), name(_name) {}
  };

private:
  typedef StaticArray<Record, 200> Array;
  typedef Array::iterator iterator;

  Array data;

public:
  typedef Array::const_iterator const_iterator;

  gcc_pure
  const_iterator begin() const {
    return data.begin();
  }

  gcc_pure
  const_iterator end() const {
    return data.end();
  }

  gcc_pure
  const TCHAR *Get(FlarmId id) const;

  gcc_pure
  FlarmId Get(const TCHAR *name) const;

  /**
   * Look up all records with the specified name.
   *
   * @param max the maximum size of the given buffer
   * @return the number of items copied to the given buffer
   */
  unsigned Get(const TCHAR *name, FlarmId *buffer, unsigned max) const;

  bool Set(FlarmId id, const TCHAR *name);

protected:
  gcc_pure
  int Find(FlarmId id) const;

  gcc_pure
  int Find(const TCHAR *name) const;
};

#endif
