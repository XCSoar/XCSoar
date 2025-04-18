# Build rules for the flarm library

FLARM_SOURCES = \
	$(SRC)/FLARM/Id.cpp \
	$(SRC)/FLARM/Error.cpp \
	$(SRC)/FLARM/List.cpp \
	$(SRC)/FLARM/FlarmNetRecord.cpp \
	$(SRC)/FLARM/FlarmNetDatabase.cpp \
	$(SRC)/FLARM/FlarmNetReader.cpp \
	$(SRC)/FLARM/Traffic.cpp \
	$(SRC)/FLARM/Calculations.cpp \
	$(SRC)/FLARM/Friends.cpp \
	$(SRC)/FLARM/Computer.cpp \
	$(SRC)/FLARM/Global.cpp \
	$(SRC)/FLARM/Glue.cpp \

FLARM_DEPENDS = FMT

$(eval $(call link-library,flarm,FLARM))
