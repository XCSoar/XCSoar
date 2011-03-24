ifeq ($(DEBUG)$(CLANG),nn)

# These sources contain a lot of "hot" code, and should be optimized
# for speed, not for size.
HOT_SOURCES = \
	$(MATH_SOURCES) \
	$(ZLIB_SOURCES) \
	$(JASPER) \
	$(SRC)/Topography/TopographyRenderer.cpp \
	$(SRC)/Terrain/RasterBuffer.cpp \
	$(SRC)/Terrain/RasterMap.cpp \
	$(SRC)/Terrain/HeightMatrix.cpp \
	$(SRC)/Terrain/RasterRenderer.cpp \
	$(SRC)/Terrain/RasterTile.cpp \
	$(SRC)/Projection.cpp \
	$(SRC)/Geo/GeoClip.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/MacCready.cpp \
	$(ENGINE_SRC_DIR)/Util/ZeroFinder.cpp \
	$(ENGINE_SRC_DIR)/Navigation/ConvexHull/GrahamScan.cpp \
	$(ENGINE_SRC_DIR)/Navigation/ConvexHull/PolygonInterior.cpp \
	$(ENGINE_SRC_DIR)/Navigation/Flat/FlatRay.cpp

$(call SRC_TO_OBJ,$(HOT_SOURCES)): OPTIMIZE += -O3

endif
