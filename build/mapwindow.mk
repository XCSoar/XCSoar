LIBMAPWINDOW_SOURCES = \
	$(SRC)/MapWindow/MapWindowBlackboard.cpp \
	$(SRC)/MapWindow/MapCanvas.cpp \
	$(SRC)/MapWindow/StencilMapCanvas.cpp \
	$(SRC)/MapWindow/Items/MapItem.cpp \
	$(SRC)/MapWindow/Items/OverlayMapItem.cpp \
	$(SRC)/MapWindow/Items/List.cpp \
	$(SRC)/MapWindow/Items/Builder.cpp \
	$(SRC)/MapWindow/Items/AirspaceBuilder.cpp \
	$(SRC)/MapWindow/Items/TrafficBuilder.cpp \
	$(SRC)/MapWindow/Items/WeatherBuilder.cpp \
	$(SRC)/MapWindow/MapWindow.cpp \
	$(SRC)/MapWindow/MapWindowEvents.cpp \
	$(SRC)/MapWindow/MapWindowGlideRange.cpp \
	$(SRC)/Projection/MapWindowProjection.cpp \
	$(SRC)/MapWindow/MapWindowRender.cpp \
	$(SRC)/MapWindow/MapWindowSymbols.cpp \
	$(SRC)/MapWindow/MapWindowContest.cpp \
	$(SRC)/MapWindow/MapWindowTask.cpp \
	$(SRC)/MapWindow/MapWindowThermal.cpp \
	$(SRC)/MapWindow/MapWindowTraffic.cpp \
	$(SRC)/MapWindow/MapWindowTrail.cpp \
	$(SRC)/MapWindow/MapWindowWaypoints.cpp \
	$(SRC)/MapWindow/GlueMapWindow.cpp \
	$(SRC)/MapWindow/GlueMapWindowItems.cpp \
	$(SRC)/MapWindow/GlueMapWindowEvents.cpp \
	$(SRC)/MapWindow/GlueMapWindowOverlays.cpp \
	$(SRC)/MapWindow/GlueMapWindowDisplayMode.cpp \
	$(SRC)/MapWindow/TargetMapWindow.cpp \
	$(SRC)/MapWindow/TargetMapWindowEvents.cpp \
	$(SRC)/MapWindow/TargetMapWindowDrag.cpp

ifeq ($(OPENGL),y)
LIBMAPWINDOW_SOURCES += \
	$(SRC)/MapWindow/OverlayBitmap.cpp
endif

LIBMAPWINDOW_CPPFLAGS_INTERNAL = $(SCREEN_CPPFLAGS)

$(eval $(call link-library,libmapwindow,LIBMAPWINDOW))
