####### build verbosity

# Internal - Control verbosity
#  make V=0 - quiet
#  make V=1 - terse (default)
#  make V=2 - show commands
ifeq ($(V),2)
Q		:=
NQ		:=\#
else
Q		:=@
ifeq ($(V),0)
NQ		:=\#
else
NQ		:=
endif
endif


OUTPUTS 	:= test

SRC := src
INCLUDES := -I$(SRC)

PROFILE := 
# --coverage

OPTIMIZE := -g 

CPPFLAGS := $(INCLUDES) 
CXXFLAGS	:=$(OPTIMIZE) -fno-exceptions $(PROFILE)
CFLAGS		:=$(OPTIMIZE) $(PROFILE)

MAKEFLAGS	+=-r

CXXFLAGS += -Wall -Wextra
CXXFLAGS += -Wwrite-strings -Wcast-qual -Wpointer-arith -Wsign-compare
CXXFLAGS += -Wmissing-noreturn -Wundef

# disable some warnings, we're not ready for them yet
CXXFLAGS += -Wno-unused-parameter 
CXXFLAGS += -Wno-reorder 
#CXXFLAGS += -Wno-format -Wno-switch
#CXXFLAGS += -Wno-non-virtual-dtor 
#CXXFLAGS += -Wno-missing-field-initializers 
# InputEvents_defaults.cpp should be fixed
#CXXFLAGS += -Wno-char-subscripts
# FastMath.h does dirty aliasing tricks
#CXXFLAGS += -Wno-strict-aliasing

# make warnings fatal (for perfectionists)
CXXFLAGS += -Werror
CXXFLAGS += -pedantic

# -Wdisabled-optimization
# -Wunused -Wshadow -Wunreachable-code

CXXFLAGS += -DDO_PRINT -DINSTRUMENT_TASK
# -DINSTRUMENT_TASK

####### sources

OBJS	:=\
	src/Airspace/Airspace.o \
	src/Airspace/AirspaceCircle.o \
	src/Airspace/AirspacePolygon.o \
	src/Airspace/Airspaces.o \
	src/Airspace/AirspaceVisitor.o \
	src/GlideSolvers/GlideState.o \
	src/GlideSolvers/GlidePolar.o \
	src/GlideSolvers/GlideResult.o \
	src/GlideSolvers/MacCready.o \
	src/Navigation/Aircraft.o \
	src/Navigation/DistanceMemento.o \
	src/Navigation/FlatBoundingBox.o \
	src/Navigation/FlatGeoPoint.o \
	src/Navigation/FlatRay.o \
	src/Navigation/FlatPoint.o \
	src/Navigation/FlatLine.o \
	src/Navigation/FlatEllipse.o \
	src/Navigation/GeoPoint.o \
	src/Navigation/GeoVector.o \
	src/Navigation/GeoVectorMemento.o \
	src/Navigation/ReferencePoint.o \
	src/Navigation/SearchPoint.o \
	src/Navigation/TaskProjection.o \
	src/Navigation/TaskProjectionClient.o \
	src/Navigation/ConvexHull/GrahamScan.o \
	src/Navigation/ConvexHull/PolygonInterior.o \
	src/Task/TaskManager.o \
	src/Task/TaskEvents.o \
	src/Task/TaskAdvance.o \
	src/Task/Tasks/TaskInterface.o \
	src/Task/Tasks/AbortTask.o \
	src/Task/Tasks/AbstractTask.o \
	src/Task/Tasks/GotoTask.o \
	src/Task/Tasks/OrderedTask.o \
	src/Task/Tasks/BaseTask/AATPoint.o \
	src/Task/Tasks/BaseTask/AATIsoline.o \
	src/Task/Tasks/BaseTask/AATIsolineSegment.o \
	src/Task/Tasks/BaseTask/AATIsolineIntercept.o \
	src/Task/Tasks/BaseTask/OrderedTaskPoint.o \
	src/Task/Tasks/BaseTask/ScoredTaskPoint.o \
	src/Task/Tasks/BaseTask/FinishPoint.o \
	src/Task/Tasks/BaseTask/SampledTaskPoint.o \
	src/Task/Tasks/BaseTask/TaskLeg.o \
	src/Task/Tasks/BaseTask/IntermediatePoint.o \
	src/Task/Tasks/BaseTask/StartPoint.o \
	src/Task/Tasks/BaseTask/TaskPoint.o \
	src/Task/Tasks/BaseTask/Scoring/ObservationZone.o \
	src/Task/Tasks/TaskPoints/FAICylinderASTPoint.o \
	src/Task/Tasks/TaskPoints/FAISectorASTPoint.o \
	src/Task/Tasks/TaskPoints/FAISectorFinishPoint.o \
	src/Task/Tasks/TaskPoints/FAISectorStartPoint.o \
	src/Task/Tasks/TaskPoints/SectorAATPoint.o \
	src/Task/Tasks/TaskPoints/CylinderAATPoint.o \
	src/Task/Tasks/TaskPoints/ObservationZones/CylinderZone.o \
	src/Task/Tasks/TaskPoints/ObservationZones/HeightLimitZone.o \
	src/Task/Tasks/TaskPoints/ObservationZones/SectorZone.o \
	src/Task/Tasks/TaskPoints/ObservationZones/SpeedLimitZone.o \
	src/Task/Tasks/TaskPoints/ObservationZones/SymmetricSectorZone.o \
	src/Task/Tasks/PathSolvers/TaskDijkstra.o \
	src/Task/Tasks/PathSolvers/IsolineCrossingFinder.o \
	src/Task/Tasks/TaskSolvers/TaskMacCready.o \
	src/Task/Tasks/TaskSolvers/TaskMacCreadyTravelled.o \
	src/Task/Tasks/TaskSolvers/TaskMacCreadyRemaining.o \
	src/Task/Tasks/TaskSolvers/TaskMacCreadyTotal.o \
	src/Task/Tasks/TaskSolvers/TaskBestMc.o \
	src/Task/Tasks/TaskSolvers/TaskCruiseEfficiency.o \
	src/Task/Tasks/TaskSolvers/TaskMinTarget.o \
	src/Task/Tasks/TaskSolvers/TaskOptTarget.o \
	src/Task/Tasks/TaskSolvers/TaskGlideRequired.o \
	src/Task/TaskStats/TaskStats.o \
	src/Waypoint/Waypoint.o \
	src/Waypoint/Waypoints.o \
	src/Waypoint/WaypointVisitor.o \
	src/Math/Geometry.o \
	src/Math/FastMath.o \
	src/Math/Earth.o \
	src/Util/Filter.o \
	src/Util/Gradient.o \
	src/Util/Printing.o \
	src/Util/ZeroFinder.o \

