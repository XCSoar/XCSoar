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

#ifndef XCSOAR_DATA_FIELD_BASE_HPP
#define XCSOAR_DATA_FIELD_BASE_HPP

#include "Compiler.h"

#include <assert.h>
#include <tchar.h>
#include <stdint.h>

#define OUTBUFFERSIZE 128

class DataFieldListener;
class ComboList;

class DataField
{
public:
  enum class Type : uint8_t {
    STRING,
    BOOLEAN,
    INTEGER,
    REAL,
    ANGLE,
    ENUM,
    FILE,
    ROUGH_TIME,
    TIME,
    PREFIX,
    GEOPOINT,
  };

private:
  DataFieldListener *listener;

  // all Types dataField support combolist except DataFieldString.
  const bool supports_combolist;

protected:
  const Type type;

  bool item_help_enabled;

protected:
  DataField(Type type, bool supports_combolist,
            DataFieldListener *listener);

public:
  virtual ~DataField() {}

  void SetListener(DataFieldListener *_listener) {
    assert(listener == nullptr);
    assert(_listener != nullptr);

    listener = _listener;
  }

  Type GetType() const {
    return type;
  }

  bool SupportsCombolist() const {
    return supports_combolist;
  }

  virtual void Inc();
  virtual void Dec();

  gcc_pure
  virtual int GetAsInteger() const;

  gcc_pure
  virtual const TCHAR *GetAsString() const;

  gcc_pure
  virtual const TCHAR *GetAsDisplayString() const;

  virtual void SetAsInteger(int value);
  virtual void SetAsString(const TCHAR *value);

  virtual void EnableItemHelp(gcc_unused bool value) {};


  /**
   * Create a #ComboList that allows the user to choose a value.
   *
   * @param reference the reference value for a long list, to
   * determine the range of displayed values; pass nullptr for the
   * "default" reference
   */
  gcc_pure
  virtual ComboList CreateComboList(const TCHAR *reference) const;

  virtual void SetFromCombo(int iDataFieldIndex,
                            gcc_unused const TCHAR *sValue)
  {
    SetAsInteger(iDataFieldIndex);
  }

  bool GetItemHelpEnabled() {
    return item_help_enabled;
  }

protected:
  /**
   * Notify interested parties that the value of this object has
   * been modified.
   */
  void Modified();
};

#endif
