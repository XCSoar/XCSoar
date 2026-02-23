// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UdevContext.hpp"
#include "thread/Mutex.hxx"

#include <libudev.h>

#include <cassert>

static UdevContext *udev_root_context = nullptr;
static Mutex udev_context_mutex;

UdevContext::UdevContext(const UdevContext &other) noexcept
{
  const std::lock_guard lock{udev_context_mutex};
  if (other.ud) {
    ud = udev_ref(other.ud);
    assert(ud);
  } else {
    ud = nullptr;
  }
}

UdevContext::~UdevContext() noexcept
{
  const std::lock_guard lock{udev_context_mutex};
  if (nullptr != ud)
    udev_unref(ud);
}

UdevContext &
UdevContext::operator=(const UdevContext &other) noexcept
{
  if (this != &other) {
    const std::lock_guard lock{udev_context_mutex};
    struct udev *new_ud = other.ud ? udev_ref(other.ud) : nullptr;
    assert(!other.ud || new_ud);
    if (ud != nullptr)
      udev_unref(ud);
    ud = new_ud;
  }

  return *this;
}

UdevContext
UdevContext::NewRef() noexcept
{
  {
    const std::lock_guard lock{udev_context_mutex};
    if (nullptr == udev_root_context) {
      udev_root_context = new UdevContext(udev_new());
      assert(udev_root_context);
    }
  }

  return UdevContext(*udev_root_context);
}
