ifeq ($(HAVE_HTTP),y)
# Build rules for libclient, a library containing clients for various
# network services.

LIBCLIENT_SOURCES = \
	$(SRC)/net/client/tim/Client.cpp \
	$(SRC)/net/client/WeGlide/Error.cpp \
	$(SRC)/net/client/WeGlide/DownloadTask.cpp \
	$(SRC)/net/client/WeGlide/ListTasks.cpp \
	$(SRC)/net/client/WeGlide/UploadFlight.cpp

LIBCLIENT_DEPENDS = LIBHTTP FMT JSON

$(eval $(call link-library,libclient,LIBCLIENT))
endif
