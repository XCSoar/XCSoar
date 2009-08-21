#include "DataField/Enum.hpp"

#include <stdlib.h>

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

int DataFieldEnum::GetAsInteger(void){
  if ((mValue>=0) && (mValue<nEnums)) {
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


TCHAR *DataFieldEnum::GetAsString(void){
  if ((mValue>=0) && (mValue<nEnums)) {
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
