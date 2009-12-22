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

#include "Airspace/AirspaceWarningManager.hpp"
#include "Dialogs/Internal.hpp"
#include "Units.hpp"
#include "Protection.hpp"
#include "Math/FastMath.h"
#include "Math/Units.h"
#include "MainWindow.hpp"
#include "Components.hpp"
#include "Screen/Layout.hpp"
#include "Compatibility/vk.h"

#include <assert.h>
#include <stdlib.h>

#ifndef _MSC_VER
#include <algorithm>
using std::max;
#endif

static WndForm *wf=NULL;
static WndListFrame *wAirspaceList=NULL;
static Brush hBrushInsideBk;
static Brush hBrushNearBk;
static Brush hBrushInsideAckBk;
static Brush hBrushNearAckBk;

static const AbstractAirspace* CursorAirspace = NULL; // Current list cursor airspace
static const AbstractAirspace* FocusAirspace = NULL;  // Current action airspace

#include "RasterTerrain.h"
// just to get the lock OLD_TASK

static void
OnAirspaceListEnter(unsigned i)
{
  terrain.Lock();
  AirspaceWarning* warning = airspace_warning.get_warning(wAirspaceList->GetCursorIndex());
  if (warning) {
    FocusAirspace = &warning->get_airspace();
  } else {
    FocusAirspace = NULL;
    // error!
  }
  terrain.Unlock();
}


static void DoAck(int Ack) {
  if (!FocusAirspace) {
    FocusAirspace = CursorAirspace;
  }
  terrain.Lock();
  AirspaceWarning* warning = airspace_warning.get_warning_ptr(*FocusAirspace);
  if (warning) {
    switch(Ack) {
    case -1:
      warning->acknowledge_warning(true);
      break;
    case 0:
      warning->acknowledge_warning(false);
      warning->acknowledge_day(false);
      break;
    case 3:
      warning->acknowledge_inside(true);
      break;
    case 4:
      warning->acknowledge_day(true);
      break;
    default:
      break;
    };
    wAirspaceList->invalidate();
  }
  terrain.Unlock();
}

static void OnAckClicked(WindowControl * Sender){
  (void)Sender;
  DoAck(3); // ack inside
}

static void OnAck1Clicked(WindowControl * Sender){
  (void)Sender;
  DoAck(-1); // ack warn
}

static void OnAck2Clicked(WindowControl * Sender){
  (void)Sender;
  DoAck(4); // ack day
}

static void OnEnableClicked(WindowControl * Sender) {
  (void)Sender;
  DoAck(0); // unack
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

  terrain.Lock();
  const AirspaceWarning* warning = airspace_warning.get_warning(i);
  terrain.Unlock();

  if (!warning) {
    if (i == 0){
      _stprintf(sTmp, _T("%s"), gettext(_T("No Warnings")));
      canvas.text_opaque(paint_rc.left + IBLSCALE(2),
                         paint_rc.top + IBLSCALE(2), sTmp);
    }
    return;
  }

  const AbstractAirspace& as = warning->get_airspace();
  const AirspaceInterceptSolution& solution = warning->get_solution();

  if (i == wAirspaceList->GetCursorIndex())
    CursorAirspace = &as;

  TCHAR sAckIndicator[6] = _T(" -++*");

  tstring sName = as.get_name_text(false);
  tstring sTop = as.get_top_text(true);
  tstring sBase = as.get_base_text(true);
  tstring sType = as.get_type_text(true);

  int          TextHeight = 12;
  int          TextTop = 1;
  int          Col0Left = 3;
  int          Col1Left = 120;
  RECT         rc;
  RECT         rcTextClip;
  Brush *hBrushBk = NULL;
    
  rc = rcTextClip = paint_rc;
  rcTextClip.right = IBLSCALE(Col1Left - 2);

  InflateRect(&rc, IBLSCALE(-2), IBLSCALE(-2));

  { // background colour
    if (warning->get_warning_state() == AirspaceWarning::WARNING_INSIDE) {
      if (warning->get_ack_expired())
        hBrushBk = &hBrushInsideBk;
      else
        hBrushBk = &hBrushInsideAckBk;
    } else if (warning->get_warning_state() > AirspaceWarning::WARNING_CLEAR) {
      if (warning->get_ack_expired())
        hBrushBk = &hBrushNearBk;
      else
        hBrushBk = &hBrushNearAckBk;
    }

    if (hBrushBk != NULL) {
      canvas.fill_rectangle(rc, *hBrushBk);
    }
  }

  if (!warning->get_ack_expired()){
    canvas.set_text_color(Color::GRAY);
  }

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

  if (warning->get_warning_state() != AirspaceWarning::WARNING_INSIDE &&
      warning->get_warning_state() > AirspaceWarning::WARNING_CLEAR) {

    _stprintf(sTmp, _T("%d secs dist %d m"),
              solution.elapsed_time.as_int(),
              solution.distance.as_int());

    canvas.text_clipped(paint_rc.left + IBLSCALE(Col0Left),
                        paint_rc.top + IBLSCALE(TextTop + TextHeight),
                        rcTextClip, sTmp);
  }
  
/*  
  
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
}



static bool
update_list()
{
  // JMW OLD_TASK this locking is just for now since we don't have any protection
  terrain.Lock();
  unsigned Count = airspace_warning.size();
  if (Count) {
    wAirspaceList->SetLength(std::max((unsigned)1, Count));
    if (CursorAirspace) {
      wAirspaceList->SetCursorIndex(airspace_warning.get_warning_index(*CursorAirspace));
    } else {
      wAirspaceList->SetCursorIndex(0);
    }
  }
  terrain.Unlock();

  if (!Count) {
    if (wf && wf->is_visible()) {
      // auto close
      OnCloseClicked(NULL);
    }
    return false;
  } else {
    wAirspaceList->invalidate();
    return true;
  }
}


static int OnTimer(WindowControl * Sender) {
  update_list();
  return(0);
}


bool dlgAirspaceWarningVisible() 
{
  return wf && wf->is_visible();
}


bool dlgAirspaceWarningIsEmpty() 
{
  // JMW OLD_TASK this locking is just for now since we don't have any protection
  terrain.Lock();
  bool retval = airspace_warning.empty();
  terrain.Unlock();
  return retval;
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


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnAckClicked),
  DeclareCallBackEntry(OnAck1Clicked),
  DeclareCallBackEntry(OnAck2Clicked),
  DeclareCallBackEntry(OnEnableClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};


void dlgAirspaceWarningInit()
{
  wf = dlgLoadFromXML(CallBackTable,
                      _T("dlgAirspaceWarning.xml"),
                      XCSoarInterface::main_window,
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
  wAirspaceList->SetActivateCallback(OnAirspaceListEnter);
}

void dlgAirspaceWarningDeInit()
{
  delete wf;
  wf = NULL;
}

