#include "DataField/Float.hpp"
#include "Math/FastMath.h"

#include <stdlib.h>
#include <stdio.h>

static bool DataFieldKeyUp = false;

bool DataFieldFloat::GetAsBoolean(void){
  return(mValue != 0.0);
}

int DataFieldFloat::GetAsInteger(void){
  return iround(mValue);
}

double DataFieldFloat::GetAsFloat(void){
  return(mValue);
}

TCHAR *DataFieldFloat::GetAsString(void){
  _stprintf(mOutBuf, mEditFormat, mValue);
  return(mOutBuf);
}

TCHAR *DataFieldFloat::GetAsDisplayString(void){
  _stprintf(mOutBuf, mDisplayFormat, mValue, mUnits);
  return(mOutBuf);
}

void DataFieldFloat::Set(double Value){
  mValue = Value;
}

double DataFieldFloat::SetMin(double Value){
  double res = mMin;
  mMin = Value;
  return(res);
};
double DataFieldFloat::SetMax(double Value){
  double res = mMax;
  mMax = Value;
  return(res);
};

bool DataFieldFloat::SetAsBoolean(bool Value){
  bool res = GetAsBoolean();
  if (res != Value){
    if (Value)
      SetAsFloat(1.0);
    else
      SetAsFloat(0.0);
  }
  return(res);
}

int DataFieldFloat::SetAsInteger(int Value){
  int res = GetAsInteger();
  SetAsFloat(Value);
  return(res);
}

double DataFieldFloat::SetAsFloat(double Value){
  double res = mValue;
  if (Value < mMin)
    Value = mMin;
  if (Value > mMax)
    Value = mMax;
  if (res != Value){
    mValue = Value;
    if (!GetDetachGUI()) (mOnDataAccess)(this, daChange);
  }
  return(res);
}

TCHAR *DataFieldFloat::SetAsString(const TCHAR *Value){
  TCHAR *res = GetAsString();
  SetAsFloat(_tcstod(Value, NULL));
  return(res);
}

void DataFieldFloat::Inc(void){
  // no keypad, allow user to scroll small values
  if(mFine && (mValue < 0.95) && (mStep>=0.5) && (mMin>=0.0))
    {
      SetAsFloat(mValue + 0.1);
    }
  else
    SetAsFloat(mValue + mStep*SpeedUp(true));
}

void DataFieldFloat::Dec(void){
  // no keypad, allow user to scroll small values
  if(mFine && (mValue <= 1.0) && (mStep>=0.5) && (mMin>=0.0))
    {
      SetAsFloat(mValue - 0.1);
    }
  else
    SetAsFloat(mValue - mStep*SpeedUp(false));
}

double DataFieldFloat::SpeedUp(bool keyup){
  double res=1.0;

#ifdef GNAV
  return res;
#endif

  if (GetDisableSpeedUp() == true)
    return 1;

  if (keyup != DataFieldKeyUp) {
    mSpeedup = 0;
    DataFieldKeyUp = keyup;
    mTmLastStep = GetTickCount();
    return 1.0;
  }

  if ((long)(GetTickCount()-mTmLastStep) < 200){
    mSpeedup++;

    if (mSpeedup > 5){
      res = 10;

      mTmLastStep = GetTickCount()+350;
      return(res);

    }
  } else
    mSpeedup = 0;

  mTmLastStep = GetTickCount();

  return(res);
}

int DataFieldFloat::SetFromCombo(int iDataFieldIndex, TCHAR *sValue) {
  SetAsString(sValue);
  return 0;
}

int DataFieldFloat::CreateComboList(void) {
  return CreateComboListStepping();
}
