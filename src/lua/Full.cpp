// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Full.hpp"
#include "Basic.hpp"
#include "Util.hxx"
#include "Log.hpp"
#include "Persistent.hpp"
#include "Http.hpp"
#include "Timer.hpp"
#include "Geo.hpp"
#include "Map.hpp"
#include "Blackboard.hpp"
#include "Dialogs.hpp"
#include "Legacy.hpp"
#include "LocalPath.hpp"
#include "Compatibility/path.h"
#include "system/Path.hpp"
#include "util/ConvertString.hpp"
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
  InitHttp(L);
  InitTimer(L);
  InitGeo(L);
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
