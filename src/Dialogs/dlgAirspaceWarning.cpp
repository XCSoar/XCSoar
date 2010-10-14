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
#include "Units.hpp"
#include "Components.hpp"
#include "Screen/Layout.hpp"
#include "Compatibility/vk.h"
#include "Airspace/AirspaceWarning.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Airspace/AirspaceWarningManager.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

static WndForm *wf=NULL;
static WndListFrame *wAirspaceList=NULL;
static Brush hBrushInsideBk;
static Brush hBrushNearBk;
static Brush hBrushInsideAckBk;
static Brush hBrushNearAckBk;

static const AbstractAirspace* CursorAirspace = NULL; // Current list cursor airspace
static const AbstractAirspace* FocusAirspace = NULL;  // Current action airspace

static void
AirspaceWarningCursorCallback(unsigned i)
{
  ProtectedAirspaceWarningManager::Lease lease(airspace_warnings);
  const AirspaceWarning *warning = lease->get_warning(i);
  CursorAirspace = (warning != NULL)
    ? &warning->get_airspace()
    : NULL;
}

static void
OnAirspaceListEnter(unsigned i)
{
  FocusAirspace = CursorAirspace;
}

static const AbstractAirspace *
GetSelectedAirspace()
{
  return has_pointer() || FocusAirspace == NULL
    ? CursorAirspace
    : FocusAirspace;
}

static bool
HasWarning()
{
  ProtectedAirspaceWarningManager::Lease lease(airspace_warnings);
  for (unsigned i = 0; i < lease->size(); ++i) {
    const AirspaceWarning *warning = lease->get_warning(i);
    if (warning != NULL && warning->get_ack_expired())
      return true;
  }

  return false;
}

static void
AutoHide()
{
  if (!HasWarning()) {
    wf->hide();
    wf->SetModalResult(mrOK);
  }
}

/** ack inside */
static void OnAckClicked(WindowControl * Sender){
  (void)Sender;

  const AbstractAirspace *airspace = GetSelectedAirspace();
  if (airspace != NULL) {
    airspace_warnings.acknowledge_inside(*airspace, true);
    wAirspaceList->invalidate();
    AutoHide();
  }
}

/** ack warn */
static void OnAck1Clicked(WindowControl * Sender){
  (void)Sender;

  const AbstractAirspace *airspace = GetSelectedAirspace();
  if (airspace != NULL) {
    airspace_warnings.acknowledge_warning(*airspace, true);
    wAirspaceList->invalidate();
    AutoHide();
  }
}

/** ack day */
static void OnAck2Clicked(WindowControl * Sender){
  (void)Sender;

  const AbstractAirspace *airspace = GetSelectedAirspace();
  if (airspace != NULL) {
    airspace_warnings.acknowledge_day(*airspace, true);
    wAirspaceList->invalidate();
    AutoHide();
  }
}

/** unack */
static void OnEnableClicked(WindowControl * Sender) {
  (void)Sender;

  const AbstractAirspace *airspace = GetSelectedAirspace();
  if (airspace != NULL) {
    airspace_warnings.acknowledge_warning(*airspace, false);
    airspace_warnings.acknowledge_day(*airspace, false);
    wAirspaceList->invalidate();
  }
}

static void OnCloseClicked(WindowControl * Sender) {
	(void)Sender;

  wf->hide();
  wf->SetModalResult(mrOK);

}


static bool
OnKeyDown(WindowControl *Sender, unsigned key_code)
{
  switch(key_code){
    case VK_ESCAPE:
      OnCloseClicked(Sender);
    return true;

#ifdef GNAV
    case VK_APP1:
    case '6':
      OnAckClicked(Sender);
    return true;

    case VK_APP2:
    case '7':
      OnAck1Clicked(Sender);
    return true;

    case VK_APP3:
    case '8':
      OnAck2Clicked(Sender);
    return true;

    case VK_APP4:
    case '9':
      OnEnableClicked(Sender);
    return true;
#endif

  default:
    return false;
  }
}


