# Build rules for the NOTAM library

LIBNOTAM_DEPENDS = LIBHTTP LIBBOOST LIBJSON LIBCOROUTINES LIBFMT AIRSPACE

LIBNOTAM_SOURCES = \
	$(SRC)/NOTAM/AirspaceSync.cpp \
	$(SRC)/NOTAM/Client.cpp \
	$(SRC)/NOTAM/Converter.cpp \
	$(SRC)/NOTAM/Delta.cpp \
	$(SRC)/NOTAM/Filter.cpp \
	$(SRC)/NOTAM/NOTAMCache.cpp \
	$(SRC)/NOTAM/NOTAMGlue.cpp \
	$(SRC)/NOTAM/Sync.cpp

$(eval $(call link-library,libnotam,LIBNOTAM))
