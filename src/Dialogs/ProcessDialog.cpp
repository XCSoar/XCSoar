// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ProcessDialog.hpp"
#include "WidgetDialog.hpp"
#include "Widget/LargeTextWidget.hpp"
#include "ui/event/poll/Queue.hpp"
#include "ui/event/Globals.hpp"
#include "ui/event/KeyCode.hpp"
#include "ui/event/Timer.hpp"
#include "Language/Language.hpp"
#include "event/PipeEvent.hxx"
#include "io/Open.hxx"
#include "io/UniqueFileDescriptor.hxx"
#include "system/Error.hxx"
#include "util/CharUtil.hxx"
#include "util/Exception.hxx"
#include "util/PrintException.hxx"
#include "util/UTF8.hpp"

#include <chrono>
#include <errno.h>
#include <signal.h>
#include <span>
#include <string.h>
#include <sys/wait.h>

class ProcessWidget final : public LargeTextWidget {
  const char *const*const argv;
  const std::function<int(int)> on_exit;

  const char *const progress_text;

  pid_t pid = 0;
  pid_t process_group = 0;

  bool cancelling = false;
  std::chrono::steady_clock::time_point cancel_deadline;
  UI::Timer wait_timer{[this](){ OnWaitTimer(); }};

  PipeEvent fd;

  Button *cancel_button;

  std::string text;
  std::string pending_text;

  WndForm *dialog;

public:
  ProcessWidget(EventLoop &event_loop, const DialogLook &_look,
                const char *const*_argv,
                std::function<int(int)> _on_exit,
                const char *_summary = nullptr) noexcept
    :LargeTextWidget(_look),
     argv(_argv),
     on_exit(std::move(_on_exit)),
     progress_text(_summary),
     fd(event_loop, BIND_THIS_METHOD(OnPipeReady)) {}

  void CreateButtons(WidgetDialog &_dialog) noexcept {
    dialog = &_dialog;

    cancel_button = _dialog.AddButton(_("Cancel"),
                                      [this](){ BeginCancel(); });
  }

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;
  bool KeyPress(unsigned key_code) noexcept override;

private:
  void Start();
  void BeginCancel() noexcept;
  void CancelImmediately() noexcept;
  void SignalProcess(int signal) noexcept;
  void OnWaitTimer() noexcept;
  void UpdateText() noexcept;
  void AppendOutput(std::span<const char> src);
  void OnExit(int code) noexcept;
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
    setpgid(0, 0);

    dev_null.CheckDuplicate(FileDescriptor{STDIN_FILENO});
    w.CheckDuplicate(FileDescriptor{STDOUT_FILENO});
    w.CheckDuplicate(FileDescriptor{STDERR_FILENO});

    execv(argv[0], const_cast<char **>(argv));
    fprintf(stderr, "Failed to execute %s: %s\n", argv[0], strerror(errno));
    _exit(EXIT_FAILURE);
  }

  setpgid(pid, pid);
  process_group = pid;

  fd.Open(r.Release());
  fd.ScheduleRead();
}

void
ProcessWidget::SignalProcess(int signal) noexcept
{
  if (process_group > 0 && kill(-process_group, signal) == 0)
    return;

  if (pid > 0)
    kill(pid, signal);
}

void
ProcessWidget::BeginCancel() noexcept
{
  if (cancelling)
    return;

  if (pid <= 0 && process_group <= 0) {
    dialog->SetModalResult(mrCancel);
    return;
  }

  cancelling = true;
  // Give helpers enough time to leave hardware and persistent state clean.
  cancel_deadline = std::chrono::steady_clock::now() +
    std::chrono::seconds{10};
  cancel_button->SetCaption(_("Stopping..."));
  cancel_button->SetEnabled(false);
  SignalProcess(SIGTERM);
  wait_timer.Schedule(std::chrono::milliseconds{100});
}

void
ProcessWidget::CancelImmediately() noexcept
{
  wait_timer.Cancel();
  fd.Close();
  SignalProcess(SIGKILL);
  process_group = 0;

  if (pid > 0) {
    UI::event_queue->ReapChild(pid);
    pid = 0;
  }
}

