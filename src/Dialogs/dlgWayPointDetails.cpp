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
#include "Protection.hpp"
#include "Math/Earth.hpp"
#include "Profile.hpp"
#include "LocalPath.hpp"
#include "LocalTime.hpp"
#include "UtilsText.hpp"
#include "Math/SunEphemeris.hpp"
#include "Blackboard.hpp"
#include "SettingsComputer.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Layout.hpp"
#include "Math/FastMath.h"
#include "MainWindow.hpp"
#include "Components.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Task/TaskManager.hpp"
#include "Task/Tasks/TaskSolvers/TaskSolution.hpp"
#include "Task/Tasks/BaseTask/UnorderedTaskPoint.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "WayPoint/WayPointGlue.hpp"
#include "Compiler.h"
#include "Compatibility/path.h"

#include <assert.h>
#include <stdio.h>

static int page = 0;
static WndForm *wf = NULL;
static WndListFrame *wDetails = NULL;
static WndFrame *wInfo = NULL;
static WndFrame *wCommand = NULL;
static WndFrame *wSpecial = NULL; // VENTA3
static WndOwnerDrawFrame *wImage = NULL;
static BOOL hasimage1 = false;
static BOOL hasimage2 = false;
static const Waypoint *selected_waypoint = NULL;

static Bitmap jpgimage1, jpgimage2;

static TCHAR path_modis[MAX_PATH];
static TCHAR path_google[MAX_PATH];
static TCHAR szWaypointFile[MAX_PATH];
static TCHAR Directory[MAX_PATH];

#define MAXLINES 100
static int LineOffsets[MAXLINES];
static unsigned nTextLines = 0;

static void
NextPage(int Step)
{
  assert(selected_waypoint);
  bool page_ok = false;
  page += Step;

  do {
    if (page < 0)
      page = 5;
    if (page > 5)
      page = 0;

    switch (page) {
    case 0:
      page_ok = true;
      break;

    case 1:
      if (selected_waypoint->Details.empty()) 
        page += Step;
      else
        page_ok = true;

      break;

    case 2:
      page_ok = true;
      break;

    case 3: // VENTA3
      page_ok = true;
      break;

    case 4:
      if (!hasimage1)
        page += Step;
      else
        page_ok = true;

      break;

    case 5:
      if (!hasimage2)
        page += Step;
      else
        page_ok = true;

      break;

    default:
      page_ok = true;
      page = 0;
      break;
      // error!
    }
  } while (!page_ok);

  wInfo->set_visible(page == 0);
  wDetails->set_visible(page == 1);
  wCommand->set_visible(page == 2);
  wSpecial->set_visible(page == 3);
  wImage->set_visible(page >= 4);
}

static void
OnPaintDetailsListItem(Canvas &canvas, const RECT rc, unsigned DrawListIndex)
{
  assert(selected_waypoint);

  if (DrawListIndex >= nTextLines)
    return;

  const TCHAR* text = selected_waypoint->Details.c_str();
  int nstart = LineOffsets[DrawListIndex];
  int nlen;
  if (DrawListIndex < nTextLines - 1) {
    nlen = LineOffsets[DrawListIndex + 1] - LineOffsets[DrawListIndex] - 1;
    nlen--;
  } else {
    nlen = _tcslen(text + nstart) - 1;
  }

  if (nlen > 0)
    canvas.text(rc.left + Layout::FastScale(2), rc.top + Layout::FastScale(2),
                text + nstart, nlen);
}

static void
OnNextClicked(gcc_unused WndButton &button)
{
  NextPage(+1);
}

static void
OnPrevClicked(gcc_unused WndButton &button)
{
  NextPage(-1);
}

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

static bool
FormKeyDown(WindowControl *Sender, unsigned key_code)
{
  (void)Sender;
  switch (key_code) {
  case VK_LEFT:
  case '6':
    ((WndButton *)wf->FindByName(_T("cmdPrev")))->set_focus();
    NextPage(-1);
    //((WndButton *)wf->FindByName(_T("cmdPrev")))->SetFocused(true, NULL);
    return true;

  case VK_RIGHT:
  case '7':
    ((WndButton *)wf->FindByName(_T("cmdNext")))->set_focus();
    NextPage(+1);
    //((WndButton *)wf->FindByName(_T("cmdNext")))->SetFocused(true, NULL);
    return true;

  default:
    return false;
  }
}

