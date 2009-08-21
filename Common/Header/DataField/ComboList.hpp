#ifndef XCSOAR_DATA_FIELD_COMBO_LIST_HPP
#define XCSOAR_DATA_FIELD_COMBO_LIST_HPP

#include <tchar.h>

    typedef struct {
      int ItemIndex;
      int DataFieldIndex;
      TCHAR *StringValue;
      TCHAR *StringValueFormatted;
    } ComboListEntry_t;

class ComboList{

  public:

    ComboList(void) {
      ComboPopupDrawListIndex=0;
      ComboPopupItemIndex=-1;
      ComboPopupItemSavedIndex=-1;
    }

#define ComboPopupLISTMAX 300
#define ComboPopupITEMMAX 100
#define ComboPopupReopenMOREDataIndex -800001
#define ComboPopupReopenLESSDataIndex -800002
#define ComboPopupNULL -800003

    ComboListEntry_t *CreateItem(int ItemIndex, int DataFieldIndex,
                                 const TCHAR *StringValue,
                                 const TCHAR *StringValueFormatted);
    void FreeComboPopupItemList(void);

    int ComboPopupDrawListIndex;
    int ComboPopupItemIndex;
    int ComboPopupItemSavedIndex;
    int ComboPopupItemCount;
    ComboListEntry_t * ComboPopupItemList[ComboPopupLISTMAX]; // RLD make this dynamic later

    int PropertyDataFieldIndexSaved;
    TCHAR PropertyValueSaved[ComboPopupITEMMAX];
    TCHAR PropertyValueSavedFormatted[ComboPopupITEMMAX];


  private:

};

#endif
