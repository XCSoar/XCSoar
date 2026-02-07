// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_DIALOG
#define ENABLE_MAIN_WINDOW

#include "Main.hpp"
#include "Dialogs/JobDialog.hpp"
#include "Job/Job.hpp"
#include "Operation/Operation.hpp"

class TestJob : public Job {
public:
  virtual void Run(OperationEnvironment &env) {
    env.SetText("Working...");
    env.SetProgressRange(30);
    for (unsigned i = 0; i < 30 && !env.IsCancelled(); ++i) {
      env.SetProgressPosition(i);
      env.Sleep(std::chrono::milliseconds(500));
    }
  }
};

static void
Main(TestMainWindow &main_window)
{
  TestJob job;
  JobDialog(main_window, *dialog_look, "RunJobDialog", job);
}
