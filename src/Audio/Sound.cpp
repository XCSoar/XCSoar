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

#include "Audio/Sound.hpp"
#include "ResourceLoader.hpp"

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#include "Android/SoundUtil.hpp"
#endif

#ifndef DISABLEAUDIO
#include <windows.h>
#include <mmsystem.h>
#endif

bool PlayResource (const TCHAR* lpName)
{
#ifdef ANDROID
  return sound_util != NULL &&
    sound_util->play(Java::GetEnv(), native_view->get_context(), lpName);
#elif defined(DISABLEAUDIO)
  return false;
#else
  BOOL bRtn;

  // TODO code: Modify to allow use of WAV Files and/or Embedded files

  if (_tcsstr(lpName, TEXT(".wav"))) {
    bRtn = sndPlaySound (lpName, SND_ASYNC | SND_NODEFAULT );

  } else {
    ResourceLoader::Data data = ResourceLoader::Load(lpName, _T("WAVE"));
    return data.first != NULL &&
      sndPlaySound((LPCTSTR)data.first,
                   SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
  }
  return bRtn;
#endif
}
