// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "Handler.hxx"
#include "net/AllocatedSocketAddress.hxx"
#include "util/Cancellable.hxx"

#include <exception>
#include <forward_list>

namespace Cares {

class Channel;

class SimpleHandler {
public:
  virtual void OnResolverSuccess(
      std::forward_list<AllocatedSocketAddress> addresses) noexcept = 0;
  virtual void OnResolverError(std::exception_ptr error) noexcept = 0;
};

class SimpleResolver final : Handler {
  std::forward_list<AllocatedSocketAddress> addresses;

  std::exception_ptr error;

  CancellablePointer cancel_ptr;

  SimpleHandler &handler;

  const unsigned port;

public:
  SimpleResolver(SimpleHandler &_handler, unsigned _port = 0) noexcept;

  ~SimpleResolver() noexcept
  {
    if (cancel_ptr) cancel_ptr.Cancel();
  }

  void Start(Channel &channel, const char *name) noexcept;

private:
  /* virtual methods from Cares::Handler */
  void OnCaresAddress(SocketAddress address) noexcept override
  {
    addresses.emplace_front(address);
    if (port != 0) addresses.front().SetPort(port);
  }

  void OnCaresSuccess() noexcept override
  {
    cancel_ptr = nullptr;
    addresses.reverse();
    handler.OnResolverSuccess(std::move(addresses));
  }

  void OnCaresError(std::exception_ptr e) noexcept override
  {
    cancel_ptr = nullptr;
    handler.OnResolverError(std::move(e));
  }
};

} // namespace Cares