Test/test: $(OBJS) Test/test.cpp
	g++ $(CXXFLAGS) $(INCLUDES) $(OBJS) Test/test.cpp -o Test/test 

Test/test_trees: $(OBJS) Test/test_trees.cpp
	g++ $(CXXFLAGS) $(INCLUDES) $(OBJS) Test/test_trees.cpp -o Test/test_trees 

Test/test_mc: $(OBJS) Test/test_mc.cpp
	g++ $(CXXFLAGS) $(INCLUDES) $(OBJS) Test/test_mc.cpp -o Test/test_mc 

TARGETS = Test/test Test/test_trees Test/test_mc

all: $(TARGETS)
	@echo "done"


DEPFILE		=$(dir $@).$(notdir $@).d
DEPFLAGS	=-Wp,-MD,$(DEPFILE)
dirtarget	=$(subst \\,_,$(subst /,_,$(dir $@)))
cc-flags	=$(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(CPPFLAGS_$(dirtarget)) $(TARGET_ARCH)
cxx-flags	=$(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(CPPFLAGS_$(dirtarget)) $(TARGET_ARCH)

#
# Useful debugging targets - make preprocessed versions of the source
#
%.i: %.cpp FORCE
	$(CXX) $(cxx-flags) -E $(OUTPUT_OPTION) $<

%.s: %.cpp FORCE
	$(CXX) $(cxx-flags) -S $(OUTPUT_OPTION) $<

%.i: %.c FORCE
	$(CC) $(cc-flags) -E $(OUTPUT_OPTION) $<

####### build rules
#
#
# Provide our own rules for building...
#
%.o: %.c
	@$(NQ)echo "  CC      $@"
	$(Q)$(CC) $(cc-flags) -c $(OUTPUT_OPTION) $<
	@sed -i '1s,^[^ :]*,$@,' $(DEPFILE)
#	@etags -a --declarations $<

%.o: %.cpp
	@$(NQ)echo "  CXX     $@"
	$(Q)$(CXX) $(cxx-flags) -c $(OUTPUT_OPTION) $<
	@sed -i '1s,^[^ :]*,$@,' $(DEPFILE)
#	@etags -a --declarations $<

%.o: %.cxx
	@$(NQ)echo "  CXX     $@"
	$(Q)$(CXX) $(cxx-flags) -c $(OUTPUT_OPTION) $<
	@sed -i '1s,^[^ :]*,$@,' $(DEPFILE)


clean: cleani FORCE
	find ./ $(IGNORE) \( -name '*.[oa]' -o -name '*.rsc' \
		-o -name '.*.d' \
		-o -name '*.bb' \
		-o -name '*.bbg' \
		-o -name '*.gcda' \
		-o -name '*.gcno' \
		-o -name '*.gcov' \
		-o -name '*.*~' \) \
	-type f -print | xargs -r $(RM)
	$(RM) $(TARGETS) gprof.out 
	$(RM) $(OBJS)

cleani: FORCE
	find ./ $(IGNORE) \( -name '*.i' \) \
		-type f -print | xargs -r $(RM)

.PHONY: FORCE

ifneq ($(wildcard $(SRC)/.*.d),)
include $(wildcard $(SRC)/.*.d)
endif
ifneq ($(wildcard $(SRC)/*/.*.d),)
include $(wildcard $(SRC)/*/.*.d)
endif
