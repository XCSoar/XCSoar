# Build rules for the NOTAM library

LIBNOTAM_DEPENDS = LIBHTTP BOOST JSON CO FMT

LIBNOTAM_SOURCES = \
	$(SRC)/NOTAM/Client.cpp \
	$(SRC)/NOTAM/Converter.cpp \
	$(SRC)/NOTAM/NOTAMGlue.cpp

$(eval $(call link-library,libnotam,LIBNOTAM))