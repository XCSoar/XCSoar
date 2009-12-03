/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Dialogs/Internal.hpp"
#include "InfoBoxLayout.h"
#include "SettingsComputer.hpp"
#include "SettingsTask.hpp"
#include "McReady.h"
#include "Math/FastMath.h"
#include "Math/Geometry.hpp"
#include "Math/Earth.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Graphics.hpp"
#include "MainWindow.hpp"
#include "GlideComputer.hpp"
#include "Atmosphere.h"
#include "Blackboard.hpp"
#include "Components.hpp"
#include "Calculations.h"

#define MAXPAGE 8

static int page=0;
static WndForm *wf=NULL;
static WndOwnerDrawFrame *wGrid=NULL;
static WndOwnerDrawFrame *wInfo=NULL;
static WndButton *wCalc=NULL;

static void SetCalcCaption(const TCHAR* caption) {
  if (wCalc) {
    wCalc->SetCaption(gettext(caption));
  }
}


#define ANALYSIS_PAGE_BAROGRAPH    0
#define ANALYSIS_PAGE_CLIMB        1
#define ANALYSIS_PAGE_TASK_SPEED   2
#define ANALYSIS_PAGE_WIND         3
#define ANALYSIS_PAGE_POLAR        4
#define ANALYSIS_PAGE_TEMPTRACE    5
#define ANALYSIS_PAGE_TASK         6
#define ANALYSIS_PAGE_OLC          7
#define ANALYSIS_PAGE_AIRSPACE     8


static void OnAnalysisPaint(WindowControl *Sender, Canvas &canvas)
{
  RECT rcgfx = Sender->get_client_rect();

  // background is painted in the base-class

  canvas.select(*Sender->GetFont());

  canvas.background_transparent();
  canvas.set_text_color(Sender->GetForeColor());

  const FlightStatistics &fs = glide_computer.GetFlightStats();
  OLCOptimizer &olc = glide_computer.GetOLC();

  switch (page) {
  case ANALYSIS_PAGE_BAROGRAPH:
    SetCalcCaption(TEXT("Settings"));
    fs.RenderBarograph(canvas, rcgfx,
                       XCSoarInterface::Calculated());
    break;
  case ANALYSIS_PAGE_CLIMB:
    SetCalcCaption(TEXT("Task calc"));
    fs.RenderClimb(canvas, rcgfx);
    break;
  case ANALYSIS_PAGE_WIND:
    SetCalcCaption(TEXT("Set wind"));
    fs.RenderWind(canvas, rcgfx,
                  XCSoarInterface::Basic(),
                  glide_computer.windanalyser.windstore);
    break;
  case ANALYSIS_PAGE_POLAR:
    SetCalcCaption(TEXT("Settings"));
    fs.RenderGlidePolar(canvas, rcgfx,
                        XCSoarInterface::Calculated(),
                        XCSoarInterface::SettingsComputer());
    break;
  case ANALYSIS_PAGE_TEMPTRACE:
    SetCalcCaption(TEXT("Settings"));
    fs.RenderTemperature(canvas, rcgfx);
    break;
  case ANALYSIS_PAGE_TASK:
    SetCalcCaption(TEXT("Task calc"));
    olc.Lock();
    fs.RenderTask(canvas, rcgfx,
                  XCSoarInterface::Basic(),
                  XCSoarInterface::SettingsComputer(),
                  XCSoarInterface::SettingsMap(),
                  olc, false);
    olc.Unlock();
    break;
  case ANALYSIS_PAGE_OLC:
    SetCalcCaption(TEXT("Optimise"));
    olc.Lock();
    fs.RenderTask(canvas, rcgfx,
                  XCSoarInterface::Basic(),
                  XCSoarInterface::SettingsComputer(),
                  XCSoarInterface::SettingsMap(),
                  olc, true);
    olc.Unlock();
    break;
  case ANALYSIS_PAGE_AIRSPACE:
    SetCalcCaption(TEXT("Warnings"));
    fs.RenderAirspace(canvas, rcgfx,
                      XCSoarInterface::Basic(),
                      XCSoarInterface::Calculated(),
                      XCSoarInterface::SettingsMap(),
                      airspace_database, terrain);
    break;
  case ANALYSIS_PAGE_TASK_SPEED:
    SetCalcCaption(TEXT("Task calc"));
    fs.RenderSpeed(canvas, rcgfx,
                   XCSoarInterface::Calculated());
    break;
  default:
    // should never get here!
    break;
  }
}



