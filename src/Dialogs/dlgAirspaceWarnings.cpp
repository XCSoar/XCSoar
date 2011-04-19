/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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
#include "Units/Units.hpp"
#include "Components.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Airspace/AirspaceWarning.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Airspace/AirspaceWarningManager.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

static WndForm *wf = NULL;
static WndButton *wbAck1 = NULL; // frmAck1 = Ack warn
static WndButton *wbAck2 = NULL; // frmAck2 = Ack day
static WndButton *wbAck = NULL; // frmAck = Ack space
static WndButton *wbEnable = NULL; // Enable

static WndListFrame *wAirspaceList = NULL;
static Brush hBrushInsideBk;
static Brush hBrushNearBk;
static Brush hBrushInsideAckBk;
static Brush hBrushNearAckBk;
static bool AutoClose = true;

static const AbstractAirspace* CursorAirspace = NULL; // Current list cursor airspace
static const AbstractAirspace* FocusAirspace = NULL;  // Current action airspace

static void
AirspaceWarningCursorCallback(unsigned i)
{
  ProtectedAirspaceWarningManager::Lease lease(*airspace_warnings);
  const AirspaceWarning *warning = lease->get_warning(i);
  CursorAirspace = (warning != NULL)
    ? &warning->get_airspace()
    : NULL;
}

static void
OnAirspaceListEnter(unsigned i)
{
  if (!has_pointer())
    /* on platforms without a pointing device (e.g. ALTAIR), allow
       "focusing" an airspace by pressing enter */
    FocusAirspace = CursorAirspace;
  else
    dlgAirspaceDetails(*CursorAirspace);
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
  ProtectedAirspaceWarningManager::Lease lease(*airspace_warnings);
  for (unsigned i = 0; i < lease->size(); ++i) {
    const AirspaceWarning *warning = lease->get_warning(i);
    if (warning != NULL && warning->get_ack_expired())
      return true;
  }

  return false;
}

static void
Hide()
{
  wf->hide();
  wf->SetModalResult(mrOK);
}

static void
AutoHide()
{
  // Close the dialog if no warning exists and AutoClose is set
  if (!HasWarning() && AutoClose)
    Hide();
}

/** ack inside */
static void
Ack()
{
  const AbstractAirspace *airspace = GetSelectedAirspace();
  if (airspace != NULL) {
    airspace_warnings->acknowledge_inside(*airspace, true);
    wAirspaceList->invalidate();
    AutoHide();
  }
}

static void
OnAckClicked(WndButton &Sender)
{
  (void)Sender;
  Ack();
}

/** ack warn */
static void
Ack1()
{
  const AbstractAirspace *airspace = GetSelectedAirspace();
  if (airspace != NULL) {
    airspace_warnings->acknowledge_warning(*airspace, true);
    wAirspaceList->invalidate();
    AutoHide();
  }
}

static void
OnAck1Clicked(WndButton &Sender)
{
  (void)Sender;
  Ack1();
}

/** ack day */
static void
Ack2()
{
  const AbstractAirspace *airspace = GetSelectedAirspace();
  if (airspace != NULL) {
    airspace_warnings->acknowledge_day(*airspace, true);
    wAirspaceList->invalidate();
    AutoHide();
  }
}

static void
OnAck2Clicked(WndButton &Sender)
{
  (void)Sender;
  Ack2();
}

/** unack */
static void
Enable()
{
  const AbstractAirspace *airspace = GetSelectedAirspace();
  if (airspace != NULL) {
    airspace_warnings->acknowledge_inside(*airspace, false);
    airspace_warnings->acknowledge_warning(*airspace, false);
    airspace_warnings->acknowledge_day(*airspace, false);
    wAirspaceList->invalidate();
  }
}

static void
OnEnableClicked(WndButton &Sender)
{
  (void)Sender;
  Enable();
}

static void
OnCloseClicked(WndButton &Sender)
{
  (void)Sender;
  Hide();
}

static bool
OnKeyDown(WndForm &Sender, unsigned key_code)
{
  switch (key_code){
    case VK_ESCAPE:
      Hide();
    return true;

#ifdef GNAV
    case VK_APP1:
    case '6':
      Ack();
    return true;

    case VK_APP2:
    case '7':
      Ack1();
    return true;

    case VK_APP3:
    case '8':
      Ack2();
    return true;

    case VK_APP4:
    case '9':
      Enable();
    return true;
#endif

  default:
    return false;
  }
}


