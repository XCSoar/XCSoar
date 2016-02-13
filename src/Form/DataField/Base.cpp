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

#include "Base.hpp"
#include "Listener.hpp"
#include "ComboList.hpp"
#include "Compiler.h"

#include <math.h>

DataField::DataField(Type _type, bool _supports_combolist,
                     DataFieldListener *_listener)
  :listener(_listener),
   supports_combolist(_supports_combolist), type(_type),
   item_help_enabled(false)
{
}

void
DataField::Modified()
{
  if (listener != nullptr)
    listener->OnModified(*this);
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
  return nullptr;
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

ComboList
DataField::CreateComboList(const TCHAR *reference) const
{
  return ComboList();
}
