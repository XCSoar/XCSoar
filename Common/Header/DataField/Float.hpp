#ifndef XCSOAR_DATA_FIELD_FLOAT_HPP
#define XCSOAR_DATA_FIELD_FLOAT_HPP

#include "DataField/Base.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class DataFieldFloat:public DataField{

  private:
    double mValue;
    double mMin;
    double mMax;
    double mStep;
    DWORD mTmLastStep;
    int mSpeedup;
    int mFine;
    TCHAR mOutBuf[OUTBUFFERSIZE+1];


  protected:
    double SpeedUp(bool keyup);


  public:
    DataFieldFloat(TCHAR *EditFormat, TCHAR *DisplayFormat, double Min, double Max, double Default, double Step, int Fine, void(*OnDataAccess)(DataField *Sender, DataAccessKind_t Mode)):
      DataField(EditFormat, DisplayFormat, OnDataAccess){
      mMin = Min;
      mMax = Max;
      mValue = Default;
      mStep = Step;
      mFine = Fine;

      SupportCombo=true;
     (mOnDataAccess)(this, daGet);

    };

  void Inc(void);
  void Dec(void);
  int CreateComboList(void);
  int SetFromCombo(int iDataFieldIndex, TCHAR *sValue);

  bool GetAsBoolean(void);
  int GetAsInteger(void);
  double GetAsFloat(void);
  TCHAR *GetAsString(void);
  TCHAR *GetAsDisplayString(void);

  virtual void Set(int Value){ Set((double)Value); };

  #if defined(__BORLANDC__)
  #pragma warn -hid
  #endif
  void Set(double Value);
  double SetMin(double Value);
  double SetMax(double Value);
  #if defined(__BORLANDC__)
  #pragma warn +hid
  #endif
  bool SetAsBoolean(bool Value);
  int SetAsInteger(int Value);
  double SetAsFloat(double Value);
  TCHAR *SetAsString(const TCHAR *Value);

};

#endif
