CLOUD_SERVER_SOURCES = \
	$(SRC)/Tracking/SkyLines/Server.cpp \
	$(SRC)/Tracking/SkyLines/Assemble.cpp \
	$(SRC)/Cloud/Client.cpp \
	$(SRC)/Cloud/Main.cpp
CLOUD_SERVER_DEPENDS = ASYNC OS GEO MATH UTIL
$(eval $(call link-program,xcsoar-cloud-server,CLOUD_SERVER))

ifeq ($(TARGET),UNIX)
OPTIONAL_OUTPUTS += $(CLOUD_SERVER_BIN)
endif
