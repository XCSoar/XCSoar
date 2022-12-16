cmake_minimum_required(VERSION 3.15)

set(INCLUDE_WITH_TOOLCHAIN 0)  # special include path for every toolchain!

prepare_3rdparty(curl curl)
if (_COMPLETE_INSTALL)

    set(CMAKE_ARGS
    #         "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
             "-DCMAKE_INSTALL_PREFIX=${_INSTALL_DIR}"
             "-DCMAKE_INSTALL_BINDIR=${_INSTALL_BIN}"
             "-DCMAKE_INSTALL_LIBDIR=${_INSTALL_LIB}"
             # "-DCMAKE_INSTALL_COMPONENT=bin/${TOOLCHAIN}"
             "-DCMAKE_INSTALL_INCLUDEDIR=include"
             "-DCMAKE_BUILD_TYPE=Release"

            "-DBUILD_CURL_EXE=OFF"
            "-DBUILD_SHARED_LIBS=OFF"
            "-DENABLE_ARES=ON"
            "-DCURL_DISABLE_LDAP=ON"
            "-DCURL_DISABLE_TELNET=ON"
            "-DCURL_DISABLE_DICT=ON"
            "-DCURL_DISABLE_FILE=ON"
            "-DCURL_DISABLE_TFTP=ON"
            "-DCURL_DISABLE_LDAPS=ON"
            "-DCURL_DISABLE_RTSP=ON"
            "-DCURL_DISABLE_PROXY=ON"
            "-DCURL_DISABLE_POP3=ON"
            "-DCURL_DISABLE_IMAP=ON"
            "-DCURL_DISABLE_SMTP=ON"
            "-DCURL_DISABLE_GOPHER=ON"
            "-DCURL_DISABLE_COOKIES=ON"
            "-DCURL_DISABLE_CRYPTO_AUTH=ON"
            "-DCURL_DISABLE_IMAP=ON"
            "-DCMAKE_USE_LIBSSH2=OFF"
            "-DBUILD_TESTING=OFF"
            "-DPERL_EXECUTABLE=${PERL_APP}"     #### "D:/Programs/Perl64/bin/perl.exe"  # Windows only!!

             "-DZLIB_INCLUDE_DIR=${ZLIB_INCLUDE_DIR}"
             "-DZLIB_LIBRARY=${ZLIB_LIB}"
             # "-DZLIB_LIBRARY_DEBUG=${ZLIB_LIB}"
             # "-DZLIB_LIBRARY_RELEASE=${ZLIB_LIB}"
            "-DCARES_LIBRARY=${CARES_LIB}"
            # "-DCARES_LIBRARY_RELEASE=${CARES_LIB}"
            # "-DCARES_LIBRARY_DEBUG=${CARES_LIB}"
            "-DCARES_INCLUDE_DIR=${CARES_INCLUDE_DIR}"

            "-DHAVE_IOCTLSOCKET_FIONBIO=1"
        
            "-DLIBCURL_OUTPUT_NAME=${LIB_PREFIX}curl"
       )

       if(WIN32)
            list(APPEND CMAKE_ARGS  # Windows Only
                "-DCURL_USE_SCHANNEL=ON"
            )
       else()
            list(APPEND CMAKE_ARGS    # for Linux/Android,...?
                "-DLIB_EAY_DEBUG=${_CRYPTO_LIB}"
                "-DLIB_EAY_RELEASE=${_CRYPTO_LIB}"
                "-DOPENSSL_INCLUDE_DIR=${_SSL_DIR}"
            )
       endif()
    string(REPLACE "." "_" GIT_TAG ${XCSOAR_${TARGET_CNAME}_VERSION})
    ExternalProject_Add(
        ${_BUILD_TARGET}
        GIT_REPOSITORY "https://github.com/curl/curl.git"
        GIT_TAG  ${GIT_TAG}
        PREFIX  "${CURL_PREFIX}"
        ${_BINARY_STEP}
         # BINARY_DIR    "${${TARGET_CNAME}_PREFIX}/build/${TOOLCHAIN}"
        INSTALL_DIR "${_INSTALL_DIR}"  # ${LINK_LIBS}/${LIB_TARGET_NAME}/${XCSOAR_${TARGET_CNAME}_VERSION}"
        CMAKE_ARGS ${CMAKE_ARGS}
         # PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/CURL_CMakeLists.txt.in" <SOURCE_DIR>/CMakeLists.txt
        # INSTALL_COMMAND   cmake --build . --target install --config Release
        ${_INSTALL_COMMAND}
        BUILD_ALWAYS ${EP_BUILD_ALWAYS}
        # BUILD_IN_SOURCE ${EP_BUILD_IN_SOURCE}
        DEPENDS  ${ZLIB_TARGET} ${CARES_TARGET}
        BUILD_BYPRODUCTS  ${_TARGET_LIBS} # ${${TARGET_CNAME}_LIB}
    )

endif()

post_3rdparty()
