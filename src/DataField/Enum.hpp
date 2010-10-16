/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
    unsigned int index;

    Entry():mText(NULL) {}
    ~Entry();
  };

private:
  StaticArray<Entry, 128> entries;
  unsigned int mValue;

public:
  DataFieldEnum(int Default, DataAccessCallback_t OnDataAccess) :
    DataField(_T(""), _T(""), OnDataAccess),
    mValue(Default >= 0 ? Default : 0)
  {
    SupportCombo = true;
  }

  void Inc(void);
  void Dec(void);
  virtual ComboList *CreateComboList() const;

  void replaceEnumText(unsigned int i, const TCHAR *Text);
  unsigned addEnumText(const TCHAR *Text);
  void addEnumTexts(const TCHAR *const*list);

  virtual int GetAsInteger() const;
  virtual const TCHAR *GetAsString() const;

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

  unsigned Count() const {
    return entries.size();
  }

  /**
   * Finds an entry with the specified text.  Returns -1 if not found.
   */
  int Find(const TCHAR *text) const;
};

#endif
