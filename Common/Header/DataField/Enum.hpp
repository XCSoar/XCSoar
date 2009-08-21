#ifndef XCSOAR_DATA_FIELD_BOOLEAN_HPP
#define XCSOAR_DATA_FIELD_BOOLEAN_HPP

#include "DataField/Base.hpp"

#define DFE_MAX_ENUMS 100

typedef struct {
  TCHAR *mText;
  unsigned int index;
} DataFieldEnumEntry;

class DataFieldEnum: public DataField {

  private:
    unsigned int nEnums;
    unsigned int mValue;
    DataFieldEnumEntry mEntries[DFE_MAX_ENUMS];

  public:
    DataFieldEnum(const TCHAR *EditFormat,
		  const TCHAR *DisplayFormat,
		  int Default,
		  void(*OnDataAccess)(DataField *Sender,
				      DataAccessKind_t Mode)):
      DataField(EditFormat, DisplayFormat, OnDataAccess){
      SupportCombo=true;

      if (Default>=0)
	{ mValue = Default; }
      else
	{mValue = 0;}
      nEnums = 0;
      if (mOnDataAccess) {
	(mOnDataAccess)(this, daGet);
      }
    };
      ~DataFieldEnum();

  void Inc(void);
  void Dec(void);
  int CreateComboList(void);

  void addEnumText(const TCHAR *Text);

  int GetAsInteger(void);
  TCHAR *GetAsString(void);
  TCHAR *GetAsDisplayString(void){
    return(GetAsString());
  };

  #if defined(__BORLANDC__)
  #pragma warn -hid
  #endif
  void Set(int Value);
  #if defined(__BORLANDC__)
  #pragma warn +hid
  #endif
  int SetAsInteger(int Value);
  void Sort(int startindex=0);
};

#endif
