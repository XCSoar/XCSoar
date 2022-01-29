/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "ProcessDialog.hpp"
#include "WidgetDialog.hpp"
#include "Widget/LargeTextWidget.hpp"
#include "ui/event/poll/Queue.hpp"
#include "ui/event/Globals.hpp"
#include "Language/Language.hpp"
#include "event/PipeEvent.hxx"
#include "io/Open.hxx"
#include "io/UniqueFileDescriptor.hxx"
#include "system/Error.hxx"
#include "util/Exception.hxx"
#include "util/PrintException.hxx"

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

class ProcessWidget final : public LargeTextWidget {
  const char *const*const argv;

  pid_t pid = 0;

  PipeEvent fd;

  std::string text;

public:
  ProcessWidget(EventLoop &event_loop, const DialogLook &_look,
                const char *const*_argv) noexcept
    :LargeTextWidget(_look),
     argv(_argv),
     fd(event_loop, BIND_THIS_METHOD(OnPipeReady)) {}

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;

private:
  void Start();
  void Cancel() noexcept;
  void OnPipeReady(unsigned) noexcept;
};

static bool
UnblockAllSignals() noexcept
{
  sigset_t ss;
  sigemptyset(&ss);
  return sigprocmask(SIG_SETMASK, &ss, nullptr) == 0;
}

void
ProcessWidget::Start()
{
  auto dev_null = OpenReadOnly("/dev/null");

  UniqueFileDescriptor r, w;
  if (!UniqueFileDescriptor::CreatePipe(r, w))
    throw MakeErrno("Failed to create pipe");

  pid = fork();
  if (pid < 0)
    throw MakeErrno("Failed to fork");

  if (pid == 0) {
    UnblockAllSignals();

    dev_null.CheckDuplicate(FileDescriptor{STDIN_FILENO});
    w.CheckDuplicate(FileDescriptor{STDOUT_FILENO});
    w.CheckDuplicate(FileDescriptor{STDERR_FILENO});

    execv(argv[0], const_cast<char **>(argv));
    fprintf(stderr, "Failed to execute %s: %s\n", argv[0], strerror(errno));
    _exit(EXIT_FAILURE);
  }

  fd.Open(r.Release());
  fd.ScheduleRead();
}

void
ProcessWidget::Cancel() noexcept
{
  fd.Close();

  if (pid > 0) {
    kill(pid, SIGTERM);

    int status;
    waitpid(pid, &status, 0);
  }
}

inline void
ProcessWidget::OnPipeReady(unsigned) noexcept
{
  char buffer[4096];
  ssize_t nbytes = fd.GetFileDescriptor().Read(buffer, sizeof(buffer));
  if (nbytes < 0) {
    text.append("\nFailed to read from pipe");
    fd.Close();
    SetText(text.c_str());
    return;
  }

  if (nbytes == 0) {
    fd.Close();
    return;
  }

  text.append(buffer, nbytes);
  if (text.length() > 16384)
    text.erase(0, 4096);

  SetText(text.c_str());
}

void
ProcessWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  LargeTextWidget::Prepare(parent, rc);

  try {
    Start();
  } catch (...) {
    text = GetFullMessage(std::current_exception());
    SetText(text.c_str());
  }
}

void
ProcessWidget::Unprepare() noexcept
{
  Cancel();

  LargeTextWidget::Unprepare();
}

void
RunProcessDialog(UI::SingleWindow &parent,
                 const DialogLook &dialog_look,
                 const TCHAR *caption,
                 const char *const*argv) noexcept
{
  TWidgetDialog<ProcessWidget> dialog(WidgetDialog::Full{},
                                      parent, dialog_look,
                                      caption);
  dialog.AddButton(_("Close"), mrOK);
  dialog.SetWidget(UI::event_queue->GetEventLoop(), dialog_look,
                   argv);
  dialog.ShowModal();
}
