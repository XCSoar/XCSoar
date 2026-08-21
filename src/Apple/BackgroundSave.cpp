// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BackgroundSave.hpp"

#include <TargetConditionals.h>

#if TARGET_OS_IPHONE

#include "Startup.hpp"
#include "LogFile.hpp"

#include <SDL_events.h>

/**
 * Unlike events which are dispatched by our event loop, an SDL event
 * watch runs synchronously inside the UIApplicationDelegate method
 * which posted the event.  For the background transition, that is the
 * only moment when iOS still lets us write to disk; once the delegate
 * method has returned, the process gets suspended and may never be
 * resumed.
 */
static int SDLCALL
OnApplicationEvent([[maybe_unused]] void *ctx, SDL_Event *event) noexcept
{
  switch (event->type) {
  case SDL_APP_DIDENTERBACKGROUND:
  case SDL_APP_TERMINATING:
    LogString("Entering background, saving user state");
    SaveUserState();
    break;
  }

  /* keep the event, it is none of our business */
  return 1;
}

void
InitializeAppleBackgroundSave() noexcept
{
  SDL_AddEventWatch(OnApplicationEvent, nullptr);
}

void
DeinitializeAppleBackgroundSave() noexcept
{
  SDL_DelEventWatch(OnApplicationEvent, nullptr);
}

#else /* !TARGET_OS_IPHONE */

void
InitializeAppleBackgroundSave() noexcept
{
}

void
DeinitializeAppleBackgroundSave() noexcept
{
}

#endif /* !TARGET_OS_IPHONE */
