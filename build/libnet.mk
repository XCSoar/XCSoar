# Build rules for the HTTP client library

LIBNET_SOURCES =
HAVE_HTTP := n

ifneq ($(findstring $(TARGET),PC WINE CYGWIN),)
HAVE_HTTP := y
LIBNET_SOURCES += \
	$(SRC)/Net/WinINet/Session.cpp \
	$(SRC)/Net/WinINet/Request.cpp
LIBNET_LDLIBS = -lwininet
endif

# don't link with CURL on Mac OS X, to keep the dynamic library
# dependencies low
ifeq ($(TARGET_IS_DARWIN),n)
ifeq ($(TARGET),UNIX)
HAVE_HTTP := y

LIBNET_SOURCES += \
	$(SRC)/Net/CURL/Multi.cpp \
	$(SRC)/Net/CURL/Session.cpp \
	$(SRC)/Net/CURL/Request.cpp \
	$(SRC)/Net/CURL/Init.cpp

$(eval $(call pkg-config-library,CURL,libcurl))

LIBNET_CPPFLAGS = $(CURL_CPPFLAGS)
LIBNET_LDLIBS = $(CURL_LDLIBS)
endif
endif

ifeq ($(TARGET),ANDROID)
HAVE_HTTP := y

LIBNET_SOURCES += \
	$(SRC)/Net/Java/Session.cpp \
	$(SRC)/Net/Java/Request.cpp
endif

ifeq ($(HAVE_HTTP),y)

LIBNET_SOURCES += \
	$(SRC)/Net/DownloadManager.cpp \
	$(SRC)/Net/ToFile.cpp \
	$(SRC)/Net/ToBuffer.cpp

$(eval $(call link-library,libnet,LIBNET))

endif
