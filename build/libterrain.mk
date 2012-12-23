TERRAIN_SOURCES = \
	$(SRC)/Terrain/RasterBuffer.cpp \
	$(SRC)/Terrain/RasterProjection.cpp \
	$(SRC)/Terrain/RasterMap.cpp \
	$(SRC)/Terrain/RasterTile.cpp \
	$(SRC)/Terrain/RasterTileCache.cpp \
	$(SRC)/Terrain/RasterTerrain.cpp \
	$(SRC)/Terrain/RasterWeather.cpp \
	$(SRC)/Terrain/HeightMatrix.cpp \
	$(SRC)/Terrain/RasterRenderer.cpp \
	$(SRC)/Terrain/TerrainRenderer.cpp \
	$(SRC)/Terrain/WeatherTerrainRenderer.cpp \
	$(SRC)/Terrain/TerrainSettings.cpp

TERRAIN_CPPFLAGS_INTERNAL = $(JASPER_CPPFLAGS) $(SCREEN_CPPFLAGS)

$(eval $(call link-library,libterrain,TERRAIN))

TERRAIN_LDADD += $(JASPER_LDADD)
TERRAIN_LDLIBS += $(JASPER_LDLIBS)
