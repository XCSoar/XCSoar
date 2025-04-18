// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "JobDialog.hpp"
#include "ProgressDialog.hpp"
#include "Form/Form.hpp"
#include "Job/Thread.hpp"

using namespace UI;

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

  if (thread.IsCancelled() || form.IsCancelled())
    result = mrCancel;

  thread.Cancel();
  thread.Join();

  return result == mrOK;
}

bool
DialogJobRunner::Run(Job &job)
{
  return JobDialog(parent, dialog_look, caption, job, cancellable);
}
