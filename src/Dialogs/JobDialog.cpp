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

#include "JobDialog.hpp"
#include "ProgressDialog.hpp"
#include "Form/Form.hpp"
#include "Job/Thread.hpp"

class DialogJobThread : public JobThread {
  WndForm &form;

public:
  DialogJobThread(OperationEnvironment &_env, Job &_job, WndForm &_form)
    :JobThread(_env, _job), form(_form) {
    form.SetForceOpen(true);
  }

protected:
  virtual void OnComplete() {
    form.SetModalResult(mrOK);
    form.SetForceOpen(false);
  }
};

bool
JobDialog(SingleWindow &parent, const DialogLook &dialog_look,
          const TCHAR *caption,
          Job &job, bool cancellable)
{
  ProgressDialog form(parent, dialog_look, caption);

  DialogJobThread thread(form, job, form);
  thread.Start();

  if (cancellable)
    form.AddCancelButton([&thread](){ thread.Cancel(); });

  int result = form.ShowModal();

  thread.Cancel();
  thread.Join();

  return result == mrOK;
}

bool
DialogJobRunner::Run(Job &job)
{
  return JobDialog(parent, dialog_look, caption, job, cancellable);
}
