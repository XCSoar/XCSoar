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

#ifndef XCSOAR_DATA_FIELD_NUMBER_HPP
#define XCSOAR_DATA_FIELD_NUMBER_HPP

#include "Base.hpp"
#include "Util/StaticString.hxx"

class NumberDataField : public DataField {
protected:
  StaticString<32> edit_format;
  StaticString<32> display_format;

public:
  void SetFormat(const TCHAR *text);

protected:
  NumberDataField(Type type, bool support_combo,
                  const TCHAR *edit_format, const TCHAR *display_format,
                  DataFieldListener *listener=nullptr);
};

#endif
