// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LinuxStorageHotplugMonitor.hpp"
#include "LogFile.hpp"
#include "ui/event/poll/Queue.hpp"
#include "Storage/StorageHotplugMonitor.hpp"
#include "util/BindMethod.hxx"

#include <sys/socket.h>
#include <linux/netlink.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <string_view>
#include <exception>

LinuxStorageHotplugMonitor::LinuxStorageHotplugMonitor(
    StorageHotplugHandler &handler)
  : handler_(handler),
    trigger_event_(UI::event_queue->GetEventLoop(),
                   BIND_THIS_METHOD(OnTriggerReady))
{}

LinuxStorageHotplugMonitor::~LinuxStorageHotplugMonitor()
{
  Stop();
}

void
LinuxStorageHotplugMonitor::Start() noexcept
{
  if (running_)
    return;

  SetupSocket();
  if (socket_fd_ < 0)
    return;

  /* register our trigger EventFD with the main event loop so the
     UI thread gets a callback when the worker thread calls
     trigger_.Write() */
  trigger_event_.Open(trigger_.Get());
  trigger_event_.ScheduleRead();

  running_ = true;
  Thread::Start();
}

void
LinuxStorageHotplugMonitor::Stop() noexcept
{
  if (!running_)
    return;

  running_ = false;

  /* wake the worker thread so it exits promptly instead of
     waiting for the poll() timeout */
  shutdown_fd_.Write();

  Thread::Join();

  /* unregister the trigger from the event loop */
  trigger_event_.Close();

  CloseSocket();
}

void
LinuxStorageHotplugMonitor::SetupSocket() noexcept
{
  socket_fd_ = ::socket(AF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
  if (socket_fd_ < 0)
    return;

  sockaddr_nl addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.nl_family = AF_NETLINK;
  addr.nl_pid = 0; // let kernel assign unique port
  addr.nl_groups = 1; // kernel uevent multicast group

  if (::bind(socket_fd_, reinterpret_cast<sockaddr *>(&addr),
             sizeof(addr)) < 0) {
    ::close(socket_fd_);
    socket_fd_ = -1;
  }
}

void
LinuxStorageHotplugMonitor::CloseSocket() noexcept
{
  if (socket_fd_ >= 0) {
    ::close(socket_fd_);
    socket_fd_ = -1;
  }
}

void
LinuxStorageHotplugMonitor::Run() noexcept
{
  char buffer[4096];
  while (running_) {
    struct pollfd pfds[2];
    pfds[0].fd = socket_fd_;
    pfds[0].events = POLLIN;
    pfds[0].revents = 0;
    pfds[1].fd = shutdown_fd_.Get().Get();
    pfds[1].events = POLLIN;
    pfds[1].revents = 0;

    int rc = ::poll(pfds, 2, -1); /* block until event or shutdown */
    if (rc <= 0)
      continue;

    /* shutdown requested? */
    if (pfds[1].revents & POLLIN)
      break;

    if (pfds[0].revents & POLLIN) {
      ssize_t len = ::recv(socket_fd_, buffer, sizeof(buffer), 0);
      if (len <= 0)
        continue;

      std::string_view msg(buffer, static_cast<size_t>(len));
      if (msg.find("SUBSYSTEM=block") != std::string_view::npos) {
        trigger_.Write();
      }
    }
  }
}

void
LinuxStorageHotplugMonitor::OnTriggerReady(unsigned) noexcept
try
{
  /* Clear the EventFD and notify the handler on the UI thread.
     This callback runs in the main/UI event loop thread. */
  (void)trigger_.Read();
  handler_.OnStorageTopologyChanged();

  /* Ensure we remain registered for future events. Some backends
     require explicit re-arming inside the callback; ScheduleRead
     is idempotent if already scheduled. */
  trigger_event_.ScheduleRead();
}
catch (...) {
  LogError(std::current_exception());
}
