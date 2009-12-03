TEST_OBJS = \
	Test/test_debug.o \
	Test/harness_aircraft.o \
	Test/harness_airspace.o \
	Test/harness_flight.o \
	Test/harness_flight2.o \
	Test/harness_waypoints.o \
	Test/harness_task.o \
	Test/harness_task2.o \
	Test/TaskEventsPrint.o \
	Test/tap.o

ifeq ($(HAVE_WIN32),y)
TEST_OBJS += Test/winmain.o
endif

Test/harness.a: $(TEST_OBJS)
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
