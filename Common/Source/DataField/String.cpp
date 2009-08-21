#include "DataField/String.hpp"

TCHAR *DataFieldString::SetAsString(const TCHAR *Value){
  _tcscpy(mValue, Value);
  return(mValue);
}

void DataFieldString::Set(const TCHAR *Value){
  _tcscpy(mValue, Value);
}

TCHAR *DataFieldString::GetAsString(void){
  return(mValue);
}

TCHAR *DataFieldString::GetAsDisplayString(void){
  return(mValue);
}
