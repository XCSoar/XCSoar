set(_SOURCES
        util/ASCII.cxx
        util/ConvertString.cpp
        util/CRC.cpp
        util/EscapeBackslash.cpp
        util/Exception.cxx
        util/ExtractParameters.cpp
        util/PrintException.cxx
        util/StaticString.cxx
        util/StringBuilder.cxx
        util/StringCompare.cxx
        util/StringStrip.cxx
        util/StringUtil.cpp
        # util/StringView.cxx
        util/TruncateString.cpp
        util/tstring.cpp
        util/UTF8.cpp
        util/MD5.cpp  # new with 6.8.14
)
if(WIN32)
  list(APPEND _SOURCES
        util/WASCII.cxx
        util/WStringCompare.cxx
        util/WStringStrip.cxx
        util/WStringUtil.cpp
  )
endif()

set(SCRIPT_FILES
    CMakeSource.cmake
)



