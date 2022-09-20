# Build rules for libclient, a library containing clients for various
# network services.

LIBCLIENT_SOURCES = \
	$(SRC)/net/client/tim/Client.cpp \
	$(SRC)/net/client/WeGlide/DownloadTask.cpp \
	$(SRC)/net/client/WeGlide/UploadFlight.cpp

LIBCLIENT_CPPFLAGS_INTERNAL = $(LIBHTTP_CPPFLAGS)
LIBCLIENT_LDLIBS = $(LIBHTTP_LDLIBS)

$(eval $(call link-library,libclient,LIBCLIENT))
