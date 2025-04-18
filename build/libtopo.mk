TOPO_SOURCES = \
	$(SRC)/Topography/ShapeFile.cpp \
	$(SRC)/Topography/TopographyFile.cpp \
	$(SRC)/Topography/TopographyStore.cpp \
	$(SRC)/Topography/TopographyFileRenderer.cpp \
	$(SRC)/Topography/TopographyRenderer.cpp \
	$(SRC)/Topography/Thread.cpp \
	$(SRC)/Topography/TopographyGlue.cpp \
	$(SRC)/Topography/XShape.cpp \
	$(SRC)/Topography/Index.cpp \
	$(SRC)/Topography/CachedTopographyRenderer.cpp

TOPO_CPPFLAGS_INTERNAL = $(SCREEN_CPPFLAGS)

TOPO_DEPENDS = SHAPELIB

$(eval $(call link-library,libtopo,TOPO))
