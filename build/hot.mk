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
	$(SRC)/Projection.cpp

$(call SRC_TO_OBJ,$(HOT_SOURCES)): OPTIMIZE += -O3

endif
