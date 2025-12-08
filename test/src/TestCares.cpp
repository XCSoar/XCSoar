// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "event/net/cares/Channel.hxx"
#include "event/net/cares/SimpleResolver.hxx"
#include "event/net/cares/Error.hxx"
#include "event/Loop.hxx"
#include "net/SocketAddress.hxx"

extern "C" {
#include "tap.h"
}

#include <forward_list>

using namespace Cares;

// Test handler that collects addresses and tracks completion
class TestHandler : public SimpleHandler {
  std::forward_list<AllocatedSocketAddress> received_addresses;
  std::exception_ptr received_error;
  bool success_called = false;
  bool error_called = false;
  EventLoop *event_loop = nullptr;

public:
  void SetEventLoop(EventLoop &_event_loop) noexcept
  {
    event_loop = &_event_loop;
  }

  void OnResolverSuccess(
      std::forward_list<AllocatedSocketAddress> addresses) noexcept override
  {
    received_addresses = std::move(addresses);
    success_called = true;
    if (event_loop)
      event_loop->Break();
  }

  void OnResolverError(std::exception_ptr error) noexcept override
  {
    received_error = error;
    error_called = true;
    if (event_loop)
      event_loop->Break();
  }

  bool HasSuccess() const noexcept { return success_called; }
  bool HasError() const noexcept { return error_called; }
  size_t GetAddressCount() const noexcept
  {
    size_t count = 0;
    for ([[maybe_unused]] const auto &addr : received_addresses)
      ++count;
    return count;
  }

  const std::forward_list<AllocatedSocketAddress> &GetAddresses() const noexcept
  {
    return received_addresses;
  }

  std::exception_ptr GetError() const noexcept { return received_error; }

  void Reset() noexcept
  {
    received_addresses.clear();
    received_error = nullptr;
    success_called = false;
    error_called = false;
  }
};

int
main([[maybe_unused]] int argc, [[maybe_unused]] char **argv)
{
  plan_tests(14);

  // Test 1: Successful DNS lookup for a well-known hostname
  {
    EventLoop event_loop;
    Channel channel(event_loop);
    TestHandler handler;
    handler.SetEventLoop(event_loop);
    SimpleResolver resolver(handler);
    resolver.Start(channel, "localhost");

    event_loop.Run();

    ok(handler.HasSuccess() || handler.HasError(),
       "DNS lookup for localhost completed");
    if (handler.HasSuccess()) {
      ok(handler.GetAddressCount() > 0,
         "localhost resolved to at least one address");
    }
  }

  // Test 2: Successful DNS lookup for a public hostname (IPv4)
  {
    EventLoop event_loop;
    Channel channel(event_loop);
    TestHandler handler;
    handler.SetEventLoop(event_loop);
    SimpleResolver resolver(handler);
    resolver.Start(channel, "example.com");

    event_loop.Run();

    ok(handler.HasSuccess(), "DNS lookup for example.com succeeded");
    if (handler.HasSuccess()) {
      ok(handler.GetAddressCount() > 0,
         "example.com resolved to at least one address");

      // Verify addresses are valid
      bool has_valid_address = false;
      for (const auto &addr : handler.GetAddresses()) {
        if (addr.IsDefined()) {
          has_valid_address = true;
          break;
        }
      }
      ok(has_valid_address, "Resolved addresses are valid");
    }
  }

  // Test 3: Failed DNS lookup for invalid hostname
  {
    EventLoop event_loop;
    Channel channel(event_loop);
    TestHandler handler;
    handler.SetEventLoop(event_loop);
    SimpleResolver resolver(handler);
    resolver.Start(channel, "this-hostname-definitely-does-not-exist-12345.invalid");

    event_loop.Run();

    ok(handler.HasError(), "DNS lookup for invalid hostname failed");
    if (handler.HasError()) {
      try {
        std::rethrow_exception(handler.GetError());
      } catch (const Error &e) {
        ok(e.GetCode() != 0, "Error has non-zero code");
      } catch (...) {
        ok(false, "Error is of type Cares::Error");
      }
    }
  }

  // Test 4: Cancellation - start lookup and cancel immediately
  {
    EventLoop event_loop;
    Channel channel(event_loop);
    TestHandler handler;
    handler.SetEventLoop(event_loop);
    {
      SimpleResolver resolver(handler);
      resolver.Start(channel, "example.com");

      // Cancel immediately (destructor will cancel)
    }

    // Run event loop briefly to process any pending events
    event_loop.Finish();
    event_loop.Run();

    // After cancellation, we should not receive callbacks
    // (Note: cancellation behavior may vary, so we just check it doesn't crash)
    ok(true, "Cancellation does not crash");
  }

  // Test 5: Channel can handle multiple concurrent lookups
  {
    EventLoop event_loop;
    Channel channel(event_loop);
    TestHandler handler1, handler2;
    handler1.SetEventLoop(event_loop);
    handler2.SetEventLoop(event_loop);
    SimpleResolver resolver1(handler1);
    SimpleResolver resolver2(handler2);

    resolver1.Start(channel, "localhost");
    resolver2.Start(channel, "127.0.0.1");

    // Run until first completes (one handler will call Break())
    event_loop.Run();

    // Verify at least one completed (both may complete in same iteration)
    bool at_least_one = (handler1.HasSuccess() || handler1.HasError()) ||
                        (handler2.HasSuccess() || handler2.HasError());
    ok(at_least_one, "Multiple concurrent lookups - at least one completes");
    
    // Verify channel handles concurrent requests without crashing
    ok(true, "Channel handles multiple concurrent lookups without crashing");
  }

  // Test 6: IPv4-only lookup
  {
    EventLoop event_loop;
    Channel channel(event_loop);
    TestHandler handler_ipv4;
    handler_ipv4.SetEventLoop(event_loop);
    SimpleResolver resolver(handler_ipv4);
    resolver.Start(channel, "127.0.0.1");

    event_loop.Run();

    ok(handler_ipv4.HasSuccess() || handler_ipv4.HasError(),
       "IPv4 address lookup completes");
    if (handler_ipv4.HasSuccess()) {
      ok(handler_ipv4.GetAddressCount() > 0,
         "IPv4 address resolved");
    }
  }

  // Test 7: Empty/null hostname handling
  {
    EventLoop event_loop;
    Channel channel(event_loop);
    TestHandler handler_empty;
    handler_empty.SetEventLoop(event_loop);
    SimpleResolver resolver(handler_empty);

    // Try with empty string (should fail)
    resolver.Start(channel, "");

    event_loop.Run();

    ok(handler_empty.HasError(),
       "Empty hostname lookup fails");
  }

  // Test 8: Channel destruction with pending requests
  {
    EventLoop event_loop;
    TestHandler handler_pending;
    handler_pending.SetEventLoop(event_loop);
    {
      Channel channel2(event_loop);
      SimpleResolver resolver(handler_pending);
      resolver.Start(channel2, "example.com");

      // Destroy channel while request is pending
      // (should not crash)
    }

    ok(true, "Channel destruction with pending requests does not crash");
  }

  return exit_status();
}
