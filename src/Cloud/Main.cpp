// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CloudGlue.hpp"
#include "CloudPolicy.hpp"
#include "Data.hpp"
#include "Dump.hpp"
#include "Serialiser.hpp"
#include "Tracking/SkyLines/Server.hpp"
#include "event/Loop.hxx"
#include "event/CoarseTimerEvent.hxx"
#include "event/SignalMonitor.hxx"
#include "net/IPv4Address.hxx"
#include "io/FileOutputStream.hxx"
#include "io/FileReader.hxx"
#include "util/PrintException.hxx"
#include "util/Exception.hxx"
#include "util/Compiler.h"
#include "util/ScopeExit.hxx"

#include <iostream>

#include <signal.h>

using std::cout;
using std::cerr;
using std::endl;

class CloudServer final : public SkyLinesTracking::Server {
  const AllocatedPath db_path;

  CloudData data;
  CloudGlue glue;

  CoarseTimerEvent save_timer, expire_timer;

public:
  CloudServer(AllocatedPath &&_db_path, EventLoop &event_loop,
              SocketAddress bind_address)
    :SkyLinesTracking::Server(event_loop, bind_address),
     db_path(std::move(_db_path)),
     glue(data, cloud_policy, *this),
     save_timer(event_loop, BIND_THIS_METHOD(OnSaveTimer)),
     expire_timer(event_loop, BIND_THIS_METHOD(OnExpireTimer))
  {
#ifndef _WIN32
    SignalMonitorRegister(SIGINT, BIND_THIS_METHOD(OnQuitSignal));
    SignalMonitorRegister(SIGTERM, BIND_THIS_METHOD(OnQuitSignal));
    SignalMonitorRegister(SIGQUIT, BIND_THIS_METHOD(OnQuitSignal));

    SignalMonitorRegister(SIGHUP, BIND_THIS_METHOD(OnReloadSignal));
    SignalMonitorRegister(SIGUSR1, BIND_THIS_METHOD(OnDumpSignal));
#endif

    ScheduleSave();
  }

  void Load();
  void Save();

  void DumpClients() { data.DumpClients(); }

private:
  void OnSaveTimer() noexcept {
    Save();
    ScheduleSave();
  }

  void ScheduleSave() {
    save_timer.Schedule(cloud_policy.save_interval);
  }

  void OnExpireTimer() noexcept {
    data.clients.Expire(GetEventLoop().SteadyNow() -
                        cloud_policy.client_expire_cutoff);
    if (!data.clients.empty())
      ScheduleExpire();
  }

  void ScheduleExpire() {
    expire_timer.Schedule(cloud_policy.expire_timer_interval);
  }

protected:
  void OnFix(const Client &client,
             std::chrono::milliseconds time_of_day,
             const ::GeoPoint &location, int altitude) override;

  void OnTrafficRequest(const Client &client, bool near) override;

  void OnWaveSubmit(const Client &client,
                    std::chrono::milliseconds time_of_day,
                    const ::GeoPoint &a, const ::GeoPoint &b,
                    int bottom_altitude,
                    int top_altitude,
                    double lift) override;

  void OnThermalSubmit(const Client &client,
                       std::chrono::milliseconds time_of_day,
                       const ::GeoPoint &bottom_location,
                       int bottom_altitude,
                       const ::GeoPoint &top_location,
                       int top_altitude,
                       double lift) override;

  void OnThermalRequest(const Client &client) override;

  void OnSendError(SocketAddress address,
                   std::exception_ptr e) noexcept override {
    cerr << "Failed to send to " << address
         << ": " << GetFullMessage(e)
         << endl;
  }

  void OnError(std::exception_ptr e) override {
    cerr << GetFullMessage(e) << endl;
    GetEventLoop().Break();
  }

#ifndef _WIN32
  void OnQuitSignal() noexcept {
    GetEventLoop().Break();
  }

  void OnReloadSignal() noexcept {
    Save();
  }

  void OnDumpSignal() noexcept {
    DumpClients();
  }
#endif
};

void
CloudServer::OnFix(const Client &client,
                   std::chrono::milliseconds time_of_day,
                   const ::GeoPoint &location, int altitude)
{
  bool schedule_expire = false;
  glue.OnFix(client, time_of_day, location, altitude,
             GetEventLoop().SteadyNow(), schedule_expire);
  if (schedule_expire)
    ScheduleExpire();
}

void
CloudServer::OnTrafficRequest(const Client &client, bool near)
{
  glue.OnTrafficRequest(client, near, GetEventLoop().SteadyNow());
}

void
CloudServer::OnWaveSubmit(const Client &client,
                          std::chrono::milliseconds time_of_day,
                          const ::GeoPoint &a, const ::GeoPoint &b,
                          int bottom_altitude,
                          int top_altitude,
                          double lift)
{
  glue.OnWaveSubmit(client, time_of_day, a, b,
                    bottom_altitude, top_altitude, lift);
}

void
CloudServer::OnThermalSubmit(const Client &client,
                             std::chrono::milliseconds time_of_day,
                             const ::GeoPoint &bottom_location,
                             int bottom_altitude,
                             const ::GeoPoint &top_location,
                             int top_altitude,
                             double lift)
{
  glue.OnThermalSubmit(client, time_of_day,
                       bottom_location, bottom_altitude,
                       top_location, top_altitude, lift,
                       GetEventLoop().SteadyNow());
}

void
CloudServer::OnThermalRequest(const Client &client)
{
  glue.OnThermalRequest(client, GetEventLoop().SteadyNow());
}

void
CloudServer::Load()
{
  FileReader fr(db_path);
  Deserialiser s(fr);
  data.Load(s);
}

void
CloudServer::Save()
{
  cout << "Saving data to " << db_path.c_str() << endl;

  FileOutputStream fos(db_path);

  {
    Serialiser s(fos);
    data.Save(s);
    s.Flush();
  }

  fos.Commit();
}

int
main(int argc, char **argv)
try {
  if (argc != 2) {
    cerr << "Usage: " << argv[0] << " DBPATH" << endl;
    return EXIT_FAILURE;
  }

  const Path db_path(argv[1]);

  EventLoop event_loop;
  SignalMonitorInit(event_loop);
  AtScopeExit() { SignalMonitorFinish(); };

  CloudServer server(db_path, event_loop,
                     IPv4Address(CloudServer::GetDefaultPort()));

  try {
    server.Load();
  } catch (const std::runtime_error &e) {
    cerr << "Failed to load database" << endl;
    PrintException(e);
  }

  event_loop.Run();

  server.Save();

  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
