// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CoDialog.hpp"
#include "ProgressDialog.hpp"
#include "Operation/PluggableOperationEnvironment.hpp"
#include "Operation/ThreadedOperationEnvironment.hpp"
#include "ui/event/Notify.hpp"
#include "co/InjectTask.hxx"
#include "io/async/AsioThread.hpp"
#include "io/async/GlobalAsioThread.hpp"

using namespace UI;

namespace {

class CoDialog {
  ProgressDialog dialog;
  ThreadedOperationEnvironment env{dialog};

  Co::InjectTask inject_task;
  std::exception_ptr error;

  UI::Notify notify{[this](){
    dialog.SetModalResult(mrOK);
    dialog.SetForceOpen(false);
  }};

public:
  CoDialog(UI::SingleWindow &parent, const DialogLook &dialog_look,
           const char *caption,
           PluggableOperationEnvironment *_env) noexcept
    :dialog(parent, dialog_look, caption),
     inject_task(asio_thread->GetEventLoop())
  {
    dialog.AddCancelButton();

    if (_env != nullptr)
      _env->SetOperationEnvironment(env);
  }

  void Start(Co::InvokeTask &&task) noexcept {
    inject_task.Start(std::move(task), BIND_THIS_METHOD(OnCompletion));
  }

  bool Show() {
    if (dialog.ShowModal() != mrOK)
      return false;

    /* cancel the coroutine (if it is still running) */
    inject_task.Cancel();

    if (error)
      std::rethrow_exception(std::move(error));

    return true;
  }

private:
  void OnCompletion(std::exception_ptr _error) noexcept {
    error = std::move(_error);

    /* close the dialog in the main thread */
    notify.SendNotification();
  }
};

} // anonymous namespace

bool
ShowCoDialog(UI::SingleWindow &parent, const DialogLook &dialog_look,
             const char *caption, Co::InvokeTask task,
             PluggableOperationEnvironment *env)
{
  CoDialog dialog{parent, dialog_look, caption, env};
  dialog.Start(std::move(task));
  return dialog.Show();
}
