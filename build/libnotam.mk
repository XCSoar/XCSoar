# Build rules for the NOTAM library

LIBNOTAM_DEPENDS = LIBHTTP LIBBOOST LIBJSON LIBCOROUTINES LIBFMT

LIBNOTAM_SOURCES = \
	$(SRC)/NOTAM/Client.cpp \
	$(SRC)/NOTAM/Converter.cpp \
	$(SRC)/NOTAM/Filter.cpp \
	$(SRC)/NOTAM/NOTAMGlue.cpp

$(eval $(call link-library,libnotam,LIBNOTAM))
