#include "DataField/Integer.hpp"
#include "Math/FastMath.h"
#include "Compatibility/string.h"

#include <stdlib.h>
#include <stdio.h>

static bool DataFieldKeyUp = false;

bool DataFieldInteger::GetAsBoolean(void){
  return(mValue != 0);
}

int DataFieldInteger::GetAsInteger(void){
  return(mValue);
}

double DataFieldInteger::GetAsFloat(void){
  return(mValue);
}

TCHAR *DataFieldInteger::GetAsString(void){
  _stprintf(mOutBuf, mEditFormat, mValue);
  return(mOutBuf);
}

TCHAR *DataFieldInteger::GetAsDisplayString(void){
  _stprintf(mOutBuf, mDisplayFormat, mValue, mUnits);
  return(mOutBuf);
}


void DataFieldInteger::Set(int Value){
  mValue = Value;
}

bool DataFieldInteger::SetAsBoolean(bool Value){
  bool res = GetAsBoolean();
  if (Value)
    SetAsInteger(1);
  else
    SetAsInteger(0);
  return(res);
}

int DataFieldInteger::SetAsInteger(int Value){
  int res = mValue;
  if (Value < mMin)
    Value = mMin;
  if (Value > mMax)
    Value = mMax;
  if (mValue != Value){
    mValue = Value;
    if (!GetDetachGUI()) (mOnDataAccess)(this, daChange);
  }
  return(res);
}

double DataFieldInteger::SetAsFloat(double Value){
  double res = GetAsFloat();
  SetAsInteger(iround(Value));
  return(res);
}

TCHAR *DataFieldInteger::SetAsString(const TCHAR *Value){
  TCHAR *res = GetAsString();
  SetAsInteger(_ttoi(Value));
  return(res);
}

void DataFieldInteger::Inc(void){
  SetAsInteger(mValue + mStep*SpeedUp(true));
}

void DataFieldInteger::Dec(void){
  SetAsInteger(mValue - mStep*SpeedUp(false));
}

int DataFieldInteger::SpeedUp(bool keyup){
  int res=1;

#ifdef GNAV
  return res;
#endif

  if (GetDisableSpeedUp() == true)
    return 1;

  if (keyup != DataFieldKeyUp) {
    mSpeedup = 0;
    DataFieldKeyUp = keyup;
    mTmLastStep = GetTickCount();
    return 1;
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
int DataFieldInteger::CreateComboList(void) {
  return CreateComboListStepping();
}
