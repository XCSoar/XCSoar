/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Full.hpp"
#include "Basic.hpp"
#include "Util.hxx"
#include "Log.hpp"
#include "Persistent.hpp"
#include "Timer.hpp"
#include "Map.hpp"
#include "Blackboard.hpp"
#include "Dialogs.hpp"
#include "Legacy.hpp"
#include "LocalPath.hpp"
#include "Compatibility/path.h"
#include "OS/Path.hpp"
#include "Util/ConvertString.hpp"
#include "Airspace.hpp"
#include "Task.hpp"
#include "Settings.hpp"
#include "Wind.hpp"
#include "Logger.hpp"
#include "Tracking.hpp"
#include "Replay.hpp"
#include "InputEvent.hpp"

lua_State *
Lua::NewFullState()
{
  lua_State *L = NewBasicState();

  InitLog(L);
  InitPersistent(L);
  InitTimer(L);
  InitMap(L);
  InitBlackboard(L);
  InitDialogs(L);
  InitLegacy(L);
  InitAirspace(L);
  InitTask(L);
  InitSettings(L);
  InitWind(L);
  InitLogger(L);
  InitTracking(L);
  InitReplay(L);
  InitInputEvent(L);

  {
    SetPackagePath(L,
                   WideToUTF8Converter(LocalPath(_T("lua" DIR_SEPARATOR_S "?.lua")).c_str()));
  }

  return L;
}
