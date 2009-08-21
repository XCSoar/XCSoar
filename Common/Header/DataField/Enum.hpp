/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#ifndef XCSOAR_DATA_FIELD_BOOLEAN_HPP
#define XCSOAR_DATA_FIELD_BOOLEAN_HPP

#include "DataField/Base.hpp"

#define DFE_MAX_ENUMS 100

typedef struct {
  TCHAR *mText;
  unsigned int index;
} DataFieldEnumEntry;

class DataFieldEnum: public DataField {

  private:
    unsigned int nEnums;
    unsigned int mValue;
    DataFieldEnumEntry mEntries[DFE_MAX_ENUMS];

  public:
    DataFieldEnum(const TCHAR *EditFormat,
		  const TCHAR *DisplayFormat,
		  int Default,
		  void(*OnDataAccess)(DataField *Sender,
				      DataAccessKind_t Mode)):
      DataField(EditFormat, DisplayFormat, OnDataAccess){
      SupportCombo=true;

      if (Default>=0)
	{ mValue = Default; }
      else
	{mValue = 0;}
      nEnums = 0;
      if (mOnDataAccess) {
	(mOnDataAccess)(this, daGet);
      }
    };
      ~DataFieldEnum();

  void Inc(void);
  void Dec(void);
  int CreateComboList(void);

  void addEnumText(const TCHAR *Text);

  int GetAsInteger(void);
  TCHAR *GetAsString(void);
  TCHAR *GetAsDisplayString(void){
    return(GetAsString());
  };

  #if defined(__BORLANDC__)
  #pragma warn -hid
  #endif
  void Set(int Value);
  #if defined(__BORLANDC__)
  #pragma warn +hid
  #endif
  int SetAsInteger(int Value);
  void Sort(int startindex=0);
};

#endif
