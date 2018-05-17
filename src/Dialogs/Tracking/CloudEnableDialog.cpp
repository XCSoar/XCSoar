/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "CloudEnableDialog.hpp"
#include "Tracking/SkyLines/Features.hpp"
#include "Tracking/SkyLines/Key.hpp"
#include "Dialogs/Message.hpp"
#include "Net/State.hpp"
#include "Asset.hpp"
#include "Interface.hpp"
#include "Simulator.hpp"
#include "Language/Language.hpp"
#include "Profile/Profile.hpp"
#include "UIGlobals.hpp"
#include "Screen/SingleWindow.hpp"

void
CloudEnableDialog()
{
#ifdef HAVE_SKYLINES_TRACKING
  if (!IsAndroid() && !IsDebug())
    /* currently only enabled on Android */
    return;

  auto &settings =
    CommonInterface::SetComputerSettings().tracking.skylines.cloud;
  if (settings.enabled != TriState::UNKNOWN)
    /* explicitly enabled or disabled - don't ask further questions */
    return;

  if (UIGlobals::GetMainWindow().HasDialog())
    /* don't ask stupid questions while the user operates in another
       modal dialog */
    return;

  if (is_simulator())
    /* only check in "Fly" mode */
    return;

  if (CommonInterface::MovementDetected())
    /* don't ask stupid questions if we're flying or about to */
    return;

#ifdef HAVE_NET_STATE
  if (GetNetState() != NetState::CONNECTED)
    /* this feature requires an internet connection; don't bother
       asking if we're not connected */
    return;
#endif

  const TCHAR *msg = _("The XCSoar project is currently developing a revolutionary service which allows sharing thermal/wave locations and more with other pilots.\n"
                       "Do you wish to participate in the field test? This means that your position, thermal/wave locations and other weather data will be transmitted to our test server. You can disable it at any time in the \"Tracking\" settings.\n"
                       "Please help us improve XCSoar!");

  int result = ShowMessageBox(msg, _T("XCSoar Cloud"),
                              MB_YESNOCANCEL|MB_ICONQUESTION);

  switch (result) {
  case IDYES:
    settings.enabled = TriState::TRUE;
    Profile::Set(ProfileKeys::CloudEnabled, true);

    if (settings.key == 0) {
      settings.key = SkyLinesTracking::GenerateKey();

      char s[64];
      snprintf(s, sizeof(s), "%llx",
               (unsigned long long)settings.key);
      Profile::Set(ProfileKeys::CloudKey, s);
    }

    Profile::Save();
    break;

  case IDNO:
    settings.enabled = TriState::FALSE;
    Profile::Set(ProfileKeys::CloudEnabled, false);
    Profile::Save();
    break;

  default:
    /* prevent further dialogs during this XCSoar run, but ask again
       later (don't save to profile) */
    settings.enabled = TriState::FALSE;
  }
#endif
}
