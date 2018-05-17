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

#include "Audio/Features.hpp"
#include "Audio/Sound.hpp"

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/SoundUtil.hpp"
#include "Android/Context.hpp"
#endif

#if defined(WIN32)
#include "ResourceLoader.hpp"
#include <windows.h>
#include <mmsystem.h>
#elif defined(HAVE_PCM_PLAYER)
#include "GlobalPCMResourcePlayer.hpp"
#include "PCMResourcePlayer.hpp"
#endif

bool
PlayResource(const TCHAR *resource_name)
{
#ifdef ANDROID

  return SoundUtil::Play(Java::GetEnv(), context->Get(), resource_name);

#elif defined(WIN32)

  if (_tcsstr(resource_name, TEXT(".wav")))
    return sndPlaySound(resource_name, SND_ASYNC | SND_NODEFAULT);

  ResourceLoader::Data data = ResourceLoader::Load(resource_name, _T("WAVE"));
  return !data.IsNull() &&
         sndPlaySound((LPCTSTR)data.data,
                      SND_MEMORY | SND_ASYNC | SND_NODEFAULT);

#elif defined(HAVE_PCM_PLAYER)

  if (nullptr == pcm_resource_player)
    return false;

  return pcm_resource_player->PlayResource(resource_name);

#else
  return false;
#endif
}