static void
OnGotoClicked(gcc_unused WndButton &button)
{
  if (selected_waypoint) {
    protected_task_manager.do_goto(*selected_waypoint);
  }

  wf->SetModalResult(mrOK);
}

static void
OnReplaceClicked(gcc_unused WndButton &button)
{
#ifdef OLD_TASK
  task.ReplaceWaypoint(SelectedWaypoint, XCSoarInterface::SettingsComputer(),
      XCSoarInterface::Basic());
  task.RefreshTask(XCSoarInterface::SettingsComputer(),
                   XCSoarInterface::Basic());
#endif
  wf->SetModalResult(mrOK);
}

static void 
OnNewHomeClicked(gcc_unused WndButton &button)
{
  XCSoarInterface::SetSettingsComputer().HomeWaypoint = selected_waypoint->id;

  {
    ScopeSuspendAllThreads suspend;
    WayPointGlue::SetHome(way_points, terrain,
                          XCSoarInterface::SetSettingsComputer(),
                          false, false);
  }

  wf->SetModalResult(mrOK);
}

// VENTA3
static void 
OnSetAlternate1Clicked(gcc_unused WndButton &button)
{
  XCSoarInterface::SetSettingsComputer().Alternate1 = selected_waypoint->id;
  Profile::Set(szProfileAlternate1, XCSoarInterface::SettingsComputer().Alternate1);
  wf->SetModalResult(mrOK);
}

static void 
OnSetAlternate2Clicked(gcc_unused WndButton &button)
{
  XCSoarInterface::SetSettingsComputer().Alternate2 = selected_waypoint->id;
  Profile::Set(szProfileAlternate2, XCSoarInterface::SettingsComputer().Alternate2);
  wf->SetModalResult(mrOK);
}

static void
OnClearAlternatesClicked(gcc_unused WndButton &button)
{
  XCSoarInterface::SetSettingsComputer().Alternate1 = -1;
  XCSoarInterface::SetSettingsComputer().EnableAlternate1 = false;
  XCSoarInterface::SetSettingsComputer().Alternate2 = -1;
  XCSoarInterface::SetSettingsComputer().EnableAlternate2 = false;
  Profile::Set(szProfileAlternate1,
      XCSoarInterface::SettingsComputer().Alternate1);
  Profile::Set(szProfileAlternate2,
      XCSoarInterface::SettingsComputer().Alternate2);

  wf->SetModalResult(mrOK);
}

static void
OnTeamCodeClicked(gcc_unused WndButton &button)
{
  XCSoarInterface::SetSettingsComputer().TeamCodeRefWaypoint =
      selected_waypoint->id;
  Profile::Set(szProfileTeamcodeRefWaypoint,
      XCSoarInterface::SettingsComputer().TeamCodeRefWaypoint);

  wf->SetModalResult(mrOK);
}

static void
OnInsertInTaskClicked(gcc_unused WndButton &button)
{
#ifdef OLD_TASK
  task.InsertWaypoint(SelectedWaypoint, XCSoarInterface::SettingsComputer(),
      XCSoarInterface::Basic());
  task.RefreshTask(XCSoarInterface::SettingsComputer(),
      XCSoarInterface::Basic());
#endif
  wf->SetModalResult(mrOK);
}

static void
OnAppendInTaskClicked(gcc_unused WndButton &button)
{
#ifdef OLD_TASK
  task.InsertWaypoint(SelectedWaypoint, XCSoarInterface::SettingsComputer(),
      XCSoarInterface::Basic(), true);
#endif
  wf->SetModalResult(mrOK);
}

static void
OnRemoveFromTaskClicked(gcc_unused WndButton &button)
{
#ifdef OLD_TASK
  task.RemoveWaypoint(SelectedWaypoint, XCSoarInterface::SettingsComputer(),
      XCSoarInterface::Basic());
#endif
  wf->SetModalResult(mrOK);
}

static void
OnImagePaint(WindowControl *Sender, Canvas &canvas)
{
  (void)Sender;

  if (page == 4) {
    BitmapCanvas bitmap_canvas(canvas, jpgimage1);
    canvas.copy(bitmap_canvas);
  } else if (page == 5) {
    BitmapCanvas bitmap_canvas(canvas, jpgimage2);
    canvas.copy(bitmap_canvas);
  }
}

