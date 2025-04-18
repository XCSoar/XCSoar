ifeq ($(COVERAGE),y)
FLAGS_COVERAGE := --coverage -g
COVERAGE_LDLIBS := --coverage
else
FLAGS_COVERAGE :=
COVERAGE_LDLIBS :=
endif

COVERAGE_DIR = $(TARGET_OUTPUT_DIR)/coverage
COVERAGE_DATA_DIR = $(COVERAGE_DIR)/data
COVERAGE_HTML_DIR = $(COVERAGE_DIR)/html

# uses lcov
#COVSTART = geninfo -q --no-checksum -b $(topdir) -i $(topdir)/src
#COVEND = geninfo -q --no-checksum -b $(topdir) $(topdir)/src

LCOVOPTS = --no-checksum -q -b $(topdir) -d $(TARGET_OUTPUT_DIR)/src

COVSTART = lcov -c -i $(LCOVOPTS) -o $(COVERAGE_DATA_DIR)/app_base.info
COVEND = lcov -c $(LCOVOPTS) -o $(COVERAGE_DATA_DIR)/app_test.info
COVMERGE = lcov -q -a $(COVERAGE_DATA_DIR)/app_base.info -a $(COVERAGE_DATA_DIR)/app_test.info -o $(COVERAGE_DATA_DIR)/app_total.info
COVCLEAN = lcov -q -r $(COVERAGE_DATA_DIR)/app_total.info "/usr/*" -o $(COVERAGE_DATA_DIR)/app_totalr.info
COVPROC = genhtml --frames -q -o $(COVERAGE_HTML_DIR) $(COVERAGE_DATA_DIR)/app_totalr.info

covstart: FORCE | $(COVERAGE_DATA_DIR)/dirstamp
	@$(NQ)echo "coverage initialise"
	echo $(COVSTART)
	@$(Q)$(COVSTART)

covend:	FORCE | $(COVERAGE_DATA_DIR)/dirstamp $(COVERAGE_HTML_DIR)/dirstamp
	@$(NQ)echo "coverage report"
	echo $(COVEND)
	echo $(COVPROC)
	@$(Q)$(COVEND)
	@$(Q)$(COVMERGE)
	@$(Q)$(COVCLEAN)
	@$(Q)$(COVPROC)
