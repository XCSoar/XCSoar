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
    ENUM,
    FILE,
    TIME,
  };

  enum DataAccessKind_t {
    daChange,
    daSpecial,
  };

  typedef void (*DataAccessCallback_t)(DataField * Sender, DataAccessKind_t Mode);

  DataFieldListener *listener;

  DataAccessCallback_t mOnDataAccess;

  // all Types dataField support combolist except DataFieldString.
  const bool SupportCombo;

protected:
  const Type type;

  bool mItemHelp;

private:
  bool mDetachGUI;

protected:
  DataField(Type _type, bool support_combo,
            DataAccessCallback_t OnDataAccess = NULL);

public:
  virtual ~DataField(void) {}

  void SetListener(DataFieldListener *_listener) {
    assert(mOnDataAccess == NULL);
    assert(listener == NULL);
    assert(_listener != NULL);

    listener = _listener;
  }

  void SetDataAccessCallback(DataAccessCallback_t _data_access_callback) {
    assert(listener == NULL);

    mOnDataAccess = _data_access_callback;
  }

  Type GetType() const {
    return type;
  }

  void Special(void);
  virtual void Inc(void);
  virtual void Dec(void);

  gcc_pure
  virtual int GetAsInteger() const;

  gcc_pure
  virtual const TCHAR *GetAsString() const;

  gcc_pure
  virtual const TCHAR *GetAsDisplayString() const;

  virtual void SetAsInteger(int Value);
  virtual void SetAsString(const TCHAR *Value);

  virtual void EnableItemHelp(bool value) {};

  // allows combolist to iterate all values w/out triggering external events
  void SetDetachGUI(bool bDetachGUI) { mDetachGUI = bDetachGUI; }
  bool GetDetachGUI(void) { return mDetachGUI; }

  gcc_malloc
  virtual ComboList *CreateComboList() const { return NULL; }

  virtual void
  SetFromCombo(int iDataFieldIndex, TCHAR *sValue)
  {
    SetAsInteger(iDataFieldIndex);
  }
  bool GetItemHelpEnabled() { return mItemHelp; }

  void CopyString(TCHAR * szStringOut, bool bFormatted);

protected:
  /**
   * Notify interested parties that the value of this object has
   * been modified.
   */
  void Modified();
};

#endif
