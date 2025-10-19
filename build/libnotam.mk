# Build rules for the NOTAM library

LIBNOTAM_DEPENDS = LIBHTTP BOOST JSON CO
LIBNOTAM_CPPFLAGS = -DHAVE_HTTP

LIBNOTAM_SOURCES = \
	$(SRC)/NOTAM/Client.cpp \
	$(SRC)/NOTAM/Converter.cpp \
	$(SRC)/NOTAM/Glue.cpp

$(eval $(call link-library,libnotam,LIBNOTAM))