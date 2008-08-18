#include "StdAfx.h"
#include "XCSoar.h"
#include "Utils.h"
#include "WindowControls.h"
#include "dlgTools.h"
#include "externs.h"
#include "InfoBoxLayout.h"

static WndForm *wf=NULL;
static WndOwnerDrawFrame *wGrid=NULL;

#define MAX_TEXTENTRY 40
static unsigned int cursor = 0;
static unsigned int max_width = MAX_TEXTENTRY;
static TCHAR edittext[MAX_TEXTENTRY];
#define MAXENTRYLETTERS (sizeof(EntryLetters)/sizeof(EntryLetters[0])-1)

static void UpdateTextboxProp(void)
{
  WndProperty *wp;
  wp = (WndProperty*)wf->FindByName(TEXT("prpText"));
  if (wp) {
    wp->SetText(edittext);
  }
}


static int FormKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam) {
  switch(wParam & 0xffff){
    case VK_LEFT:
      if (cursor<1)
        return(0); // min width
      cursor--;
      edittext[cursor] = 0;
      UpdateTextboxProp();
      return(0);
    case VK_RETURN:
      wf->SetModalResult(mrOK);
      return(0);
  }
  return(1);
}

static void OnKey(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo)
{
  TCHAR *Caption = Sender->GetCaption();
  if (cursor < max_width-1)
    {
      edittext[cursor++] = Caption[0];
    }
  UpdateTextboxProp();
}

static void OnOk(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo)
{
  wf->SetModalResult(mrOK);
}

static void ClearText(void)
{
  cursor = 0;
  memset(edittext, 0, sizeof(TCHAR)*MAX_TEXTENTRY);
  UpdateTextboxProp();
}


static void OnClear(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo)
{
  ClearText();
}

static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(OnKey),
  DeclearCallBackEntry(OnClear),
  DeclearCallBackEntry(OnOk),
  DeclearCallBackEntry(NULL)
};

void dlgTextEntryKeyboardShowModal(TCHAR *text, int width)
{
  wf = NULL;
  wGrid = NULL;
  if (width==0) {
    width = MAX_TEXTENTRY;
  }
  max_width = min(MAX_TEXTENTRY, width);
  char filename[MAX_PATH];
  if (InfoBoxLayout::landscape)
  {
    LocalPathS(filename, TEXT("frmTextEntry_Keyboard_L.xml"));
    wf = dlgLoadFromXML(CallBackTable,
			filename,
			hWndMainWindow,
			TEXT("IDR_XML_TEXTENTRY_KEYBOARD_L"));
    if (!wf) return;
  } else {
    LocalPathS(filename, TEXT("frmTextEntry_Keyboard.xml"));
    wf = dlgLoadFromXML(CallBackTable,
			filename,
			hWndMainWindow,
			TEXT("IDR_XML_TEXTENTRY_KEYBOARD"));
    if (!wf) return;
  }

  wGrid = (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmGrid"));

  cursor = 0;
  ClearText();

  /*edittext[0]= 0;
    edittext[1]= 0;*/

  if (_tcslen(text)>0) {
    _tcsupr(text);
    _tcsncpy(edittext, text, max_width-1);
    edittext[max_width-1]= 0;
  }

  UpdateTextboxProp();
  wf->SetKeyDownNotify(FormKeyDown);
  wf->ShowModal();
  _tcsncpy(text, edittext, max_width);
  text[max_width-1]=0;
  delete wf;
}


