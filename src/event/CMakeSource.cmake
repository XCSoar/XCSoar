set(_SOURCES
    Loop.cxx
    Call.cxx
    DeferEvent.cxx
    IdleEvent.cxx
    InjectEvent.cxx
    SocketEvent.cxx
    SignalMonitor.cxx
    TimerWheel.cxx
    TimerList.cxx
    CoarseTimerEvent.cxx
#   TimerEvent.cxx

    net/ConnectSocket.cxx
    net/cares/Error.cxx
    net/cares/Init.cxx
    net/cares/Channel.cxx
    net/cares/SimpleResolver.cxx
#    Backend.hxx
#    BackendEvents.hxx
#    Call.cxx
#    Call.hxx
#    Chrono.hxx
#    DeferEvent.cxx
#    DeferEvent.hxx
#    EpollBackend.hxx
#    EpollEvents.hxx
#    Features.h
#    IdleEvent.cxx
#    IdleEvent.hxx
#    InjectEvent.cxx
#    InjectEvent.hxx
#    Loop.cxx
#    Loop.hxx
#    SignalMonitor.cxx
#    SignalMonitor.hxx
#    SocketEvent.cxx
#    SocketEvent.hxx
#    TimerEvent.cxx
#    TimerEvent.hxx
#    WakeFD.hxx

    ../io/async/AsioThread.cpp
      ../io/async/GlobalAsioThread.cpp
)

if(UNIX)
  list(APPEND _SOURCES
    PollBackend.cxx
    PollBackend.hxx
    PollEvents.hxx
    PollResultGeneric.hxx
  )
elseif(WIN32)
  list(APPEND _SOURCES
    WinSelectBackend.cxx
    WinSelectBackend.hxx
    WinSelectEvents.hxx
  )
endif()

set(SCRIPT_FILES
    CMakeSource.cmake
)
