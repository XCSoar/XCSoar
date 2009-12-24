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

#include "DataField/Enum.hpp"

#include <stdlib.h>

#ifndef WIN32
#define _cdecl
#endif

DataFieldEnum::~DataFieldEnum()
{
  for (unsigned int i=0; i<nEnums; i++) {
    if (mEntries[i].mText) {
      free(mEntries[i].mText);
      mEntries[i].mText= NULL;
    }
  }
  nEnums = 0;
}

int
DataFieldEnum::GetAsInteger(void) const
{
  if (mValue <nEnums) {
    return mEntries[mValue].index;
  } else {
    return 0; // JMW shouldn't get here
  }
}

void DataFieldEnum::addEnumText(const TCHAR *Text) {
  if (nEnums<DFE_MAX_ENUMS-1) {
    mEntries[nEnums].mText = (TCHAR*)malloc((_tcslen(Text)+1)*sizeof(TCHAR));
    _tcscpy(mEntries[nEnums].mText, Text);
    mEntries[nEnums].index = nEnums;
    nEnums++;
  }
}

const TCHAR *
DataFieldEnum::GetAsString(void) const
{
  if (mValue < nEnums) {
    return(mEntries[mValue].mText);
  } else {
    return NULL;
  }
}


void DataFieldEnum::Set(int Value){
  // first look it up
  if (Value<0) {
    Value = 0;
  }
  for (unsigned int i=0; i<nEnums; i++) {
    if (mEntries[i].index == (unsigned int) Value) {
      int lastValue = mValue;
      mValue = i;

      if (mValue != (unsigned int) lastValue){
        if (!GetDetachGUI()) (mOnDataAccess)(this, daChange);
      }
      return;
    }
  }
  mValue = 0; // fallback
}

int DataFieldEnum::SetAsInteger(int Value){
  Set(Value);
  return mEntries[mValue].index;  // this returns incorrect value RLD
  // JMW fixed (was Value, should be mValue)
}

void DataFieldEnum::Inc(void){
  if (mValue<nEnums-1) {
    mValue++;
    if (!GetDetachGUI()) (mOnDataAccess)(this, daChange);
  }
}

void DataFieldEnum::Dec(void){
  if (mValue>0) {
    mValue--;
    if (!GetDetachGUI()) (mOnDataAccess)(this, daChange);
  }
}

static int _cdecl DataFieldEnumCompare(const void *elem1,
                                             const void *elem2 ){
  return _tcscmp(((DataFieldEnumEntry*)elem1)->mText,
                 ((DataFieldEnumEntry*)elem2)->mText);
}

void DataFieldEnum::Sort(int startindex){
  qsort(mEntries+startindex, nEnums-startindex, sizeof(DataFieldEnumEntry),
        DataFieldEnumCompare);
}
int DataFieldEnum::CreateComboList(void) {
  unsigned int i=0;
  for (i=0; i < nEnums; i++){
    mComboList.ComboPopupItemList[i] = mComboList.CreateItem(
                                          i,
                                          mEntries[i].index,
                                          mEntries[i].mText,
                                          mEntries[i].mText);
//    if (mEntries[i].index == mValue) {
//      mComboList.ComboPopupItemSavedIndex=i;
//    }
  }
  mComboList.ComboPopupItemSavedIndex=mValue;
  mComboList.ComboPopupItemCount=i;
  return mComboList.ComboPopupItemCount;
}
