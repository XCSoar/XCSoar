TERRAIN_SOURCES = \
	$(SRC)/Terrain/RasterBuffer.cpp \
	$(SRC)/Terrain/RasterProjection.cpp \
	$(SRC)/Terrain/RasterMap.cpp \
	$(SRC)/Terrain/RasterTile.cpp \
	$(SRC)/Terrain/RasterTileCache.cpp \
	$(SRC)/Terrain/ZzipStream.cpp \
	$(SRC)/Terrain/Loader.cpp \
	$(SRC)/Terrain/WorldFile.cpp \
	$(SRC)/Terrain/Intersection.cpp \
	$(SRC)/Terrain/ScanLine.cpp \
	$(SRC)/Terrain/RasterTerrain.cpp \
	$(SRC)/Terrain/Thread.cpp \
	$(SRC)/Terrain/HeightMatrix.cpp \
	$(SRC)/Terrain/RasterRenderer.cpp \
	$(SRC)/Terrain/TerrainRenderer.cpp \
	$(SRC)/Terrain/TerrainSettings.cpp

TERRAIN_CPPFLAGS_INTERNAL = $(JASPER_CPPFLAGS) $(SCREEN_CPPFLAGS)

$(eval $(call link-library,libterrain,TERRAIN))

TERRAIN_LDADD += $(JASPER_LDADD)
TERRAIN_LDLIBS += $(JASPER_LDLIBS)
