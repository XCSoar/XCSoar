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

#include "DataField/ComboList.hpp"

#include <assert.h>
#include <stdlib.h>

ComboListEntry_t * ComboList::CreateItem(int ItemIndex,
                                        int DataFieldIndex,
                                        const TCHAR *StringValue,
                                        const TCHAR *StringValueFormatted)
{
  int iLen = -1;
  ComboListEntry_t * theItem;

  // Copy current strings into structure
  theItem = (ComboListEntry_t*) malloc(sizeof(ComboListEntry_t));
  theItem->DataFieldIndex=0;  // NULL is same as 0, so it fails to set it if index value is 0
  theItem->ItemIndex=0;

  assert(theItem!= NULL);

  theItem->ItemIndex=ItemIndex;

  if (DataFieldIndex != ComboPopupNULL) { // optional
    theItem->DataFieldIndex=DataFieldIndex;
  }

  if (StringValue == NULL)
  {
    theItem->StringValue = (TCHAR*)malloc((1) * sizeof(TCHAR));
    assert(theItem->StringValue != NULL);
    theItem->StringValue[0]='\0';
  }
  else
  {
    iLen = _tcslen(StringValue);
    theItem->StringValue = (TCHAR*)malloc((iLen + 1) * sizeof(TCHAR));
    assert(theItem->StringValue != NULL);
    _tcscpy(theItem->StringValue, StringValue);
  }


  // copy formatted display string
  if (StringValueFormatted == NULL)
  {
    theItem->StringValueFormatted = (TCHAR*)malloc((1) * sizeof(TCHAR));
    assert(theItem->StringValueFormatted != NULL);
    theItem->StringValueFormatted[0]='\0';
  }
  else
  {
    iLen = _tcslen(StringValueFormatted);
    theItem->StringValueFormatted = (TCHAR*)malloc((iLen + 1) * sizeof(TCHAR));
    assert(theItem->StringValueFormatted != NULL);
    _tcscpy(theItem->StringValueFormatted, StringValueFormatted);
  }

  return theItem;
}

void ComboList::FreeComboPopupItemList(void)
{
  for (int i = 0; i < ComboPopupItemCount && i < ComboPopupLISTMAX; i++)
  {
    if (ComboPopupItemList[i] != NULL)
    {
      free (ComboPopupItemList[i]->StringValue);
      ComboPopupItemList[i]->StringValue=NULL;

      free (ComboPopupItemList[i]->StringValueFormatted);
      ComboPopupItemList[i]->StringValueFormatted=NULL;

      free (ComboPopupItemList[i]);
      ComboPopupItemList[i]=NULL;

    }
  }
}
