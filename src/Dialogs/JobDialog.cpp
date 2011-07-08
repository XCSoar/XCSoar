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

#include "Dialogs/JobDialog.hpp"
#include "Form/Form.hpp"
#include "Screen/ProgressWindow.hpp"
#include "Screen/SingleWindow.hpp"
#include "Operation.hpp"
#include "Thread/JobThread.hpp"

class ProgressWindowOperation
  : public ProgressWindow, public QuietOperationEnvironment {
public:
  ProgressWindowOperation(ContainerWindow &parent)
    :ProgressWindow(parent) {}

  virtual void SetText(const TCHAR *text) {
    set_message(text);
  }

  virtual void SetProgressRange(unsigned range) {
    set_range(0, range);
  }

  virtual void SetProgressPosition(unsigned position) {
    set_pos(position);
  }
};

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

void
JobDialog(SingleWindow &parent, const TCHAR *caption,
          Job &job)
{
  WindowStyle form_style;
  form_style.hide();
  WndForm form(parent, 0, 0, parent.get_width(), parent.get_height(),
               caption);
  ProgressWindowOperation progress(form);
  DialogJobThread thread(progress, job, form);
  thread.Start();

  form.ShowModal();

  thread.Cancel();
  thread.Join();
}
