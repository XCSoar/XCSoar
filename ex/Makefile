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
# -pg
COVERAGE := 
# --coverage

OPTIMIZE := -g 
# -O2

CPPFLAGS := $(INCLUDES) 
CXXFLAGS	:=$(OPTIMIZE) -fno-exceptions $(PROFILE) $(COVERAGE)
CFLAGS		:=$(OPTIMIZE) $(PROFILE) $(COVERAGE)

MAKEFLAGS	+=-r

CXXFLAGS += -Wall -Wextra
CXXFLAGS += -Wwrite-strings -Wcast-qual -Wpointer-arith -Wsign-compare
CXXFLAGS += -Wmissing-noreturn -Wundef
CXXFLAGS += -Wno-variadic-macros 

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

CXXFLAGS += -DDO_PRINT 
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
	src/Navigation/GeoPoint.o \
	src/Navigation/ReferencePoint.o \
	src/Navigation/SearchPoint.o \
	src/Navigation/SearchPointVector.o \
	src/Navigation/TaskProjection.o \
	src/Navigation/ConvexHull/GrahamScan.o \
	src/Navigation/ConvexHull/PolygonInterior.o \
	src/Navigation/Memento/DistanceMemento.o \
	src/Navigation/Memento/GeoVectorMemento.o \
	src/Navigation/Geometry/GeoVector.o \
	src/Navigation/Flat/FlatBoundingBox.o \
	src/Navigation/Flat/FlatGeoPoint.o \
	src/Navigation/Flat/FlatRay.o \
	src/Navigation/Flat/FlatPoint.o \
	src/Navigation/Flat/FlatEllipse.o \
	src/Navigation/Flat/FlatLine.o \
	src/Task/TaskAdvance.o \
	src/Task/TaskBehaviour.o \
	src/Task/TaskEvents.o \
	src/Task/TaskManager.o \
	src/Task/Factory/AbstractTaskFactory.o \
	src/Task/Factory/FAITaskFactory.o \
	src/Task/Factory/AATTaskFactory.o \
	src/Task/Factory/MixedTaskFactory.o \
	src/Task/Tasks/TaskInterface.o \
	src/Task/Tasks/AbortTask.o \
	src/Task/Tasks/AbstractTask.o \
	src/Task/Tasks/GotoTask.o \
	src/Task/Tasks/OrderedTask.o \
	src/Task/Tasks/BaseTask/OrderedTaskPoint.o \
	src/Task/Tasks/BaseTask/ScoredTaskPoint.o \
	src/Task/Tasks/BaseTask/SampledTaskPoint.o \
	src/Task/Tasks/BaseTask/TaskLeg.o \
	src/Task/Tasks/BaseTask/TaskPoint.o \
	src/Task/Tasks/BaseTask/IntermediatePoint.o \
	src/Task/Tasks/BaseTask/ObservationZone.o \
	src/Task/Tasks/BaseTask/ObservationZoneClient.o \
	src/Task/Tasks/BaseTask/ObservationZonePoint.o \
	src/Task/TaskPoints/StartPoint.o \
	src/Task/TaskPoints/FinishPoint.o \
	src/Task/TaskPoints/ASTPoint.o \
	src/Task/TaskPoints/AATPoint.o \
	src/Task/TaskPoints/AATIsoline.o \
	src/Task/TaskPoints/AATIsolineSegment.o \
	src/Task/ObservationZones/CylinderZone.o \
	src/Task/ObservationZones/SectorZone.o \
	src/Task/ObservationZones/LineSectorZone.o \
	src/Task/ObservationZones/SymmetricSectorZone.o \
	src/Task/ObservationZones/FAISectorZone.o \
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
	src/Task/TaskStats/DistanceStat.o \
	src/Task/TaskStats/ElementStat.o \
	src/Task/TaskStats/TaskStats.o \
	src/Waypoint/Waypoint.o \
	src/Waypoint/Waypoints.o \
	src/Waypoint/WaypointVisitor.o \
	src/Math/Geometry.o \
	src/Math/FastMath.o \
	src/Math/Earth.o \
	src/Util/AvFilter.o \
	src/Util/DiffFilter.o \
	src/Util/Filter.o \
	src/Util/Gradient.o \
	src/Util/Printing.o \
	src/Util/ZeroFinder.o 

