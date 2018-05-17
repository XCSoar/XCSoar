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

#define ENABLE_DIALOG
#define ENABLE_MAIN_WINDOW

#include "Main.hpp"
#include "Dialogs/JobDialog.hpp"
#include "Job/Job.hpp"
#include "Operation/Operation.hpp"

class TestJob : public Job {
public:
  virtual void Run(OperationEnvironment &env) {
    env.SetText(_T("Working..."));
    env.SetProgressRange(30);
    for (unsigned i = 0; i < 30 && !env.IsCancelled(); ++i) {
      env.SetProgressPosition(i);
      env.Sleep(500);
    }
  }
};

static void
Main()
{
  TestJob job;
  JobDialog(main_window, *dialog_look, _T("RunJobDialog"), job);
}