static void Update(void){
  TCHAR sTmp[1000];
  //  WndProperty *wp;
  double d=0;

  FlightStatistics &fs = glide_computer.GetFlightStats();

  switch(page){
    case ANALYSIS_PAGE_BAROGRAPH:
      _stprintf(sTmp, TEXT("%s: %s"),
                gettext(TEXT("Analysis")),
                gettext(TEXT("Barograph")));
      wf->SetCaption(sTmp);
      fs.CaptionBarograph(sTmp);
      wInfo->SetCaption(sTmp);

    break;
    case ANALYSIS_PAGE_CLIMB:
      _stprintf(sTmp, TEXT("%s: %s"),
                gettext(TEXT("Analysis")),
                gettext(TEXT("Climb")));
      wf->SetCaption(sTmp);
      fs.CaptionClimb(sTmp);
      wInfo->SetCaption(sTmp);

    break;
    case ANALYSIS_PAGE_WIND:
      _stprintf(sTmp, TEXT("%s: %s"),
                gettext(TEXT("Analysis")),
                gettext(TEXT("Wind at Altitude")));
      wf->SetCaption(sTmp);
      _stprintf(sTmp, TEXT(" "));
      wInfo->SetCaption(sTmp);
    break;
    case ANALYSIS_PAGE_POLAR:
      _stprintf(sTmp, TEXT("%s: %s (Mass %3.0f kg)"),
                gettext(TEXT("Analysis")),
                gettext(TEXT("Glide Polar")),
                GlidePolar::GetAUW());
      wf->SetCaption(sTmp);
      fs.CaptionPolar(sTmp);
      wInfo->SetCaption(sTmp);
    break;
  case ANALYSIS_PAGE_TEMPTRACE:
    _stprintf(sTmp, TEXT("%s: %s"),
              gettext(TEXT("Analysis")),
              gettext(TEXT("Temp trace")));
    wf->SetCaption(sTmp);
    fs.CaptionTempTrace(sTmp);
    wInfo->SetCaption(sTmp);
    break;
  case ANALYSIS_PAGE_TASK_SPEED:
    _stprintf(sTmp, TEXT("%s: %s"),
              gettext(TEXT("Analysis")),
              gettext(TEXT("Task speed")));
    wf->SetCaption(sTmp);
    wInfo->SetCaption(TEXT(""));
    break;
  case ANALYSIS_PAGE_TASK:
    _stprintf(sTmp, TEXT("%s: %s"),
              gettext(TEXT("Analysis")),
              gettext(TEXT("Task")));
    wf->SetCaption(sTmp);
    RefreshTaskStatistics();
    fs.CaptionTask(sTmp, XCSoarInterface::Calculated());
    wInfo->SetCaption(sTmp);
    break;
  case ANALYSIS_PAGE_OLC:
    _stprintf(sTmp, TEXT("%s: %s"),
              gettext(TEXT("Analysis")),
              gettext(TEXT("OnLine Contest")));
    wf->SetCaption(sTmp);

    TCHAR sFinished[20];
    double dt, score;
    bool olcvalid;
    bool olcfinished;

    dt = glide_computer.GetOLC().getDt(XCSoarInterface::SettingsComputer());
    d = glide_computer.GetOLC().getD(XCSoarInterface::SettingsComputer());
    olcvalid = glide_computer.GetOLC().getValid(XCSoarInterface::SettingsComputer());
    score = glide_computer.GetOLC().getScore(XCSoarInterface::SettingsComputer());
    olcfinished = glide_computer.GetOLC().getFinished(XCSoarInterface::SettingsComputer());

    if (olcfinished) {
      _tcscpy(sFinished,TEXT("Finished"));
    } else {
      _tcscpy(sFinished,TEXT("..."));
    }

    if (olcvalid) {
      TCHAR timetext1[100];
      Units::TimeToText(timetext1, (int)dt);
      if (InfoBoxLayout::landscape) {
        _stprintf(sTmp,
                  TEXT("%s\r\n%s:\r\n  %5.0f %s\r\n%s: %s\r\n%s: %3.0f %s\r\n%s: %.2f\r\n"),
                  sFinished,
                  gettext(TEXT("Distance")),
                  DISTANCEMODIFY*d,
                  Units::GetDistanceName(),
                  gettext(TEXT("Time")),
                  timetext1,
                  gettext(TEXT("Speed")),
                  TASKSPEEDMODIFY*d/dt,
                  Units::GetTaskSpeedName(),
                  gettext(TEXT("Score")),
                  score);
      } else {
        _stprintf(sTmp,
                  TEXT("%s\r\n%s: %5.0f %s\r\n%s: %s\r\n%s: %3.0f %s\r\n%s: %.2f\r\n"),
                  sFinished,
                  gettext(TEXT("Distance")),
                  DISTANCEMODIFY*d,
                  Units::GetDistanceName(),
                  gettext(TEXT("Time")),
                  timetext1,
                  gettext(TEXT("Speed")),
                  TASKSPEEDMODIFY*d/dt,
                  Units::GetTaskSpeedName(),
                  gettext(TEXT("Score")),
                  score);
      }
    } else {
      _stprintf(sTmp, TEXT("%s\r\n"),
                gettext(TEXT("No valid path")));
    }
    wInfo->SetCaption(sTmp);

    break;
  case ANALYSIS_PAGE_AIRSPACE:
    _stprintf(sTmp, TEXT("%s: %s"),
              gettext(TEXT("Analysis")),
              gettext(TEXT("Airspace")));
    wf->SetCaption(sTmp);
    wInfo->SetCaption(TEXT(" "));
    break;
  }

  wGrid->SetVisible(page<MAXPAGE+1);

  if (wGrid != NULL)
    wGrid->Redraw();

}

