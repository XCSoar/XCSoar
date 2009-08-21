#include "DataField/ComboList.hpp"

#include <assert.h>
#include <stdlib.h>

ComboListEntry_t * ComboList::CreateItem(int ItemIndex,
                                        int DataFieldIndex,
                                        const TCHAR *StringValue,
                                        const TCHAR *StringValueFormatted)
{
  int iLen = -1;
  ComboListEntry_t * theItem;

  // Copy current strings into structure
  theItem = (ComboListEntry_t*) malloc(sizeof(ComboListEntry_t));
  theItem->DataFieldIndex=0;  // NULL is same as 0, so it fails to set it if index value is 0
  theItem->ItemIndex=0;

  assert(theItem!= NULL);

  theItem->ItemIndex=ItemIndex;

  if (DataFieldIndex != ComboPopupNULL) { // optional
    theItem->DataFieldIndex=DataFieldIndex;
  }

  if (StringValue == NULL)
  {
    theItem->StringValue = (TCHAR*)malloc((1) * sizeof(TCHAR));
    assert(theItem->StringValue != NULL);
    theItem->StringValue[0]='\0';
  }
  else
  {
    iLen = _tcslen(StringValue);
    theItem->StringValue = (TCHAR*)malloc((iLen + 1) * sizeof(TCHAR));
    assert(theItem->StringValue != NULL);
    _tcscpy(theItem->StringValue, StringValue);
  }


  // copy formatted display string
  if (StringValueFormatted == NULL)
  {
    theItem->StringValueFormatted = (TCHAR*)malloc((1) * sizeof(TCHAR));
    assert(theItem->StringValueFormatted != NULL);
    theItem->StringValueFormatted[0]='\0';
  }
  else
  {
    iLen = _tcslen(StringValueFormatted);
    theItem->StringValueFormatted = (TCHAR*)malloc((iLen + 1) * sizeof(TCHAR));
    assert(theItem->StringValueFormatted != NULL);
    _tcscpy(theItem->StringValueFormatted, StringValueFormatted);
  }

  return theItem;
}

void ComboList::FreeComboPopupItemList(void)
{
  for (int i = 0; i < ComboPopupItemCount && i < ComboPopupLISTMAX; i++)
  {
    if (ComboPopupItemList[i] != NULL)
    {
      free (ComboPopupItemList[i]->StringValue);
      ComboPopupItemList[i]->StringValue=NULL;

      free (ComboPopupItemList[i]->StringValueFormatted);
      ComboPopupItemList[i]->StringValueFormatted=NULL;

      free (ComboPopupItemList[i]);
      ComboPopupItemList[i]=NULL;

    }
  }
}
