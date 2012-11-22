/*
Copyright_License {

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

#ifndef XCSOAR_INFOBOX_FACTORY_HPP
#define XCSOAR_INFOBOX_FACTORY_HPP

#include "Type.hpp"

#include <assert.h>
#include <tchar.h>

class InfoBoxContent;

namespace InfoBoxFactory
{
  struct MetaData {
    const TCHAR *name;
    const TCHAR *caption;
    const TCHAR *description;
    InfoBoxContent *(*Create)();
    Type next, previous;
  };

  extern const MetaData meta_data[NUM_TYPES];

  /**
   * Returns the human-readable name of the info box type.
   */
  static inline const TCHAR *
  GetName(Type type)
  {
    assert(type < NUM_TYPES);

    return meta_data[type].name;
  }

  /**
   * Returns the default caption of the info box type.  This is
   * usually a shorter version of the string returned by GetName(), to
   * fit in the small #InfoBoxWindow.
   */
  static inline const TCHAR *
  GetCaption(Type type)
  {
    assert(type < NUM_TYPES);

    return meta_data[type].caption;
  }

  /**
   * Returns the long description (help text) of the info box type.
   */
  static inline const TCHAR *
  GetDescription(Type type)
  {
    assert(type < NUM_TYPES);

    return meta_data[type].description;
  }

  static inline Type
  GetNext(Type type)
  {
    assert(type < NUM_TYPES);

    return meta_data[type].next;
  }

  static inline Type
  GetPrevious(Type type)
  {
    assert(type < NUM_TYPES);

    return meta_data[type].previous;
  }

  bool
  Get(const TCHAR *key, Type &val);

  InfoBoxContent* Create(Type infobox_type);
};

#endif
