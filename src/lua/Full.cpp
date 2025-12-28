// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Full.hpp"
#include "Basic.hpp"
#include "Util.hxx"
#include "Log.hpp"
#include "Persistent.hpp"
#ifdef HAVE_HTTP
#include "Http.hpp"
#endif
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
#ifdef HAVE_HTTP
#include "Tracking.hpp"
#endif
#include "Replay.hpp"
#include "InputEvent.hpp"

lua_State *
Lua::NewFullState()
{
  lua_State *L = NewBasicState();

  InitLog(L);
  InitPersistent(L);
#ifdef HAVE_HTTP
  InitHttp(L);
#endif
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
#ifdef HAVE_HTTP
  InitTracking(L);
#endif
  InitReplay(L);
  InitInputEvent(L);

  {
    SetPackagePath(L,
                   WideToUTF8Converter(LocalPath(_T("lua" DIR_SEPARATOR_S "?.lua")).c_str()));
  }

  return L;
}
