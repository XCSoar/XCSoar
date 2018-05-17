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

#ifndef XCSOAR_DATA_FIELD_PREFIX_HPP
#define XCSOAR_DATA_FIELD_PREFIX_HPP

#include "String.hpp"

#include <functional>

class PrefixDataField final : public DataFieldString {
public:
  typedef std::function<const TCHAR *(const TCHAR *)> AllowedCharactersFunction;

private:
  AllowedCharactersFunction allowed_characters;

public:
  PrefixDataField(const TCHAR *value,
                  AllowedCharactersFunction _allowed_characters,
                  DataFieldListener *listener=nullptr)
    :DataFieldString(Type::PREFIX, value, listener),
     allowed_characters(_allowed_characters) {}

  PrefixDataField(const TCHAR *value=_T(""),
                  DataFieldListener *listener=nullptr)
    :DataFieldString(Type::PREFIX, value, listener) {}

  const AllowedCharactersFunction &GetAllowedCharactersFunction() const {
    return allowed_characters;
  }

  /* virtual methods from class DataField */
  void Inc() override;
  void Dec() override;
  const TCHAR *GetAsDisplayString() const override;

protected:
  gcc_pure
  const TCHAR *GetAllowedCharacters() const {
    return allowed_characters
      ? allowed_characters(_T(""))
      : _T("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  }
};

#endif
