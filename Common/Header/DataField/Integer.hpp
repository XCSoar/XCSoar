#ifndef XCSOAR_DATA_FIELD_INTEGER_HPP
#define XCSOAR_DATA_FIELD_INTEGER_HPP

#include "DataField/Base.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class DataFieldInteger:public DataField{

  private:
    int mValue;
    int mMin;
    int mMax;
    int mStep;
    DWORD mTmLastStep;
    int mSpeedup;
    TCHAR mOutBuf[OUTBUFFERSIZE+1];

  protected:
    int SpeedUp(bool keyup);
  public:
    DataFieldInteger(TCHAR *EditFormat, TCHAR *DisplayFormat, int Min, int Max, int Default, int Step, void(*OnDataAccess)(DataField *Sender, DataAccessKind_t Mode)):
      DataField(EditFormat, DisplayFormat, OnDataAccess){
      mMin = Min;
      mMax = Max;
      mValue = Default;
      mStep = Step;

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
  TCHAR *GetAsDisplayString(void);

  #if defined(__BORLANDC__)
  #pragma warn -hid
  #endif
  void Set(int Value);
  int SetMin(int Value){mMin=Value; return(mMin);};
  int SetMax(int Value){mMax=Value; return(mMax);};
  #if defined(__BORLANDC__)
  #pragma warn +hid
  #endif
  bool SetAsBoolean(bool Value);
  int SetAsInteger(int Value);
  double SetAsFloat(double Value);
  TCHAR *SetAsString(const TCHAR *Value);

};

#endif
