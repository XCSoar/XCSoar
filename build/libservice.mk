SERVICE_SOURCES = \
	$(SRC)/Service/TerrainService.cpp

SERVICE_CPPFLAGS_INTERNAL = $(SCREEN_CPPFLAGS)

SERVICE_DEPENDS = TERRAIN EVENT THREAD

$(eval $(call link-library,libservice,SERVICE))
