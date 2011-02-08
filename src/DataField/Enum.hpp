/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_DATA_FIELD_ENUM_HPP
#define XCSOAR_DATA_FIELD_ENUM_HPP

#include "DataField/Base.hpp"
#include "Util/NonCopyable.hpp"
#include "Util/StaticArray.hpp"

class DataFieldEnum: public DataField
{
public:
  struct Entry : private NonCopyable {
    TCHAR *mText;
    unsigned id;
    TCHAR *mHelp;

    Entry():mText(NULL), mHelp(NULL) {}
    ~Entry();
  };

private:
  StaticArray<Entry, 128> entries;
  unsigned int mValue;

public:
  DataFieldEnum(DataAccessCallback_t OnDataAccess) :
    DataField(_T(""), _T(""), OnDataAccess),
    mValue(0)
  {
    SupportCombo = true;
  }

  gcc_pure
  bool Exists(const TCHAR *text) const {
    return Find(text) >= 0;
  }

  void Inc(void);
  void Dec(void);
  virtual ComboList *CreateComboList() const;

  void replaceEnumText(unsigned int i, const TCHAR *Text);
  bool addEnumText(const TCHAR *Text, unsigned id, const TCHAR *ItemHelpText = NULL);
  unsigned addEnumText(const TCHAR *Text, const TCHAR *ItemHelpText = NULL);
  void addEnumTexts(const TCHAR *const*list);

  gcc_pure
  virtual int GetAsInteger() const;

  gcc_pure
  virtual const TCHAR *GetAsString() const;

  /**
   * @param value True if display item help in text box below picker
   * Displays help strings associated with enums Items
   */
  void EnableItemHelp(bool value) { mItemHelp = value; }


  #if defined(__BORLANDC__)
  #pragma warn -hid
  #endif

  void Set(int Value);

  #if defined(__BORLANDC__)
  #pragma warn +hid
  #endif

  virtual void SetAsInteger(int Value);
  virtual void SetAsString(const TCHAR *Value);
  void Sort(int startindex = 0);

  gcc_pure
  unsigned Count() const {
    return entries.size();
  }
  unsigned getItem(unsigned index) const;

protected:
  /**
   * Finds an entry with the specified text.  Returns -1 if not found.
   */
  gcc_pure
  int Find(const TCHAR *text) const;

  gcc_pure
  int Find(unsigned id) const;

  void SetIndex(unsigned new_value);
};

#endif
