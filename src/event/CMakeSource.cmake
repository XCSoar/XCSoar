set(_SOURCES
    Loop.cxx
    Call.cxx
    DeferEvent.cxx
    InjectEvent.cxx
    SocketEvent.cxx
    SignalMonitor.cxx
    TimerWheel.cxx
    TimerList.cxx
    CoarseTimerEvent.cxx

    net/ConnectSocket.cxx
    net/cares/Error.cxx
    net/cares/Init.cxx
    net/cares/Channel.cxx
    net/cares/SimpleResolver.cxx

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
