CLOUD_SERVER_SOURCES = \
	$(SRC)/Tracking/SkyLines/Server.cpp \
	$(SRC)/Tracking/SkyLines/Assemble.cpp \
	$(SRC)/Cloud/Serialiser.cpp \
	$(SRC)/Cloud/Client.cpp \
	$(SRC)/Cloud/Thermal.cpp \
	$(SRC)/Cloud/Data.cpp \
	$(SRC)/Cloud/Sender.cpp \
	$(SRC)/Cloud/Main.cpp
CLOUD_SERVER_DEPENDS = ASYNC IO OS GEO MATH UTIL
$(eval $(call link-program,xcsoar-cloud-server,CLOUD_SERVER))

CLOUD_TO_KML_SOURCES = \
	$(SRC)/Tracking/SkyLines/Assemble.cpp \
	$(SRC)/Cloud/Serialiser.cpp \
	$(SRC)/Cloud/Client.cpp \
	$(SRC)/Cloud/Thermal.cpp \
	$(SRC)/Cloud/Data.cpp \
	$(SRC)/Cloud/ToKML.cpp
CLOUD_TO_KML_DEPENDS = ASYNC IO OS GEO MATH UTIL
$(eval $(call link-program,xcsoar-cloud-to-kml,CLOUD_TO_KML))

ifeq ($(TARGET),UNIX)
OPTIONAL_OUTPUTS += $(CLOUD_SERVER_BIN) $(CLOUD_TO_KML_BIN)
endif
