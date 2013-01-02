/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Base.hpp"
#include "Listener.hpp"
#include "Util/StringUtil.hpp"
#include "Compiler.h"

#include <math.h>

enum {
  ComboPopupITEMMAX = 100
};

DataField::DataField(Type _type, bool _supports_combolist,
                     DataFieldListener *_listener)
  :listener(_listener), data_access_callback(nullptr),
   supports_combolist(_supports_combolist), type(_type),
   item_help_enabled(false), detach_gui(false)
{
}

DataField::DataField(Type _type, bool _supports_combolist,
                     DataAccessCallback _data_access_callback)
  :listener(NULL), data_access_callback(_data_access_callback),
   supports_combolist(_supports_combolist), type(_type),
   item_help_enabled(false), detach_gui(false)
{
}

void
DataField::Modified()
{
  if (GetDetachGUI())
    return;

  if (listener != NULL)
    listener->OnModified(*this);
  else if (data_access_callback != NULL)
    data_access_callback(this, daChange);
}

void
DataField::Special()
{
  if (listener != NULL)
    listener->OnSpecial(*this);
  else if (data_access_callback != NULL)
    data_access_callback(this, daSpecial);
}

void
DataField::Inc()
{
}

void
DataField::Dec()
{
}

int
DataField::GetAsInteger() const
{
  return 0;
}

const TCHAR *
DataField::GetAsString() const
{
  return NULL;
}

const TCHAR *
DataField::GetAsDisplayString() const
{
  return GetAsString();
}

void
DataField::SetAsInteger(gcc_unused int value)
{
}

void
DataField::SetAsString(gcc_unused const TCHAR *value)
{
}

void
DataField::CopyString(TCHAR *buffer, bool formatted)
{
  const TCHAR *src = formatted ? GetAsDisplayString() : GetAsString();
  if (src == NULL)
    src = _T("");

  ::CopyString(buffer, src, ComboPopupITEMMAX);
}
