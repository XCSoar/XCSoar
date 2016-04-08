# Build rules for the HTTP client library

LIBNET_SOURCES = \
	$(SRC)/Net/State.cpp \
	$(SRC)/Net/IPv4Address.cxx \
	$(SRC)/Net/StaticSocketAddress.cxx \
	$(SRC)/Net/AllocatedSocketAddress.cxx \
	$(SRC)/Net/SocketAddress.cxx \
	$(SRC)/Net/SocketDescriptor.cpp

HAVE_HTTP := y

LIBNET_SOURCES += \
	$(SRC)/Net/HTTP/Multi.cpp \
	$(SRC)/Net/HTTP/Session.cpp \
	$(SRC)/Net/HTTP/Request.cpp \
	$(SRC)/Net/HTTP/FormData.cpp \
	$(SRC)/Net/HTTP/Init.cpp

ifeq ($(TARGET_IS_OSX),y)
# We use the libcurl which is included in Mac OS X.
# Mac OS X SDKs contain the required headers / library stubs,
# but no pkg-config file.
LIBNET_LDLIBS = -lcurl
else
$(eval $(call pkg-config-library,CURL,libcurl))

LIBNET_CPPFLAGS = $(CURL_CPPFLAGS)
LIBNET_LDADD = $(ZLIB_LDADD)
LIBNET_LDLIBS = $(CURL_LDLIBS) $(ZLIB_LDLIBS)
endif

ifeq ($(HAVE_HTTP),y)

LIBNET_SOURCES += \
	$(SRC)/Net/HTTP/DownloadManager.cpp \
	$(SRC)/Net/HTTP/ToFile.cpp \
	$(SRC)/Net/HTTP/ToBuffer.cpp

endif

$(eval $(call link-library,libnet,LIBNET))
