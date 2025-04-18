// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Audio/Features.hpp"
#include "Audio/Sound.hpp"

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/SoundUtil.hpp"
#include "Android/Context.hpp"
#endif

#if defined(_WIN32)
#include "ResourceLoader.hpp"
#include <mmsystem.h>
#elif defined(HAVE_PCM_PLAYER)
#include "GlobalPCMResourcePlayer.hpp"
#include "PCMResourcePlayer.hpp"
#endif

bool
PlayResource(const TCHAR *resource_name)
{
#ifdef ANDROID

  if (_tcsstr(resource_name, _T(".wav")))
    return SoundUtil::PlayExternal(Java::GetEnv(), context->Get(), resource_name);
  return SoundUtil::Play(Java::GetEnv(), context->Get(), resource_name);

#elif defined(_WIN32)

  if (_tcsstr(resource_name, TEXT(".wav")))
    return sndPlaySound(resource_name, SND_ASYNC | SND_NODEFAULT);

  ResourceLoader::Data data = ResourceLoader::Load(resource_name, _T("WAVE"));
  return data.data() != nullptr &&
    sndPlaySound((LPCTSTR)data.data(), SND_MEMORY | SND_ASYNC | SND_NODEFAULT);

#elif defined(HAVE_PCM_PLAYER)

  if (nullptr == pcm_resource_player)
    return false;

  return pcm_resource_player->PlayResource(resource_name);

#else
  return false;
#endif
}
