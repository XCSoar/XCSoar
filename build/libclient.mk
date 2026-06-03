ifeq ($(HAVE_HTTP),y)
# Build rules for libclient, a library containing clients for various
# network services.

LIBCLIENT_SOURCES = \
	$(SRC)/net/client/SyncHttp.cpp \
	$(SRC)/net/client/auth/JwtBearerSession.cpp \
	$(SRC)/net/client/xctherm/Http.cpp \
	$(SRC)/net/client/tim/Client.cpp \
	$(SRC)/net/client/WeGlide/AircraftCache.cpp \
	$(SRC)/net/client/WeGlide/AircraftList.cpp \
	$(SRC)/net/client/WeGlide/Error.cpp \
	$(SRC)/net/client/WeGlide/DownloadTask.cpp \
	$(SRC)/net/client/WeGlide/ListTasks.cpp \
	$(SRC)/net/client/WeGlide/UploadFlight.cpp

LIBCLIENT_DEPENDS = LIBHTTP FMT JSON

$(eval $(call link-library,libclient,LIBCLIENT))
endif
