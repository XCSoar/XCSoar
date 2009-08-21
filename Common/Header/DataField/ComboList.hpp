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

#ifndef XCSOAR_DATA_FIELD_COMBO_LIST_HPP
#define XCSOAR_DATA_FIELD_COMBO_LIST_HPP

#include <tchar.h>

    typedef struct {
      int ItemIndex;
      int DataFieldIndex;
      TCHAR *StringValue;
      TCHAR *StringValueFormatted;
    } ComboListEntry_t;

class ComboList{

  public:

    ComboList(void) {
      ComboPopupDrawListIndex=0;
      ComboPopupItemIndex=-1;
      ComboPopupItemSavedIndex=-1;
    }

#define ComboPopupLISTMAX 300
#define ComboPopupITEMMAX 100
#define ComboPopupReopenMOREDataIndex -800001
#define ComboPopupReopenLESSDataIndex -800002
#define ComboPopupNULL -800003

    ComboListEntry_t *CreateItem(int ItemIndex, int DataFieldIndex,
                                 const TCHAR *StringValue,
                                 const TCHAR *StringValueFormatted);
    void FreeComboPopupItemList(void);

    int ComboPopupDrawListIndex;
    int ComboPopupItemIndex;
    int ComboPopupItemSavedIndex;
    int ComboPopupItemCount;
    ComboListEntry_t * ComboPopupItemList[ComboPopupLISTMAX]; // RLD make this dynamic later

    int PropertyDataFieldIndexSaved;
    TCHAR PropertyValueSaved[ComboPopupITEMMAX];
    TCHAR PropertyValueSavedFormatted[ComboPopupITEMMAX];


  private:

};

#endif
