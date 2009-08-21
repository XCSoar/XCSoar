#include "DataField/Boolean.hpp"

int DataFieldBoolean::CreateComboList(void) {
  int i=0;
  mComboList.ComboPopupItemList[i] = mComboList.CreateItem(i,
                                                  i,
                                                  mTextFalse,
                                                  mTextFalse);

  i=1;
  mComboList.ComboPopupItemList[i] = mComboList.CreateItem(i,
                                                  i,
                                                  mTextTrue,
                                                  mTextTrue);
  mComboList.ComboPopupItemCount=2;
  mComboList.ComboPopupItemSavedIndex=GetAsInteger();
  return mComboList.ComboPopupItemCount;
}

bool DataFieldBoolean::GetAsBoolean(void){
  return(mValue);
}

int DataFieldBoolean::GetAsInteger(void){
  if (mValue)
    return(1);
  else
    return(0);
}

double DataFieldBoolean::GetAsFloat(void){
  if (mValue)
    return(1.0);
  else
    return(0.0);
}

TCHAR *DataFieldBoolean::GetAsString(void){
  if (mValue)
    return(mTextTrue);
  else
    return(mTextFalse);
}


void DataFieldBoolean::Set(bool Value){
  mValue = Value;
}

bool DataFieldBoolean::SetAsBoolean(bool Value){
  bool res = mValue;
  if (mValue != Value){
    mValue = Value;
    if (!GetDetachGUI()) (mOnDataAccess)(this, daChange);
  }
  return(res);
}

int DataFieldBoolean::SetAsInteger(int Value){
  int res = GetAsInteger();
  if (GetAsInteger() != Value){
    SetAsBoolean(!(Value==0));
  }
  return(res);
}

double DataFieldBoolean::SetAsFloat(double Value){
  double res = GetAsFloat();
  if (GetAsFloat() != Value){
    SetAsBoolean(!(Value==0.0));
  }
  return(res);
}

TCHAR *DataFieldBoolean::SetAsString(const TCHAR *Value){
  TCHAR *res = GetAsString();
  if (_tcscmp(res, Value) != 0){
    SetAsBoolean(_tcscmp(Value, mTextTrue) == 0);
  }
  return(res);
}

void DataFieldBoolean::Inc(void){
  SetAsBoolean(!GetAsBoolean());
}

void DataFieldBoolean::Dec(void){
  SetAsBoolean(!GetAsBoolean());
}
