#ifndef XCSOAR_DATA_FIELD_STRING_HPP
#define XCSOAR_DATA_FIELD_STRING_HPP

#include "DataField/Base.hpp"

#define EDITSTRINGSIZE 32

class DataFieldString:public DataField{

  private:
    TCHAR mValue[EDITSTRINGSIZE];

  public:
    DataFieldString(const TCHAR *EditFormat, const TCHAR *DisplayFormat,
                    const TCHAR *Default,
                    void(*OnDataAccess)(DataField *Sender,
                                        DataAccessKind_t Mode)):
      DataField(EditFormat, DisplayFormat, OnDataAccess){
      _tcscpy(mValue, Default);
      SupportCombo=false;
    };

  TCHAR *SetAsString(const TCHAR *Value);
  #if defined(__BORLANDC__)
  #pragma warn -hid
  #endif
  void Set(const TCHAR *Value);
  #if defined(__BORLANDC__)
  #pragma warn +hid
  #endif
  TCHAR *GetAsString(void);
  TCHAR *GetAsDisplayString(void);

};

#endif