static void NextPage(int Step){
  page += Step;
  if (page > MAXPAGE)
    page = 0;
  if (page < 0)
    page = MAXPAGE;
  Update();
}


static void OnNextClicked(WindowControl * Sender){
	(void)Sender;
  NextPage(+1);
}

static void OnPrevClicked(WindowControl * Sender){
	(void)Sender;
  NextPage(-1);
}

static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}


static int FormKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam){
  (void)Sender; (void)lParam;

  if (wGrid->GetFocused())
    return(0);

  switch(wParam & 0xffff){
    case VK_LEFT:
    case '6':
      ((WndButton *)wf->FindByName(TEXT("cmdPrev")))->set_focus();
      NextPage(-1);
      //((WndButton *)wf->FindByName(TEXT("cmdPrev")))->SetFocused(true, NULL);
    return(0);
    case VK_RIGHT:
    case '7':
      ((WndButton *)wf->FindByName(TEXT("cmdNext")))->set_focus();
      NextPage(+1);
      //((WndButton *)wf->FindByName(TEXT("cmdNext")))->SetFocused(true, NULL);
    return(0);
  }
  return(1);
}

static void OnCalcClicked(WindowControl * Sender,
			  WndListFrame::ListInfo_t *ListInfo){
  (void)ListInfo;
  (void)Sender;
  if (page==ANALYSIS_PAGE_BAROGRAPH) {
    dlgBasicSettingsShowModal();
  }
  if (page==ANALYSIS_PAGE_CLIMB) {
    wf->SetVisible(false);
    dlgTaskCalculatorShowModal();
    wf->SetVisible(true);
  }
  if (page==ANALYSIS_PAGE_WIND) {
    dlgWindSettingsShowModal();
  }
  if (page==ANALYSIS_PAGE_POLAR) {
    dlgBasicSettingsShowModal();
  }
  if (page==ANALYSIS_PAGE_TEMPTRACE) {
    dlgBasicSettingsShowModal();
  }
  if ((page==ANALYSIS_PAGE_TASK) || (page==ANALYSIS_PAGE_TASK_SPEED)) {
    wf->SetVisible(false);
    dlgTaskCalculatorShowModal();
    wf->SetVisible(true);
  }
  if (page==ANALYSIS_PAGE_OLC) {
    XCSoarInterface::StartHourglassCursor();
    glide_computer.GetOLC().Optimize(XCSoarInterface::SettingsComputer(),(XCSoarInterface::Calculated().Flying==1));
    XCSoarInterface::StopHourglassCursor();
  }
  if (page==ANALYSIS_PAGE_AIRSPACE) {
    dlgAirspaceWarningShowDlg(true);
  }
  Update();
}



static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnAnalysisPaint),
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(OnCalcClicked),
  DeclareCallBackEntry(NULL)
};


void dlgAnalysisShowModal(void){

  wf=NULL;
  wGrid=NULL;
  wInfo=NULL;
  wCalc=NULL;

  if (!InfoBoxLayout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgAnalysis_L.xml"),
                        XCSoarInterface::main_window,
                        TEXT("IDR_XML_ANALYSIS_L"));
  } else  {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgAnalysis.xml"),
                        XCSoarInterface::main_window,
                        TEXT("IDR_XML_ANALYSIS"));
  }

  if (!wf) return;

  wf->SetKeyDownNotify(FormKeyDown);

  wGrid = (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmGrid"));
  wInfo = (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmInfo"));
  ((WndButton *)wf->FindByName(TEXT("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wCalc = ((WndButton *)wf->FindByName(TEXT("cmdCalc")));

  Update();

  wf->ShowModal();

  delete wf;

  wf = NULL;
}
