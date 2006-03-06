#if (NEWINFOBOX>0)

#include "stdafx.h"
#include "XCSoar.h"

#include "WindowControls.h"
#include "Externs.h"
#include "dlgTools.h"


static int page=0;
static WndForm *wf=NULL;
static WndOwnerDrawFrame *wSplash=NULL;
static HBITMAP hSplash;
extern HINSTANCE hInst;

static void OnSplashPaint(WindowControl * Sender, HDC hDC){

  RECT  rc;

  hSplash=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_DISCLAIMER));

  CopyRect(&rc, Sender->GetBoundRect());
  HDC hDCTemp = CreateCompatibleDC(hDC);

  SelectObject(hDCTemp, hSplash);
  StretchBlt(hDC,
	     rc.left, rc.top,
	     rc.right, rc.bottom,
	     hDCTemp, 0, 0, 318, 163, SRCCOPY);

  DeleteObject(hSplash);
  DeleteDC(hDCTemp);

}

static void OnCloseClicked(WindowControl * Sender){
  wf->SetModalResult(mrOK);
}

static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(OnSplashPaint),
  DeclearCallBackEntry(NULL)
};

extern TCHAR startProfileFile[];

void dlgStartupShowModal(void){
  WndProperty* wp;

  wf = dlgLoadFromXML(CallBackTable, "\\NOR Flash\\dlgStartup.xml",
		      hWndMainWindow);
  if (!wf) return;

  wSplash = (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmSplash"));

  ((WndButton *)wf->FindByName(TEXT("cmdClose")))
    ->SetOnClickNotify(OnCloseClicked);

  TCHAR temp[MAX_PATH];

  _stprintf(temp,TEXT("XCSoar: Version %s"), XCSoar_Version);
  wf->SetCaption(temp);

  wp = ((WndProperty *)wf->FindByName(TEXT("prpDisclaimer")));
  if (wp)
    wp->SetText(TEXT("Pilot assumes complete\r\nresponsibility to operate\r\nthe aircraft safely.\r\nMaintain effective lookout.\r\n"));

  wp = ((WndProperty *)wf->FindByName(TEXT("prpProfile")));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.prf"));
    dfe->Lookup(startProfileFile);
    wp->RefreshDisplay();
    if (dfe->GetNumFiles()<=2) {
      delete wf;
      wf = NULL;
    }
  }

  wf->ShowModal();

  wp = (WndProperty*)wf->FindByName(TEXT("prpProfile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    if (_tcslen(dfe->GetPathFile())>0) {
      _tcscpy(startProfileFile,dfe->GetPathFile());
    }
  }

  delete wf;

  wf = NULL;

}

#endif

