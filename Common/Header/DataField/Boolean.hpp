#ifndef XCSOAR_DATA_FIELD_ENUM_HPP
#define XCSOAR_DATA_FIELD_ENUM_HPP

#include "DataField/Base.hpp"

class DataFieldBoolean:public DataField{

  private:
    bool mValue;
    TCHAR mTextTrue[FORMATSIZE+1];
    TCHAR mTextFalse[FORMATSIZE+1];

  public:
    DataFieldBoolean(const TCHAR *EditFormat, const TCHAR *DisplayFormat,
                     int Default,
                     const TCHAR *TextTrue, const TCHAR *TextFalse,
                     void(*OnDataAccess)(DataField *Sender,
                                         DataAccessKind_t Mode)):
      DataField(EditFormat, DisplayFormat, OnDataAccess){
		  if (Default) {mValue=true;} else {mValue=false;}
      _tcscpy(mTextTrue, TextTrue);
      _tcscpy(mTextFalse, TextFalse);
      SupportCombo=true;

     (mOnDataAccess)(this, daGet);

    };

  void Inc(void);
  void Dec(void);
  int CreateComboList(void);

  bool GetAsBoolean(void);
  int GetAsInteger(void);
  double GetAsFloat(void);
  TCHAR *GetAsString(void);
  TCHAR *GetAsDisplayString(void){
    return(GetAsString());
  };

  virtual void Set(int Value){
    if (Value>0)
      Set(true);
    else
      Set(false);
  };

  #if defined(__BORLANDC__)
  #pragma warn -hid
  #endif
  void Set(bool Value);
  #if defined(__BORLANDC__)
  #pragma warn +hid
  #endif
  bool SetAsBoolean(bool Value);
  int SetAsInteger(int Value);
  double SetAsFloat(double Value);
  TCHAR *SetAsString(const TCHAR *Value);

};

#endif
