ifeq ($(DEBUG)$(CLANG),nn)

# These sources contain a lot of "hot" code, and should be optimized
# for speed, not for size.
HOT_SOURCES = \
	$(MATH_SOURCES) \
	$(GEO_SOURCES) \
	$(JASPER_SOURCES) \
	$(SRC)/MapWindow/OverlayBitmap.cpp \
	$(SRC)/Topography/TopographyFileRenderer.cpp \
	$(SRC)/Terrain/RasterBuffer.cpp \
	$(SRC)/Terrain/RasterMap.cpp \
	$(SRC)/Terrain/HeightMatrix.cpp \
	$(SRC)/Terrain/RasterRenderer.cpp \
	$(SRC)/Terrain/RasterTile.cpp \
	$(SRC)/Terrain/ScanLine.cpp \
	$(SRC)/Terrain/Intersection.cpp \
	$(SRC)/Projection/Projection.cpp \
	$(SRC)/Screen/Memory/Canvas.cpp \
	$(ENGINE_SRC_DIR)/Waypoints/Waypoints.cpp \
	$(ENGINE_SRC_DIR)/Airspace/Airspaces.cpp \
	$(ENGINE_SRC_DIR)/Task/Shapes/FAITriangleArea.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/MacCready.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/GlidePolar.cpp \
	$(ENGINE_SRC_DIR)/Route/FlatTriangleFan.cpp \
	$(ENGINE_SRC_DIR)/Route/FlatTriangleFanTree.cpp \
	$(ENGINE_SRC_DIR)/Route/ReachFan.cpp \
	$(ENGINE_SRC_DIR)/Route/RoutePolar.cpp \
	$(ENGINE_SRC_DIR)/Route/RouteLink.cpp \
	$(ENGINE_SRC_DIR)/Route/RoutePolars.cpp \
	$(ENGINE_SRC_DIR)/Contest/Solvers/ContestDijkstra.cpp \
	$(ENGINE_SRC_DIR)/Contest/Solvers/TraceManager.cpp \
	$(ENGINE_SRC_DIR)/Contest/Solvers/OLCTriangle.cpp

$(call SRC_TO_OBJ,$(HOT_SOURCES)): OPTIMIZE += -O3

endif
