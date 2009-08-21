#ifndef XCSOAR_DATA_FIELD_BASE_HPP
#define XCSOAR_DATA_FIELD_BASE_HPP

#include "DataField/ComboList.hpp"

#include <tchar.h>

#define FORMATSIZE 32
#define UNITSIZE 10
#define OUTBUFFERSIZE 128

class DataField{

  public:

    typedef enum{
     daGet,
     daPut,
     daChange,
     daInc,
     daDec,
     daSpecial,
    }DataAccessKind_t;

    typedef void (*DataAccessCallback_t)(DataField * Sender, DataAccessKind_t Mode);

    DataField(const TCHAR *EditFormat, const TCHAR *DisplayFormat,
	      void(*OnDataAccess)(DataField *Sender, DataAccessKind_t Mode)=NULL);
    virtual ~DataField(void){};


  virtual void Special(void);
  virtual void Inc(void);
  virtual void Dec(void);

  virtual void GetData(void);
  virtual void SetData(void);

  virtual bool GetAsBoolean(void){return(false);};
  virtual int GetAsInteger(void){return(0);};
  virtual double GetAsFloat(void){return(0);};
  virtual TCHAR *GetAsString(void){return(NULL);};
  virtual TCHAR *GetAsDisplayString(void){return(NULL);};

  virtual bool SetAsBoolean(bool Value){ (void)Value;
	  return(false);};
	  virtual int SetAsInteger(int Value){ (void)Value;
	  return(0);};
	  virtual double SetAsFloat(double Value){ (void) Value;
	  return(0.0);};
  virtual TCHAR *SetAsString(const TCHAR *Value){(void)Value; return(NULL);};

  virtual void Set(bool Value){ (void)Value; };
  virtual void Set(int Value){ (void)Value;};
  virtual void Set(double Value){ (void)Value; };
  virtual void Set(const TCHAR *Value){ (void)Value; };

  virtual int SetMin(int Value){(void)Value; return(0);};
  virtual double SetMin(double Value){(void)Value; return(false);};

  virtual int SetMax(int Value){(void)Value; return(0);};
  virtual double SetMax(double Value){(void)Value; return(0);};
  void SetUnits(const TCHAR *text) { _tcscpy(mUnits, text); }

  void Use(void){
    mUsageCounter++;
  }

  int Unuse(void){
    mUsageCounter--;
    return(mUsageCounter);
  }

  void SetDisplayFormat(TCHAR *Value);
  void SetDisableSpeedUp(bool bDisable) {mDisableSpeedup=bDisable;}  // allows combolist to iterate all values
  bool GetDisableSpeedUp(void) {return mDisableSpeedup;}
  void SetDetachGUI(bool bDetachGUI) {mDetachGUI=bDetachGUI;}  // allows combolist to iterate all values w/out triggering external events
  bool GetDetachGUI(void) {return mDetachGUI;}
  virtual int CreateComboList(void) {return 0;};
  ComboList* GetCombo(void) { return &mComboList;}
  virtual int SetFromCombo(int iDataFieldIndex, TCHAR *sValue) {return SetAsInteger(iDataFieldIndex);};
  void CopyString(TCHAR * szStringOut, bool bFormatted);
  bool SupportCombo;  // all Types dataField support combolist except DataFieldString.
  protected:
    void (*mOnDataAccess)(DataField *Sender, DataAccessKind_t Mode);
    TCHAR mEditFormat[FORMATSIZE+1];
    TCHAR mDisplayFormat[FORMATSIZE+1];
    TCHAR mUnits[UNITSIZE+1];
    ComboList mComboList;
    int CreateComboListStepping(void);

  private:

    int mUsageCounter;
    bool mDisableSpeedup;
    bool mDetachGUI;

};

#endif
