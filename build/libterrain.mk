TERRAIN_SOURCES = \
	$(SRC)/Terrain/AsyncLoader.cpp \
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
	$(SRC)/Terrain/TerrainSettings.cpp \
	$(SRC)/Terrain/CogLoader.cpp \
	$(SRC)/Terrain/CogTerrain.cpp \
	$(SRC)/Terrain/TerrainBase.cpp

TERRAIN_CXXFLAGS_INTERNAL = -Wno-shift-negative-value
TERRAIN_CPPFLAGS_INTERNAL = $(SCREEN_CPPFLAGS)

TERRAIN_DEPENDS = JASPER ZZIP GEO UTIL

ifeq ($(TIFF),y)
TERRAIN_CPPFLAGS_INTERNAL += $(LIBTIFF_CPPFLAGS)
TERRAIN_DEPENDS += LIBTIFF
endif

ifeq ($(HAVE_HTTP),y)
TERRAIN_DEPENDS += ZLIB
endif

$(eval $(call link-library,libterrain,TERRAIN))
