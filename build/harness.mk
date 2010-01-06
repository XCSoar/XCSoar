TEST_SRC = \
	$(TEST_SRC_DIR)/Printing.cpp \
	$(TEST_SRC_DIR)/test_debug.cpp \
	$(TEST_SRC_DIR)/harness_aircraft.cpp \
	$(TEST_SRC_DIR)/harness_airspace.cpp \
	$(TEST_SRC_DIR)/harness_flight.cpp \
	$(TEST_SRC_DIR)/harness_flight2.cpp \
	$(TEST_SRC_DIR)/harness_waypoints.cpp \
	$(TEST_SRC_DIR)/harness_task.cpp \
	$(TEST_SRC_DIR)/harness_task2.cpp \
	$(TEST_SRC_DIR)/TaskEventsPrint.cpp \
	$(TEST_SRC_DIR)/tap.c

ifeq ($(HAVE_WIN32),y)
TEST_SRC += $(TEST_SRC_DIR)/winmain.cpp
endif

$(TEST_SRC_DIR)/harness-$(TARGET).a: $(call SRC_TO_OBJ,$(TEST_SRC))
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