void
ProcessWidget::OnWaitTimer() noexcept
{
  if (pid > 0) {
    int status;
    const pid_t result = waitpid(pid, &status, WNOHANG);
    if (result == pid || (result < 0 && errno != EINTR)) {
      pid = 0;
      if (!cancelling) {
        fd.Close();
        process_group = 0;
        OnExit(result > 0 && WIFEXITED(status)
               ? WEXITSTATUS(status) : EXIT_FAILURE);
        return;
      }
    }
  }

  if (cancelling) {
    const bool cancel_expired =
      std::chrono::steady_clock::now() >= cancel_deadline;

    // Reaping the script does not mean its descendants have stopped.
    if (process_group > 0 &&
        kill(-process_group, 0) < 0 && errno == ESRCH)
      process_group = 0;

    // A stuck child or inherited pipe must not keep the dialog open.
    if (cancel_expired) {
      CancelImmediately();
      OnExit(EXIT_FAILURE);
      return;
    }

    // Drain cleanup output before completing cancellation.
    if (pid <= 0 && process_group <= 0 && !fd.IsDefined()) {
      OnExit(EXIT_FAILURE);
      return;
    }
  }

  if (pid > 0 || (cancelling && (process_group > 0 || fd.IsDefined())))
    wait_timer.Schedule(std::chrono::milliseconds{100});
}

void
ProcessWidget::UpdateText() noexcept
{
  SetText(text.empty() ? progress_text : text.c_str());
  UI::event_queue->Interrupt();
}

void
ProcessWidget::AppendOutput(std::span<const char> src)
{
  static constexpr std::size_t MAX_OUTPUT_CHARACTERS = 4096;

  std::string input = std::move(pending_text);
  pending_text.clear();
  input.append(src.data(), src.size());

  for (std::size_t i = 0; i < input.size();) {
    const std::size_t sequence = SequenceLengthUTF8(input[i]);
    if (sequence > input.size() - i) {
      pending_text.assign(input, i, std::string::npos);
      break;
    }

    const std::string_view character{input.data() + i, sequence};
    if (sequence == 0 || !ValidateUTF8(character)) {
      text.push_back('?');
      ++i;
    } else {
      if (sequence != 1 || IsTextByte(input[i]))
        text.append(character);
      i += sequence;
    }
  }

  const auto suffix = SuffixUTF8(text, MAX_OUTPUT_CHARACTERS);
  if (suffix.size() < text.size())
    text.erase(0, static_cast<std::size_t>(suffix.data() - text.data()));
}

void
ProcessWidget::OnExit(int code) noexcept
{
  wait_timer.Cancel();
  int result = mrCancel;
  if (!cancelling) {
    if (progress_text != nullptr) {
      if (!text.empty())
        text.append(text.back() == '\n' ? "\n" : "\n\n");
      text.append(code == EXIT_SUCCESS ? _("Complete") : _("Failed"));
    }

    result = on_exit ? on_exit(code) : 0;
    if (result == 0)
      cancel_button->SetCaption(_("Close"));
  }

  if (result != 0)
    dialog->SetModalResult(result);
  UpdateText();
}

inline void
ProcessWidget::OnPipeReady(unsigned) noexcept
{
  char buffer[4096];
  const ssize_t nbytes = fd.GetFileDescriptor().Read(
    std::as_writable_bytes(std::span{buffer}));
  if (nbytes < 0 && (errno == EINTR || errno == EAGAIN))
    return;

  if (nbytes <= 0) {
    fd.Close();

    if (!pending_text.empty()) {
      text.push_back('?');
      pending_text.clear();
    }

    if (nbytes < 0)
      text.append("\nFailed to read from pipe");

    UpdateText();
    OnWaitTimer();
    return;
  }

  AppendOutput({buffer, static_cast<std::size_t>(nbytes)});

  UpdateText();
}

void
ProcessWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  LargeTextWidget::Prepare(parent, rc);

  if (progress_text != nullptr)
    SetText(progress_text);

  try {
    Start();
  } catch (...) {
    text = GetFullMessage(std::current_exception());
    OnExit(EXIT_FAILURE);
  }
}

void
ProcessWidget::Unprepare() noexcept
{
  CancelImmediately();

  LargeTextWidget::Unprepare();
}

bool
ProcessWidget::KeyPress(unsigned key_code) noexcept
{
  if (key_code == KEY_ESCAPE && (pid > 0 || cancelling)) {
    BeginCancel();
    return true;
  }

  return LargeTextWidget::KeyPress(key_code);
}

int
RunProcessDialog(UI::SingleWindow &parent,
                 const DialogLook &dialog_look,
                 const char *caption,
                 const char *const*argv,
                 std::function<int(int)> on_exit,
                 const char *progress_text) noexcept
{
  TWidgetDialog<ProcessWidget> dialog(WidgetDialog::Full{},
                                      parent, dialog_look,
                                      caption);
  dialog.SetWidget(UI::event_queue->GetEventLoop(), dialog_look,
                   argv, std::move(on_exit), progress_text);
  dialog.GetWidget().CreateButtons(dialog);
  dialog.EnableCursorSelection();
  return dialog.ShowModal();
}