static CallBackTableEntry_t CallBackTable[] = {
    DeclareCallBackEntry(OnNextClicked),
    DeclareCallBackEntry(OnPrevClicked),
    DeclareCallBackEntry(NULL)
};

void 
dlgWayPointDetailsShowModal(SingleWindow &parent, const Waypoint& way_point)
{
  selected_waypoint = &way_point;

  TCHAR sTmp[128];
  double sunsettime;
  int sunsethours;
  int sunsetmins;
  WndProperty *wp;

  if (Layout::landscape)
    wf = dlgLoadFromXML(CallBackTable,
                        parent, _T("IDR_XML_WAYPOINTDETAILS_L"));
  else
    wf = dlgLoadFromXML(CallBackTable,
                        parent, _T("IDR_XML_WAYPOINTDETAILS"));

  nTextLines = 0;

  if (!wf)
    return;

  Profile::Get(szProfileWayPointFile, szWaypointFile, MAX_PATH);
  ExpandLocalPath(szWaypointFile);
  ExtractDirectory(Directory, szWaypointFile);

  _stprintf(path_modis, _T("%s" DIR_SEPARATOR_S "modis-%03d.jpg"),
           Directory,
           selected_waypoint->id+1);
  _stprintf(path_google,_T("%s" DIR_SEPARATOR_S "google-%03d.jpg"),
           Directory,
           selected_waypoint->id+1);

  _stprintf(sTmp, _T("%s: '%s'"), wf->GetCaption(), selected_waypoint->Name.c_str());
  wf->SetCaption(sTmp);

  wp = ((WndProperty *)wf->FindByName(_T("prpWpComment")));
  wp->SetText(selected_waypoint->Comment.c_str());
  wp->SetButtonSize(16);

  Units::LongitudeToString(selected_waypoint->Location.Longitude, sTmp, sizeof(sTmp)-1);
  ((WndProperty *)wf->FindByName(_T("prpLongitude")))
    ->SetText(sTmp);

  Units::LatitudeToString(selected_waypoint->Location.Latitude, sTmp, sizeof(sTmp)-1);
  ((WndProperty *)wf->FindByName(_T("prpLatitude")))
    ->SetText(sTmp);

  Units::FormatUserAltitude(selected_waypoint->Altitude, sTmp, sizeof(sTmp)-1);
  ((WndProperty *)wf->FindByName(_T("prpAltitude")))
    ->SetText(sTmp);

  SunEphemeris sun;
  sunsettime = sun.CalcSunTimes
    (selected_waypoint->Location,
     XCSoarInterface::Basic().DateTime,
     GetUTCOffset()/3600);

  sunsethours = (int)sunsettime;
  sunsetmins = (int)((sunsettime - sunsethours) * 60);

  _stprintf(sTmp, _T("%02d:%02d"), sunsethours, sunsetmins);
  ((WndProperty *)wf->FindByName(_T("prpSunset")))->SetText(sTmp);

  fixed distance;
  Angle bearing;
  DistanceBearing(XCSoarInterface::Basic().Location,
                  selected_waypoint->Location,
                  &distance,
                  &bearing);

  TCHAR DistanceText[MAX_PATH];
  Units::FormatUserDistance(distance, DistanceText, 10);
  ((WndProperty *)wf->FindByName(_T("prpDistance"))) ->SetText(DistanceText);

  _stprintf(sTmp, _T("%d")_T(DEG), iround(bearing.value_degrees()));
  ((WndProperty *)wf->FindByName(_T("prpBearing"))) ->SetText(sTmp);

  GlidePolar glide_polar = protected_task_manager.get_glide_polar();
  GlidePolar safety_polar = protected_task_manager.get_safety_polar();

  UnorderedTaskPoint t(way_point, XCSoarInterface::SettingsComputer());
  GlideResult r;

  // alt reqd at current mc

  const AIRCRAFT_STATE aircraft_state =
    ToAircraftState(XCSoarInterface::Basic());
  r = TaskSolution::glide_solution_remaining(t, aircraft_state, glide_polar);
  wp = (WndProperty *)wf->FindByName(_T("prpMc2"));
  if (wp) {
    _stprintf(sTmp, _T("%.0f %s"),
              (double)Units::ToUserAltitude(r.AltitudeDifference),
              Units::GetAltitudeName());
    wp->SetText(sTmp);
  }

  // alt reqd at mc 0

  glide_polar.set_mc(fixed_zero);
  r = TaskSolution::glide_solution_remaining(t, aircraft_state, glide_polar);
  wp = (WndProperty *)wf->FindByName(_T("prpMc0"));
  if (wp) {
    _stprintf(sTmp, _T("%.0f %s"),
              (double)Units::ToUserAltitude(r.AltitudeDifference),
              Units::GetAltitudeName());
    wp->SetText(sTmp);
  }

  // alt reqd at safety mc

  r = TaskSolution::glide_solution_remaining(t, aircraft_state, safety_polar);
  wp = (WndProperty *)wf->FindByName(_T("prpMc1"));
  if (wp) {
    _stprintf(sTmp, _T("%.0f %s"),
              (double)Units::ToUserAltitude(r.AltitudeDifference),
              Units::GetAltitudeName());
    wp->SetText(sTmp);
  }

  wf->SetKeyDownNotify(FormKeyDown);

  ((WndButton *)wf->FindByName(_T("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wInfo = ((WndFrame *)wf->FindByName(_T("frmInfos")));
  wCommand = ((WndFrame *)wf->FindByName(_T("frmCommands")));
  wSpecial = ((WndFrame *)wf->FindByName(_T("frmSpecial")));
  wImage = ((WndOwnerDrawFrame *)wf->FindByName(_T("frmImage")));
  wDetails = (WndListFrame*)wf->FindByName(_T("frmDetails"));
  wDetails->SetPaintItemCallback(OnPaintDetailsListItem);

  assert(wInfo != NULL);
  assert(wCommand != NULL);
  assert(wSpecial != NULL);
  assert(wImage != NULL);
  assert(wDetails != NULL);

  nTextLines = TextToLineOffsets(way_point.Details.c_str(), LineOffsets, MAXLINES);
  wDetails->SetLength(nTextLines);

  /*
  TODO enhancement: wpdetails
  wp = ((WndProperty *)wf->FindByName(_T("prpWpDetails")));
  wp->SetText(way_point.Details);
  */

  wCommand->hide();
  wSpecial->hide();
  wImage->SetCaption(_("Blank!"));
  wImage->SetOnPaintNotify(OnImagePaint);

  WndButton *wb;

  wb = ((WndButton *)wf->FindByName(_T("cmdGoto")));
  if (wb)
    wb->SetOnClickNotify(OnGotoClicked);

  wb = ((WndButton *)wf->FindByName(_T("cmdReplace")));
  if (wb)
    wb->SetOnClickNotify(OnReplaceClicked);

  wb = ((WndButton *)wf->FindByName(_T("cmdNewHome")));
  if (wb)
    wb->SetOnClickNotify(OnNewHomeClicked);

  wb = ((WndButton *)wf->FindByName(_T("cmdSetAlternate1")));
  if (wb)
    wb->SetOnClickNotify(OnSetAlternate1Clicked);

  wb = ((WndButton *)wf->FindByName(_T("cmdSetAlternate2")));
  if (wb)
    wb->SetOnClickNotify(OnSetAlternate2Clicked);

  wb = ((WndButton *)wf->FindByName(_T("cmdClearAlternates")));
  if (wb)
    wb->SetOnClickNotify(OnClearAlternatesClicked);

  wb = ((WndButton *)wf->FindByName(_T("cmdTeamCode")));
  if (wb)
    wb->SetOnClickNotify(OnTeamCodeClicked);

  wb = ((WndButton *)wf->FindByName(_T("cmdInserInTask")));
  if (wb)
    wb->SetOnClickNotify(OnInsertInTaskClicked);

  wb = ((WndButton *)wf->FindByName(_T("cmdAppendInTask")));
  if (wb)
    wb->SetOnClickNotify(OnAppendInTaskClicked);

  wb = ((WndButton *)wf->FindByName(_T("cmdRemoveFromTask")));
  if (wb)
    wb->SetOnClickNotify(OnRemoveFromTaskClicked);

  hasimage1 = jpgimage1.load_file(path_modis);
  hasimage2 = jpgimage2.load_file(path_google);

  page = 0;

  NextPage(0); // JMW just to turn proper pages on/off

  wf->ShowModal();

  delete wf;

  wf = NULL;
}
