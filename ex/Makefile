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

INCLUDES := -I.

PROFILE := 
OPTIMIZE := -g

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
#CXXFLAGS += -pedantic
#CXXFLAGS += -pedantic-errors

# -Wdisabled-optimization
# -Wunused -Wshadow -Wunreachable-code


####### sources

OBJS	:=\
	test.o \
	Util.o \
	GlideSolvers/MacCready.o \
	GlideSolvers/ZeroFinder.o \
	Navigation/Aircraft.o \
	BaseTask/AATPoint.o \
	BaseTask/OrderedTaskPoint.o \
	BaseTask/TaskProjection.o \
	BaseTask/FinishPoint.o \
	BaseTask/SampledTaskPoint.o \
	BaseTask/SearchPoint.o \
	BaseTask/TaskLeg.o \
	BaseTask/IntermediatePoint.o \
	BaseTask/StartPoint.o \
	BaseTask/TaskPoint.o \
	BaseTask/ConvexHull/GrahamScan.o \
	BaseTask/ConvexHull/PolygonInterior.o \
	Scoring/ObservationZone.o \
	TaskPoints/FAICylinderASTPoint.o \
	TaskPoints/FAISectorASTPoint.o \
	TaskPoints/FAISectorFinishPoint.o \
	TaskPoints/FAISectorStartPoint.o \
	TaskPoints/SectorAATPoint.o \
	TaskPoints/CylinderAATPoint.o \
	TaskPoints/ObservationZones/CylinderZone.o \
	TaskPoints/ObservationZones/HeightLimitZone.o \
	TaskPoints/ObservationZones/SectorZone.o \
	TaskPoints/ObservationZones/SpeedLimitZone.o \
	TaskPoints/ObservationZones/SymmetricSectorZone.o \
	Tasks/TaskStats.o \
	Tasks/AbortTask.o \
	Tasks/AbstractTask.o \
	Tasks/GotoTask.o \
	Tasks/OrderedTask.o \
	Tasks/TaskDijkstra.o \
	Tasks/TaskInterface.o \
	Tasks/TaskManager.o \
	Tasks/TaskSolvers/TaskMacCready.o \
	Tasks/TaskSolvers/TaskMacCreadyTravelled.o \
	Tasks/TaskSolvers/TaskMacCreadyRemaining.o \
	Tasks/TaskSolvers/TaskMacCreadyTotal.o \
	Tasks/TaskSolvers/TaskBestMc.o \
	Tasks/TaskSolvers/TaskCruiseEfficiency.o 


all: $(OBJS)
	g++ $(CXXFLAGS) -o test $(OBJS)


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
	find ./ $(IGNORE) \( -name '*.[oa]' -o -name '*.rsc' -o -name '.*.d' \) \
	-type f -print | xargs -r $(RM)

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
