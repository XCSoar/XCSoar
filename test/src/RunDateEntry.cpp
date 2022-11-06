/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#define ENABLE_DIALOG
#define ENABLE_MAIN_WINDOW

#include "Main.hpp"
#include "Dialogs/DateEntry.hpp"
#include "time/BrokenDate.hpp"

#include <stdio.h>

static void
Main([[maybe_unused]] TestMainWindow &main_window)
{
  auto value = BrokenDate::TodayUTC();
  if (!DateEntryDialog(_T("The caption"), value, true))
    return;

  if (value.IsPlausible())
    printf("%04u-%02u-%02u\n", value.year, value.month, value.day);
  else
    printf("invalid\n");
}