static void
OnAirspaceListItemPaint(Canvas &canvas, const RECT paint_rc, unsigned i)
{
  TCHAR sTmp[128];

  ProtectedAirspaceWarningManager::Lease lease(airspace_warnings);
  if (lease->empty()) {
    assert(i == 0);

    canvas.text(paint_rc.left + IBLSCALE(2),
                paint_rc.top + IBLSCALE(2), _("No Warnings"));
    return;
  }

  assert(i < lease->size());

  const AirspaceWarning* _warning = lease->get_warning(i);

  const AirspaceWarning warning = *_warning;

  const AbstractAirspace& as = warning.get_airspace();
  const AirspaceInterceptSolution& solution = warning.get_solution();

  tstring sName = as.get_name_text(false);
  tstring sTop = as.get_top_text(true);
  tstring sBase = as.get_base_text(true);
  tstring sType = as.get_type_text(true);

  const int TextHeight = 12, TextTop = 1;
  const int Col0Left = 3, Col1Left = 120, Col2Left = 200;

  RECT         rcTextClip;
    
  rcTextClip = paint_rc;
  rcTextClip.right = IBLSCALE(Col1Left - 2);

  Color old_text_color = canvas.get_text_color();
  if (!warning.get_ack_expired())
    canvas.set_text_color(Color::GRAY);

  { // name, altitude info
    _stprintf(sTmp, _T("%-20s"), sName.c_str());

    canvas.text_clipped(paint_rc.left + IBLSCALE(Col0Left),
                        paint_rc.top + IBLSCALE(TextTop),
                        rcTextClip, sTmp);
    
    _stprintf(sTmp, _T("%-20s"), sTop.c_str());
    canvas.text(paint_rc.left + IBLSCALE(Col1Left),
                paint_rc.top + IBLSCALE(TextTop), sTmp);
    
    _stprintf(sTmp, _T("%-20s"), sBase.c_str());
    canvas.text(paint_rc.left + IBLSCALE(Col1Left),
                paint_rc.top + IBLSCALE(TextTop + TextHeight),
                sTmp);
  }

  if (warning.get_warning_state() != AirspaceWarning::WARNING_INSIDE &&
      warning.get_warning_state() > AirspaceWarning::WARNING_CLEAR) {

    _stprintf(sTmp, _T("%d secs dist %d m"),
              (int)solution.elapsed_time,
              (int)solution.distance);

    canvas.text_clipped(paint_rc.left + IBLSCALE(Col0Left),
                        paint_rc.top + IBLSCALE(TextTop + TextHeight),
                        rcTextClip, sTmp);
  }

  /* draw the warning state indicator */

  Brush *state_brush;
  const TCHAR *state_text;

  if (warning.get_warning_state() == AirspaceWarning::WARNING_INSIDE) {
    if (warning.get_ack_expired())
      state_brush = &hBrushInsideBk;
    else
      state_brush = &hBrushInsideAckBk;
    state_text = _T("inside");
  } else if (warning.get_warning_state() > AirspaceWarning::WARNING_CLEAR) {
    if (warning.get_ack_expired())
      state_brush = &hBrushNearBk;
    else
      state_brush = &hBrushNearAckBk;
    state_text = _T("near");
  } else {
    state_brush = NULL;
    state_text = NULL;
  }

  const SIZE state_text_size = canvas.text_size(state_text != NULL
                                                ? state_text : _T("W"));

  if (state_brush != NULL) {
    /* colored background */
    RECT rc;

    rc.left = paint_rc.left + Layout::FastScale(Col2Left);
    rc.top = paint_rc.top + Layout::FastScale(2);
    rc.right = rc.left + state_text_size.cx + Layout::FastScale(4);
    rc.bottom = paint_rc.bottom - Layout::FastScale(2);

    canvas.fill_rectangle(rc, *state_brush);
  }

  if (state_text != NULL)
    canvas.text(paint_rc.left + Layout::FastScale(Col2Left + 2),
                (paint_rc.bottom + paint_rc.top - state_text_size.cy) / 2,
                state_text);

/*  
  
  TCHAR sAckIndicator[6] = _T(" -++*");

  if (pAS.Inside){
    
    _stprintf(sTmp, _T("> %c %s"), sAckIndicator[pAS.Acknowledge], sType);
    
  } else {
    
    TCHAR DistanceText[MAX_PATH];
    if (pAS.hDistance == 0) {
      
      // Directly above or below airspace
      
      Units::FormatUserAltitude(fabs((double)pAS.vDistance),DistanceText, 7);
      if (pAS.vDistance > 0) {
        _stprintf(sTmp, _T("< %c %s ab %s"),
                  sAckIndicator[pAS.Acknowledge],
                  sType, DistanceText);
      }
      if (pAS.vDistance < 0) {
        Units::FormatUserAltitude(fabs((double)pAS.vDistance),DistanceText, 7);
        _stprintf(sTmp, _T("< %c %s bl %s"),
                  sAckIndicator[pAS.Acknowledge],
                  sType, DistanceText);
      }
    } else {
      if ((pAS.vDistance == 0) ||
          (pAS.hDistance < abs(pAS.vDistance)*30 )) {
        
        // Close to airspace altitude, horizontally separated
        
        Units::FormatUserDistance(fabs((double)pAS.hDistance),DistanceText, 7);
        _stprintf(sTmp, _T("< %c %s H %s"), sAckIndicator[pAS.Acknowledge],
                  sType, DistanceText);
      } else {
        
        // Effectively above or below airspace, steep climb or descent
        // necessary to enter
        
        Units::FormatUserAltitude(fabs((double)pAS.vDistance),DistanceText, 7);
        if (pAS.vDistance > 0) {
          _stprintf(sTmp, _T("< %c %s ab %s"),
                    sAckIndicator[pAS.Acknowledge],
                    sType, DistanceText);
        } else {
          _stprintf(sTmp, _T("< %c %s bl %s"),
                    sAckIndicator[pAS.Acknowledge], sType,
                    DistanceText);
        }
      }
    }
  }
  canvas.text_clipped(paint_rc.left + IBLSCALE(Col0Left),
                      paint_rc.top + IBLSCALE(TextTop + TextHeight),
                      rcTextClip, sTmp);
*/  

  if (!warning.get_ack_expired())
    canvas.set_text_color(old_text_color);
}



