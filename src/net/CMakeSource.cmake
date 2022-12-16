set(_SOURCES
        net/AddressInfo.cxx
        net/HostParser.cxx
        net/Resolver.cxx
        net/SocketError.cxx
        net/State.cpp
        net/ToString.cxx
        net/IPv4Address.cxx
        net/IPv6Address.cxx
        net/StaticSocketAddress.cxx
        net/AllocatedSocketAddress.cxx
        net/SocketAddress.cxx
        net/SocketDescriptor.cxx
)
list(APPEND _SOURCES
        net/client/tim/Client.cpp
        net/client/tim/Glue.cpp
)
list(APPEND _SOURCES
        net/http/DownloadManager.cpp
        net/http/Progress.cpp
        ../lib/curl/OutputStreamHandler.cxx
        ../lib/curl/Adapter.cxx
        ../lib/curl/Setup.cxx
        ../lib/curl/Request.cxx
        ../lib/curl/CoRequest.cxx
        ../lib/curl/CoStreamRequest.cxx
        net/http/CoDownloadToFile.cpp
        ../lib/curl/Global.cxx
        net/http/Init.cpp
)

set (SCRIPT_FILES
    CMakeSource.cmake
)