TESTOBJS = Test/test_debug.o \
	Test/harness_aircraft.o \
	Test/harness_airspace.o \
	Test/harness_flight.o \
	Test/harness_waypoints.o \
	Test/harness_task.o \
	Test/tap.o

TESTFAST = \
	Test/test_waypoints.exe \
	Test/test_task.exe \
	Test/test_mc.exe \
	Test/test_airspace.exe \
	Test/test_automc.exe \
	Test/test_modes.exe \
	Test/test_trees.exe \
	Test/test.exe \
	Test/test_edittp.exe \
	Test/test_highterrain.exe

TESTSLOW = \
	Test/test_randomtask.exe \
	Test/test_vopt.exe \
	Test/test_cruiseefficiency.exe \
	Test/test_bestcruisetrack.exe \
	Test/test_aat.exe \
	Test/test_flight.exe 

TESTS = $(TESTFAST) $(TESTSLOW)

TARGETS = $(TESTS) 

all: $(OBJS) $(TESTOBJS) $(TARGETS)
	@echo "done"

COVSTART = geninfo -q --no-checksum -b . -i src
COVEND = geninfo -q --no-checksum -b . src

testslow:	$(TESTSLOW)
	$(Q)perl Test/testall.pl $(TESTSLOW)

testfast:	$(TESTFAST)
	$(Q)perl Test/testall.pl $(TESTFAST)


%.exe: %.cpp $(OBJS) $(TESTOBJS)
	@$(NQ)echo "  CXX/LN      $@"
	$(Q)g++ $(CXXFLAGS) $(INCLUDES) $(OBJS) $(TESTOBJS) $< -o $@

doco: FORCE
	tools/cloc --exclude-lang=D src > doc/cloc.txt
	doxygen doc/doxygen/Doxyfile | grep Warning

DEPFILE		=$(dir $@).$(notdir $@).d
DEPFLAGS	=-Wp,-MM,-MF,$(DEPFILE)
dirtarget	=$(subst \\,_,$(subst /,_,$(dir $@)))
cc-flags	=$(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(CPPFLAGS_$(dirtarget)) $(TARGET_ARCH)
cxx-flags	=$(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(CPPFLAGS_$(dirtarget)) $(TARGET_ARCH)

covstart:	FORCE
	@$(NQ)echo "coverage initialise"
	@$(Q)$(COVSTART)

cov:	FORCE
	@$(NQ)echo "coverage report"
	@$(Q)$(COVEND)
	@$(Q)genhtml --frames -q -o doc/lcov `find -iname '*\.gcda\.info'`

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


clean: cleani cleancov FORCE
	@$(NQ)echo "cleaning all"
	$(Q)find ./ $(IGNORE) \( \
		   -name '*.[oa]' \
		-o -name '*.rsc' \
		-o -name '.*.d' \
		-o -name '*.*~' \
	\) -type f -print | xargs -r $(RM)
	$(Q)$(RM) $(TARGETS) gprof.out 
	$(Q)$(RM) $(OBJS)

cleancov: FORCE
	@$(NQ)echo "cleaning cov"
	$(Q)find ./ $(IGNORE) \( \
		   -name '*.bb' \
		-o -name '*.bbg' \
		-o -name '*.gcda' \
		-o -name '*.gcda.info' \
		-o -name '*.gcno' \
		-o -name '*.gcno.info' \
	\) -type f -print | xargs -r $(RM)

cleani: FORCE
	@$(NQ)echo "cleaning .i"
	$(Q)find ./ $(IGNORE) \( -name '*.i' \) \
		-type f -print | xargs -r $(RM)

.PHONY: FORCE

ifneq ($(wildcard $(SRC)/.*.d),)
include $(wildcard $(SRC)/.*.d)
endif
ifneq ($(wildcard $(SRC)/*/.*.d),)
include $(wildcard $(SRC)/*/.*.d)
endif
