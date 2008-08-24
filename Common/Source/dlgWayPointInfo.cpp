
#include "StdAfx.h"
#include "XCSoar.h"

#include "WindowControls.h"
#include "Statistics.h"
#include "externs.h"
#include "McReady.h"
#include "dlgTools.h"

static int page=0;


void NextPage(int Step){
  page += Step;
  if (page > 3)
    page = 0;
  if (page < 0)
    page = 3;
  Update();
}

void OnNextClicked(WindowControl * Sender){
  NextPage(+1);
}

void OnPrevClicked(WindowControl * Sender){
  NextPage(-1);
}

static void OnCloseClicked(WindowControl * Sender){
  wf->SetModalResult(mrOK);
}

static int FormKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam){

  if (wGrid->GetFocused())
    return(0);

  switch(wParam & 0xffff){
    case VK_LEFT:
      NextPage(-1);
    return(0);
    case VK_RIGHT:
      NextPage(+1);
    return(0);
  }
  return(1);
}

static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnAnalysisPaint),
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(NULL)
};

void dlgAnalysisShowModal(void){

  wf = dlgLoadFromXML(CallBackTable, "T:\\Project\\WINCE\\TNAV\\XCSoar\\dlgWayPointInfo.xml", hWndMainWindow);

  wf->SetKeyDownNotify(FormKeyDown);

  ((WndButton *)wf->FindByName(TEXT("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  Update();

  wf->ShowModal();

  delete wf;

  wf = NULL;

}
