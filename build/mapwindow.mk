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
	$(SRC)/MapWindow/MapWindowDistanceRings.cpp \
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
	$(SRC)/MapWindow/UserMapScale.cpp \
	$(SRC)/MapWindow/TargetMapWindow.cpp \
	$(SRC)/MapWindow/TargetMapWindowEvents.cpp \
	$(SRC)/MapWindow/TargetMapWindowDrag.cpp

LIBMAPWINDOW_DEPENDS = SCREEN

ifeq ($(OPENGL),y)
LIBMAPWINDOW_SOURCES += \
	$(SRC)/MapWindow/OverlayBitmap.cpp

ifeq ($(SQLITE),y)
LIBMAPWINDOW_SOURCES += \
	$(SRC)/MapWindow/MbTilesDatabase.cpp \
	$(SRC)/MapWindow/MbTilesOverlay.cpp

LIBMAPWINDOW_DEPENDS += IO SQLITE
endif
endif

$(eval $(call link-library,libmapwindow,LIBMAPWINDOW))

# Rebuild the objects which evaluate DEBUG_ALL_MAP_OVERLAYS when the
# option is toggled; make does not track preprocessor flag changes.
MAP_OVERLAYS_FLAGS_STAMP = $(ABI_OUTPUT_DIR)/.debug_all_map_overlays.stamp
$(MAP_OVERLAYS_FLAGS_STAMP): FORCE | $(ABI_OUTPUT_DIR)/dirstamp
	@value=$(DEBUG_ALL_MAP_OVERLAYS); \
	if [ ! -f $@ ] || [ "$$(cat $@ 2>/dev/null)" != "$$value" ]; then \
		echo "$$value" > $@.$(RANDOM_NUMBER).tmp && \
			mv $@.$(RANDOM_NUMBER).tmp $@; \
	fi

MAP_OVERLAYS_FLAGS_SOURCES = \
	$(SRC)/MapWindow/GlueMapWindowEvents.cpp \
	$(SRC)/MapWindow/GlueMapWindowOverlays.cpp
$(call SRC_TO_OBJ,$(MAP_OVERLAYS_FLAGS_SOURCES)): $(MAP_OVERLAYS_FLAGS_STAMP)
