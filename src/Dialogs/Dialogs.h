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

#if !defined(XCSOAR_DIALOGS_H)
#define XCSOAR_DIALOGS_H

#include <tchar.h>

class SingleWindow;

void dlgAlternatesListShowModal(SingleWindow &parent);

void dlgBasicSettingsShowModal();
void dlgBrightnessShowModal();
void dlgHelpShowModal(SingleWindow &parent, const TCHAR* Caption,
    const TCHAR* HelpText);

void dlgChecklistShowModal();
void dlgConfigurationShowModal();
void dlgConfigFontsShowModal();

void dlgVegaDemoShowModal();
bool dlgConfigurationVarioShowModal();
void dlgLoggerReplayShowModal();

/**
 * @return true on success, false if the user has pressed the "Quit"
 * button
 */
bool
dlgStartupShowModal();

void dlgWindSettingsShowModal();

void dlgStatusShowModal(int page);

void dlgSwitchesShowModal();

void
dlgInfoBoxAccessShowModal(SingleWindow &parent, const int id);

void dlgVoiceShowModal();

void dlgThermalAssistantShowModal();

void dlgCreditsShowModal(SingleWindow &parent);

#endif
