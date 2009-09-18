/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "MapWindow.h"
#include "UtilsSystem.hpp"
#include "SettingsUser.hpp"
#include "SettingsTask.hpp"
#include "SettingsComputer.hpp"
#include "Settings.hpp"
#include "Protection.hpp"
#include "InputEvents.h"
#include "Language.hpp"
#include "Message.h"
#include "Components.hpp"
#include "Task.h"
#include "InfoBoxLayout.h"
#include "Dialogs.h"
#include "Screen/Graphics.hpp"
#include "XCSoar.h"
#include "options.h"
#include "McReady.h"
#include "Math/Geometry.hpp"
#include "Math/Earth.hpp"
#include "Screen/Fonts.hpp"

#ifdef _SIM_
#include "DeviceBlackboard.hpp"
#endif
#include <stdlib.h>

/////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
// Interface/touchscreen callbacks
//

/*
	Virtual Key Manager by Paolo Ventafridda

	Returns 0 if invalid virtual scan code, otherwise a valid
	transcoded keycode.

 */
int MapWindow::ProcessVirtualKey(int X, int Y, long keytime, short vkmode) {

// 0 is always thermal mode, and does not account
#define MAXBOTTOMMODES 5
#define VKTIMELONG 1500

#ifdef DEBUG_PROCVK
  TCHAR buf[100];
  wsprintf(buf,_T("R=%d,%d,%d,%d, X=%d Y=%d kt=%ld"),
	   MapRect.left, MapRect.top,
	   MapRect.right, MapRect.bottom,
	   X,Y,keytime);
  DoStatusMessage(buf);
#endif
  
  short sizeup=MapRect.bottom-MapRect.top;
  short yup=(sizeup/3)+MapRect.top;
  short ydown=MapRect.bottom-(sizeup/3);
  
  if (Y<yup) {
    if (keytime>=VKTIMELONG)
      return 0xc1;
    else
      return 38;
  }
  if (Y>ydown) {
    if (keytime>=VKTIMELONG)
      return 0xc2;
    else
      
      return 40;
  }

  /*
   * FIX ready: do not pass virtual ENTER while in Panmode.
   * Currently it is allowed, should be better tested.  VNT 090702
   if ( !MapWindow::EnablePan ) {
     DoStatusMessage(_T("Virtual ENTER"));
     return 13;
   }
   return 0; // ignore it
  */
  return 13;
//	}
  Message::AddMessage(_T("VirtualKey Error"));
  return 0;
}


//////////

/////////////////////////////////////////

bool MapWindow::on_resize(unsigned width, unsigned height) {
  MaskedPaintWindow::on_resize(width, height);

  draw_canvas.resize(width, height);
  buffer_canvas.resize(width, height);

  SetFontInfoAll(get_canvas());

  return true;
}

bool MapWindow::on_create()
{
  if (!MaskedPaintWindow::on_create())
    return false;

  draw_canvas.set(get_canvas());
  buffer_canvas.set(get_canvas());
  return true;
}

bool MapWindow::on_destroy()
{
  draw_canvas.reset();
  buffer_canvas.reset();

  MaskedPaintWindow::on_destroy();
  return true;
}

///////

void MapWindow::on_paint(Canvas& _canvas) {
  mutexBuffer.Lock();
  _canvas.copy(draw_canvas);
  mutexBuffer.Unlock();
}

bool
MapWindow::on_setfocus()
{
  MaskedPaintWindow::on_setfocus();

  return true;
}

//////////

static GEOPOINT LLstart;
static int XstartScreen, YstartScreen;
static bool ignorenext=true;

bool MapWindow::on_mouse_double(int x, int y)
{
  // Added by ARH to show menu button when mapwindow is double clicked.
  //
  // VNT TODO: do not handle this event and remove CS_DBLCLKS in register class.
  // Only handle timed clicks in BUTTONDOWN with no proximity.
  //
  mouse_down_clock.update();
  InputEvents::ShowMenu();
  ignorenext = true;
  return true;
}

bool MapWindow::on_mouse_move(int x, int y)
{
  if (AATEnabled && SettingsMap().TargetPan && (TargetDrag_State>0)) {
    // target follows "finger" so easier to drop near edge of
    // sector
    if (TargetDrag_State == 1) {
      GEOPOINT mouseMove;
      Screen2LonLat((int)x, (int)y, mouseMove);
      if (task.InAATTurnSector(mouseMove, 
                               SettingsMap().TargetPanIndex)) {
	// update waypoints so if we drag out of the cylinder, it
	// will remain adjacent to the edge

        mutexTaskData.Lock();
	task_stats[SettingsMap().TargetPanIndex].AATTargetLocation = mouseMove;
	TargetDrag_Location = mouseMove;
        mutexTaskData.Unlock();

	draw_masked_bitmap(get_canvas(), MapGfx.hBmpTarget, x, y, 10, 10, true);
      }
    }
  }
  return true;
}

bool MapWindow::on_mouse_down(int x, int y)
{
  mouse_down_clock.update();
  if (ignorenext) return true;

  set_focus();

  // TODO VNT move Screen2LonLat in LBUTTONUP after making sure we
  // really need Xstart and Ystart so we save precious
  // milliseconds waiting for BUTTONUP GetTickCount
  Screen2LonLat(x, y, LLstart);
  XstartScreen = x;
  YstartScreen = y;

  if (AATEnabled && SettingsMap().TargetPan) {
    if (task.ValidTaskPoint(SettingsMap().TargetPanIndex)) {
      POINT tscreen;
      LonLat2Screen(task.getTargetLocation(SettingsMap().TargetPanIndex),
		    tscreen);
      double distance = isqrt4((long)((XstartScreen-tscreen.x)
			       *(XstartScreen-tscreen.x)+
			       (YstartScreen-tscreen.y)
			       *(YstartScreen-tscreen.y)))
	/InfoBoxLayout::scale;
      
      if (distance<10) {
	TargetDrag_State = 1;
      }
    }
  }
  return true;
}




