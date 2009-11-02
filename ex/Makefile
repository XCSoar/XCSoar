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

SRC := .
INCLUDES := -I$(SRC) -I$(SRC)/../xcsoar/Common/Header

PROFILE := 
OPTIMIZE := -g -O2

CPPFLAGS := $(INCLUDES) 
CXXFLAGS	:=$(OPTIMIZE) -fno-exceptions $(PROFILE)
CFLAGS		:=$(OPTIMIZE) $(PROFILE)

MAKEFLAGS	+=-r

CXXFLAGS += -Wall -Wextra
CXXFLAGS += -Wwrite-strings -Wcast-qual -Wpointer-arith -Wsign-compare
CXXFLAGS += -Wmissing-noreturn -Wundef

# disable some warnings, we're not ready for them yet
CXXFLAGS += -Wno-unused-parameter -Wno-format -Wno-reorder -Wno-missing-field-initializers -Wno-switch
CXXFLAGS += -Wno-non-virtual-dtor

# InputEvents_defaults.cpp should be fixed
CXXFLAGS += -Wno-char-subscripts

# FastMath.h does dirty aliasing tricks
CXXFLAGS += -Wno-strict-aliasing

# make warnings fatal (for perfectionists)
CXXFLAGS += -Werror
CXXFLAGS += -pedantic
#CXXFLAGS += -pedantic-errors

# -Wdisabled-optimization
# -Wunused -Wshadow -Wunreachable-code

CXXFLAGS += -DDO_PRINT

####### sources

OBJS	:=\
	Util.o \
	Util/Intersection.o \
	Util/Printing.o \
	GlideSolvers/GlideState.o \
	GlideSolvers/GlidePolar.o \
	GlideSolvers/GlideResult.o \
	GlideSolvers/MacCready.o \
	GlideSolvers/ZeroFinder.o \
	Airspace/FlatBoundingBox.o \
	Airspace/Airspace.o \
	Airspace/AirspaceCircle.o \
	Airspace/AirspacePolygon.o \
	Airspace/Airspaces.o \
	Airspace/AirspaceVisitor.o \
	Navigation/Aircraft.o \
	Navigation/DistanceMemento.o \
	Navigation/GeoVector.o \
	Navigation/GeoVectorMemento.o \
	Navigation/ReferencePoint.o \
	Navigation/SearchPoint.o \
	Navigation/TaskProjection.o \
	Navigation/TaskProjectionClient.o \
	Navigation/Waypoint.o \
	Navigation/Waypoints.o \
	Navigation/WaypointVisitor.o \
	Navigation/ConvexHull/GrahamScan.o \
	Navigation/ConvexHull/PolygonInterior.o \
	Task/TaskManager.o \
	Task/TaskEvents.o \
	Task/TaskAdvance.o \
	Task/Tasks/TaskInterface.o \
	Task/Tasks/AbortTask.o \
	Task/Tasks/AbstractTask.o \
	Task/Tasks/GotoTask.o \
	Task/Tasks/OrderedTask.o \
	Task/Tasks/BaseTask/AATPoint.o \
	Task/Tasks/BaseTask/AATIsoline.o \
	Task/Tasks/BaseTask/AATIsolineSegment.o \
	Task/Tasks/BaseTask/AATIsolineIntercept.o \
	Task/Tasks/BaseTask/OrderedTaskPoint.o \
	Task/Tasks/BaseTask/ScoredTaskPoint.o \
	Task/Tasks/BaseTask/FinishPoint.o \
	Task/Tasks/BaseTask/SampledTaskPoint.o \
	Task/Tasks/BaseTask/TaskLeg.o \
	Task/Tasks/BaseTask/IntermediatePoint.o \
	Task/Tasks/BaseTask/StartPoint.o \
	Task/Tasks/BaseTask/TaskPoint.o \
	Task/Tasks/BaseTask/Scoring/ObservationZone.o \
	Task/Tasks/TaskPoints/FAICylinderASTPoint.o \
	Task/Tasks/TaskPoints/FAISectorASTPoint.o \
	Task/Tasks/TaskPoints/FAISectorFinishPoint.o \
	Task/Tasks/TaskPoints/FAISectorStartPoint.o \
	Task/Tasks/TaskPoints/SectorAATPoint.o \
	Task/Tasks/TaskPoints/CylinderAATPoint.o \
	Task/Tasks/TaskPoints/ObservationZones/CylinderZone.o \
	Task/Tasks/TaskPoints/ObservationZones/HeightLimitZone.o \
	Task/Tasks/TaskPoints/ObservationZones/SectorZone.o \
	Task/Tasks/TaskPoints/ObservationZones/SpeedLimitZone.o \
	Task/Tasks/TaskPoints/ObservationZones/SymmetricSectorZone.o \
	Task/Tasks/PathSolvers/TaskDijkstra.o \
	Task/Tasks/PathSolvers/IsolineCrossingFinder.o \
	Task/Tasks/TaskSolvers/TaskMacCready.o \
	Task/Tasks/TaskSolvers/TaskMacCreadyTravelled.o \
	Task/Tasks/TaskSolvers/TaskMacCreadyRemaining.o \
	Task/Tasks/TaskSolvers/TaskMacCreadyTotal.o \
	Task/Tasks/TaskSolvers/TaskBestMc.o \
	Task/Tasks/TaskSolvers/TaskCruiseEfficiency.o \
	Task/Tasks/TaskSolvers/TaskMinTarget.o \
	Task/Tasks/TaskSolvers/TaskOptTarget.o \
	Task/Tasks/TaskSolvers/TaskGlideRequired.o \
	Task/TaskStats/TaskStats.o \
	\
	../xcsoar/Common/Source/Math/Geometry.o \
	../xcsoar/Common/Source/Math/FastMath.o \
	../xcsoar/Common/Source/Math/Earth.o 

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
	find ./ $(IGNORE) \( -name '*.[oa]' -o -name '*.rsc' -o -name '.*.d' -o -name '*.*~' \) \
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
