// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "event/CoarseTimerEvent.hxx"
#include "event/SocketEvent.hxx"
#include "event/net/ConnectSocket.hxx"
#include "event/net/cares/SimpleResolver.hxx"
#include "net/AllocatedSocketAddress.hxx"
#include "net/UniqueSocketDescriptor.hxx"

#include <forward_list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

class EventLoop;
namespace Cares {
class Channel;
}

class OGNAprsHandler {
public:
  virtual void OnAprsLine(std::string_view line) noexcept = 0;
};

/**
 * Maintains a TCP connection to an APRS-IS server (OGN / glidernet full feed),
 * forwarding text lines to #OGNAprsHandler.
 */
class OGNClient final : ConnectSocketHandler {
  class ResolverHandler final : public Cares::SimpleHandler {
    OGNClient &parent;

  public:
    explicit ResolverHandler(OGNClient &_parent) noexcept
      :parent(_parent) {}

    void OnResolverSuccess(
        std::forward_list<AllocatedSocketAddress> addresses) noexcept override {
      parent.resolver_job.reset();
      parent.TryConnect(std::move(addresses));
    }

    void OnResolverError([[maybe_unused]] std::exception_ptr error) noexcept override {
      parent.resolver_job.reset();
      parent.ScheduleReconnect();
    }
  };

  friend class ResolverHandler;

  EventLoop &loop;
  Cares::Channel &cares;
  OGNAprsHandler &handler;

  const std::string host;
  const unsigned port;
  const std::string user;
  const std::string pass;

  ResolverHandler resolver_handler;
  std::optional<Cares::SimpleResolver> resolver_job;

  ConnectSocket connector;
  SocketEvent read_event;

  /**
   * Did APRS-IS verify the login on this connection?  It drops the
   * packets of an unverified login without telling.
   */
  bool login_verified = false;
  CoarseTimerEvent reconnect_timer;

  std::string rx_buffer;

public:
  OGNClient(EventLoop &_loop, Cares::Channel &_cares,
            OGNAprsHandler &_handler,
            std::string &&_host, unsigned _port,
            std::string &&_user, std::string &&_pass) noexcept;

  void Start() noexcept;
  void Stop() noexcept;

  /**
   * May lines be sent?  Only a login which APRS-IS verified may send;
   * it drops the packets of "N0CALL" and of a wrong passcode.
   */
  [[gnu::pure]]
  bool CanSend() const noexcept;

  /**
   * Send one APRS line (without line end).
   *
   * @return false if not connected or the line could not be written
   * at once; it is then lost
   */
  bool SendLine(std::string_view line) noexcept;

private:
  /* ConnectSocketHandler */
  void OnSocketConnectSuccess(UniqueSocketDescriptor fd) noexcept override;
  void OnSocketConnectError(std::exception_ptr error) noexcept override;

  void OnReconnectTimer() noexcept;
  void OnReadReady(unsigned events) noexcept;

  void BeginLookup() noexcept;
  void TryConnect(std::forward_list<AllocatedSocketAddress> addresses) noexcept;
  void SendLogin() noexcept;
  void CloseConnection() noexcept;
  void ScheduleReconnect() noexcept;
  void CheckLoginResponse(std::string_view line) noexcept;
  void ConsumeInput(std::string_view chunk);
};