bool MapWindow::on_mouse_up(int x, int y)
{
  if (ignorenext) {
    ignorenext=false;
    return true;
  }

  int dwInterval = mouse_down_clock.elapsed();
  mouse_down_clock.reset();
  if (dwInterval < 0)
    return true;

  RECT rc = MapRect;
  bool my_target_pan = SettingsMap().TargetPan;

  if (dwInterval == 0) {
#ifdef DEBUG_VIRTUALKEYS
    Message::AddMessage(_T("dwInterval==0 impossible!"));
#endif
    return true; // should be impossible
  }

  double distance = isqrt4((long)((XstartScreen-x)*(XstartScreen-x)+
			   (YstartScreen-y)*(YstartScreen-y)))
    /InfoBoxLayout::scale;

#ifdef DEBUG_VIRTUALKEYS
  TCHAR buf[80]; char sbuf[80];
  sprintf(sbuf,"%.0f",distance);
  _stprintf(buf,_T("XY=%d,%d dist=%S Up=%ld Down=%ld Int=%ld"),
	    x,y,sbuf,dwUpTime,dwDownTime,dwInterval);
  Message::AddMessage(buf);
#endif

  // Caution, timed clicks from PC with a mouse are different
  // from real touchscreen devices
  
  if ((distance<50) 
       && (CommonInterface::VirtualKeys==(VirtualKeys_t)vkEnabled) 
       && (dwInterval>= DOUBLECLICKINTERVAL)) {
    unsigned wParam=ProcessVirtualKey(x,y,dwInterval,0);
    if (wParam==0) {
#ifdef DEBUG_VIRTUALKEYS
      Message::AddMessage(_T("E02 INVALID Virtual Key!"));
#endif
      return true;
    }
    //    dwDownTime= 0L;
    //    InputEvents::processKey(wParam);
    //    return;
  }

  GEOPOINT G;
  Screen2LonLat(x, y, G);

  if (AATEnabled && my_target_pan && (TargetDrag_State>0)) {
    TargetDrag_State = 2;
    if (task.InAATTurnSector(G, SettingsMap().TargetPanIndex)) {
      // if release mouse out of sector, don't update w/ bad coords
      TargetDrag_Location = G;
    }
    return true;
  }
 
  if (!my_target_pan && SettingsMap().EnablePan && (distance>IBLSCALE(36))) {
    // JMW broken!
    PanLocation.Longitude += LLstart.Longitude-G.Longitude;
    PanLocation.Latitude  += LLstart.Latitude-G.Latitude;
    RefreshMap();
    return true;
  }

#ifdef _SIM_
  if (!Basic().Replay && !my_target_pan && (distance>IBLSCALE(36))) {
    // This drag moves the aircraft (changes speed and direction)
    double oldbearing = XCSoarInterface::Basic().TrackBearing;
    double minspeed = 1.1*GlidePolar::Vminsink;
    double newbearing = Bearing(LLstart, G);
    if ((fabs(AngleLimit180(newbearing-oldbearing))<30)
	|| (XCSoarInterface::Basic().Speed<minspeed)) {

      device_blackboard.SetSpeed(min(100.0,max(minspeed,distance/3)));
    }
    device_blackboard.SetTrackBearing(newbearing);
    // change bearing without changing speed if direction change > 30
    // 20080815 JMW prevent dragging to stop glider
    
    // JMW trigger recalcs immediately
    TriggerGPSUpdate();
    return true;
  }
#endif

  if (!my_target_pan) {
    if (CommonInterface::VirtualKeys==(VirtualKeys_t)vkEnabled) {
      if(dwInterval < VKSHORTCLICK) {
	//100ms is NOT enough for a short click since GetTickCount
	//is OEM custom!
        if (PopupNearestWaypointDetails(way_points, LLstart,
					DistancePixelsToMeters(IBLSCALE(10)), false)) {
	  return true;
	}
      } else {
	if (PopupInteriorAirspaceDetails(LLstart)) {
	  return true;
	}
      }
    } else {
      if(dwInterval < AIRSPACECLICK) { // original and untouched interval
        if (PopupNearestWaypointDetails(way_points, LLstart,
					DistancePixelsToMeters(IBLSCALE(10)), false)) {
	  return true;
	}
      } else {
	if (PopupInteriorAirspaceDetails(LLstart)) {
	  return true;
	}
      }
    } // VK enabled
  } // !TargetPan

  return false;
}


bool MapWindow::on_key_down(unsigned key_code)
{
  // VENTA-TODO careful here, keyup no more trapped for PNA.
  // Forbidden usage of keypress timing.
  
  key_code = TranscodeKey(key_code);
#if defined(GNAV)
  if (key_code == 0xF5){
    XCSoarInterface::SignalShutdown(false);
    return true;
  }
#endif

  mouse_down_clock.reset();
  if (InputEvents::processKey(key_code)) {
    return true; // don't go to default handler
  }

  return MaskedPaintWindow::on_key_down(key_code);
}