static void
OnAirspaceListItemPaint(Canvas &canvas, const PixelRect paint_rc, unsigned i)
{
  TCHAR sTmp[128];
  const int paint_rc_margin = 2;   ///< This constant defines the margin that should be respected for renderring within the paint_rc area.

  bool ack1_vis;
  bool ack2_vis;
  bool ack_vis;
  bool enable_vis;
  bool update_vis = false;

  {
    ProtectedAirspaceWarningManager::Lease lease(*airspace_warnings);
    if (lease->empty()) {
      // the warnings were emptied between the opening of the dialog
      // and this refresh, so only need to display "No Warnings" for
      // top item, otherwise exit immediately
      if (i==0) {
        canvas.text(paint_rc.left + IBLSCALE(paint_rc_margin),
                    paint_rc.top + IBLSCALE(paint_rc_margin), _("No Warnings"));
      }
      return;
    }
    
    if (i >= lease->size())
      /* this cannot be an assertion, because another thread may have
         modified the AirspaceWarningManager */
      return;
    
    const AirspaceWarning warning = *(lease->get_warning(i));
    const AbstractAirspace& as = warning.get_airspace();
    const AirspaceInterceptSolution& solution = warning.get_solution();
    
    tstring sName = as.get_name_text(false);
    tstring sTop = as.get_top_text(true);
    tstring sBase = as.get_base_text(true);
    tstring sType = as.get_type_text(true);
    
    const int TextHeight = 12, TextTop = 1;

    const int statusColWidth = canvas.text_width(_T("inside"));     //<-- word "inside" is used as the etalon, because it is longer than "near" and currently (9.4.2011) there is no other possibility for the status text.
    const int heightColWidth = canvas.text_width(_T("1888 m AGL")); // <-- "1888" is used in order to have enough space for 4-digit heights with "AGL"


    /// Dynamic columns scaling - "name" column is flexible, altitude and state columns are fixed-width.
    const int Col0LeftScreenCoords = Layout::FastScale(paint_rc_margin),
        Col2LeftScreenCoords = paint_rc.right - Layout::FastScale(paint_rc_margin) - (statusColWidth + 2 * Layout::FastScale(paint_rc_margin)),
        Col1LeftScreenCoords = Col2LeftScreenCoords - Layout::FastScale(paint_rc_margin) - heightColWidth;

    PixelRect rcTextClip;
    
    rcTextClip = paint_rc;
    rcTextClip.right = Col1LeftScreenCoords - Layout::FastScale(paint_rc_margin);
    
    Color old_text_color = canvas.get_text_color();
    if (!warning.get_ack_expired())
      canvas.set_text_color(Color::GRAY);
    
    { // name, altitude info
      _stprintf(sTmp, _T("%-20s"), sName.c_str());
      
      canvas.text_clipped(paint_rc.left + Col0LeftScreenCoords,
                          paint_rc.top + IBLSCALE(TextTop),
                          rcTextClip, sTmp);
      
      _stprintf(sTmp, _T("%-20s"), sTop.c_str());
      canvas.text(paint_rc.left + Col1LeftScreenCoords,
                  paint_rc.top + IBLSCALE(TextTop), sTmp);
      
      _stprintf(sTmp, _T("%-20s"), sBase.c_str());
      canvas.text(paint_rc.left + Col1LeftScreenCoords,
                  paint_rc.top + IBLSCALE(TextTop + TextHeight),
                  sTmp);
    }
    
    if (warning.get_warning_state() != AirspaceWarning::WARNING_INSIDE &&
        warning.get_warning_state() > AirspaceWarning::WARNING_CLEAR) {
      
      _stprintf(sTmp, _T("%d secs dist %d m"),
                (int)solution.elapsed_time,
                (int)solution.distance);
      
      canvas.text_clipped(paint_rc.left + Col0LeftScreenCoords,
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
    
    const PixelSize state_text_size =
      canvas.text_size(state_text != NULL ? state_text : _T("W"));
    
    if (state_brush != NULL) {
      /* colored background */
      PixelRect rc;
      
      rc.left = paint_rc.left + Col2LeftScreenCoords;
      rc.top = paint_rc.top + Layout::FastScale(paint_rc_margin);
      rc.right = paint_rc.right - Layout::FastScale(paint_rc_margin);
      rc.bottom = paint_rc.bottom - Layout::FastScale(paint_rc_margin);
      
      canvas.fill_rectangle(rc, *state_brush);
    }
    
    if (state_text != NULL) {
      // -- status text will be centered inside its table cell:
      canvas.text(paint_rc.left + Col2LeftScreenCoords + Layout::FastScale(paint_rc_margin) + (statusColWidth / 2)  - (canvas.text_width(state_text) / 2),
                  (paint_rc.bottom + paint_rc.top - state_text_size.cy) / 2,
                  state_text);
    }
    
    if (!warning.get_ack_expired())
      canvas.set_text_color(old_text_color);
        
    if (CursorAirspace == &as) {
      update_vis = true;
      if (!warning.get_ack_expired()) {
        ack1_vis = false;
        ack_vis = false;
      } else {
        if (warning.get_warning_state() == 
            AirspaceWarning::WARNING_INSIDE) {
          ack_vis = true;
          ack1_vis = false;
        } else {
          ack_vis = false;
          ack1_vis = true;
        }
      }
      ack2_vis = !warning.get_ack_day();
      enable_vis = !warning.get_ack_expired();
    }
  } // close scope
  if (update_vis) {
    wbAck1->set_visible(ack1_vis);
    wbAck2->set_visible(ack2_vis);
    wbAck->set_visible(ack_vis);
    wbEnable->set_visible(enable_vis);
  }
}

static void
update_list()
{
  unsigned Count = airspace_warnings->warning_size();
  if (Count > 0) {
    wAirspaceList->SetLength(Count);

    int i = -1;
    if (CursorAirspace != NULL) {
      i = airspace_warnings->get_warning_index(*CursorAirspace);
      if (i >= 0)
        wAirspaceList->SetCursorIndex(i);
    }

    if (i < 0)
      /* the selection may have changed, update CursorAirspace */
      AirspaceWarningCursorCallback(wAirspaceList->GetCursorIndex());
  } else {
    wAirspaceList->SetLength(1);
    CursorAirspace = NULL;
  }
  wAirspaceList->invalidate();

  AutoHide();
}

static void
OnTimer(WndForm &Sender)
{
  update_list();
}

bool
dlgAirspaceWarningVisible()
{
  return (wf != NULL);
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnAckClicked),
  DeclareCallBackEntry(OnAck1Clicked),
  DeclareCallBackEntry(OnAck2Clicked),
  DeclareCallBackEntry(OnEnableClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};

void
dlgAirspaceWarningsShowModal(SingleWindow &parent, bool auto_close)
{
  if (dlgAirspaceWarningVisible())
    return;

  assert(airspace_warnings != NULL);

  wf = LoadDialog(CallBackTable, parent, _T("IDR_XML_AIRSPACEWARNINGS"));
  assert(wf != NULL);

  wbAck1 = (WndButton *)wf->FindByName(_T("frmAck1"));
  wbAck2 = (WndButton *)wf->FindByName(_T("frmAck2"));
  wbAck = (WndButton *)wf->FindByName(_T("frmAck"));
  wbEnable = (WndButton *)wf->FindByName(_T("frmEnable"));
  assert(wbAck1 != NULL);
  assert(wbAck2 != NULL);
  assert(wbAck != NULL);
  assert(wbEnable != NULL);

  wf->SetKeyDownNotify(OnKeyDown);

  hBrushInsideBk.set(Color(254,50,50));
  hBrushNearBk.set(Color(254,254,50));
  hBrushInsideAckBk.set(Color(254,100,100));
  hBrushNearAckBk.set(Color(254,254,100));

  wAirspaceList = (WndListFrame*)wf->FindByName(_T("frmAirspaceWarningList"));
  assert(wAirspaceList != NULL);
  wAirspaceList->SetPaintItemCallback(OnAirspaceListItemPaint);
  wAirspaceList->SetCursorCallback(AirspaceWarningCursorCallback);
  wAirspaceList->SetActivateCallback(OnAirspaceListEnter);

  AutoClose = auto_close;
  update_list();

  // JMW need to deselect everything on new reopening of dialog
  CursorAirspace = NULL;
  FocusAirspace = NULL;

  wf->SetTimerNotify(OnTimer);
  wAirspaceList->SetCursorIndex(0);
  wf->ShowModal();
  wf->SetTimerNotify(NULL);

  delete wf;

  // Needed for dlgAirspaceWarningVisible()
  wf = NULL;
}
