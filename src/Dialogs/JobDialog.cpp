/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Look/DialogLook.hpp"
#include "Form/Form.hpp"
#include "ProgressWindow.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/ButtonWindow.hpp"
#include "Screen/Layout.hpp"
#include "Operation/Operation.hpp"
#include "Job/Thread.hpp"
#include "Language/Language.hpp"

class ProgressWindowOperation
  : public ProgressWindow, public QuietOperationEnvironment {
public:
  ProgressWindowOperation(ContainerWindow &parent)
    :ProgressWindow(parent) {}

  virtual void SetText(const TCHAR *text) {
    SetMessage(text);
  }

  virtual void SetProgressRange(unsigned range) {
    SetRange(0, range);
  }

  virtual void SetProgressPosition(unsigned position) {
    SetValue(position);
  }
};

class JobCancelButton : public ButtonWindow {
  JobThread &thread;

public:
  JobCancelButton(JobThread &_thread)
    :thread(_thread) {}

  virtual bool OnClicked() {
    thread.Cancel();
    return true;
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

bool
JobDialog(SingleWindow &parent, const DialogLook &dialog_look,
          const TCHAR *caption,
          Job &job, bool cancellable)
{
  WindowStyle form_style;
  form_style.Hide();
  WndForm form(parent, dialog_look,
               parent.GetClientRect(),
               caption);

  ContainerWindow &client_area = form.GetClientAreaWindow();

  ProgressWindowOperation progress(client_area);
  DialogJobThread thread(progress, job, form);
  thread.Start();

  JobCancelButton cancel_button(thread);
  if (cancellable) {
    ButtonWindowStyle style;
    style.TabStop();

    PixelRect rc = client_area.GetClientRect();
    rc.right -= Layout::Scale(2);
    rc.left = rc.right - Layout::Scale(78);
    rc.top += Layout::Scale(2);
    rc.bottom = rc.top + Layout::Scale(35);
    cancel_button.Create(client_area, _("Cancel"), rc, style);
    cancel_button.SetFont(*dialog_look.button.font);
    cancel_button.BringToTop();
  }

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