static bool
update_list()
{
  unsigned Count = airspace_warnings.warning_size();
  if (Count > 0) {
    wAirspaceList->SetLength(Count);

    int i = -1;
    if (CursorAirspace != NULL) {
      i = airspace_warnings.get_warning_index(*CursorAirspace);
      if (i >= 0)
        wAirspaceList->SetCursorIndex(i);
    }

    if (i < 0)
      /* the selection may have changed, update CursorAirspace */
      AirspaceWarningCursorCallback(wAirspaceList->GetCursorIndex());

    wAirspaceList->invalidate();

    return true;
  } else {
    wAirspaceList->SetLength(1);

    CursorAirspace = NULL;

    if (wf && wf->is_visible())
      // auto close
      OnCloseClicked(NULL);
    else
      wAirspaceList->invalidate();

    return false;
  }
}

static void
OnTimer(WindowControl * Sender)
{
  update_list();
}

bool dlgAirspaceWarningVisible()
{
  return wf && wf->is_visible();
}


bool dlgAirspaceWarningIsEmpty() 
{
  return airspace_warnings.warning_empty();
}


void dlgAirspaceWarningShowDlg()
{
  assert(wf != NULL);
  assert(wAirspaceList != NULL);

  update_list();

  if (wf->is_visible()) {
    return;
  } else {
    // JMW need to deselect everything on new reopening of dialog
    CursorAirspace = NULL;
    FocusAirspace = NULL;
    wf->ShowModal();
  }
}


static CallBackTableEntry CallBackTable[]={
  DeclareCallBackEntry(OnAckClicked),
  DeclareCallBackEntry(OnAck1Clicked),
  DeclareCallBackEntry(OnAck2Clicked),
  DeclareCallBackEntry(OnEnableClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};

void
dlgAirspaceWarningInit(SingleWindow &parent)
{
  wf = LoadDialog(CallBackTable,
                      parent,
                      _T("IDR_XML_AIRSPACEWARNING"));
  if (wf == NULL)
    return;

  wf->SetKeyDownNotify(OnKeyDown);
  wf->SetTimerNotify(OnTimer);

  hBrushInsideBk.set(Color(254,50,50));
  hBrushNearBk.set(Color(254,254,50));
  hBrushInsideAckBk.set(Color(254,100,100));
  hBrushNearAckBk.set(Color(254,254,100));

  wAirspaceList = (WndListFrame*)wf->FindByName(_T("frmAirspaceWarningList"));
  wAirspaceList->SetPaintItemCallback(OnAirspaceListItemPaint);
  wAirspaceList->SetCursorCallback(AirspaceWarningCursorCallback);

  if (!has_pointer())
    /* on platforms without a pointing device (e.g. ALTAIR), allow
       "focusing" an airspace by pressing enter */
    wAirspaceList->SetActivateCallback(OnAirspaceListEnter);
}

void dlgAirspaceWarningDeInit()
{
  if (dlgAirspaceWarningVisible()) {
    wf->hide();
    wf->SetModalResult(mrOK);
  }
  delete wf;
  wf = NULL;
}

