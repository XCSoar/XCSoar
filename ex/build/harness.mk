TEST_SRC = \
	Test/test_debug.cpp \
	Test/harness_aircraft.cpp \
	Test/harness_airspace.cpp \
	Test/harness_flight.cpp \
	Test/harness_flight2.cpp \
	Test/harness_waypoints.cpp \
	Test/harness_task.cpp \
	Test/harness_task2.cpp \
	Test/TaskEventsPrint.cpp \
	Test/tap.c

ifeq ($(HAVE_WIN32),y)
TEST_SRC += Test/winmain.cpp
endif

$(topdir)/Test/harness-$(TARGET).a: $(patsubst %.cpp,%-$(TARGET).o,$(TEST_SRC:.c=-$(TARGET).o))
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
