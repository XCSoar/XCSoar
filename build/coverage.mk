ifeq ($(COVERAGE),y)
FLAGS_COVERAGE := --coverage -g
COVERAGE_LDLIBS := --coverage
else
FLAGS_COVERAGE :=
COVERAGE_LDLIBS :=
endif

# uses lcov
#COVSTART = geninfo -q --no-checksum -b $(topdir) -i $(topdir)/src
#COVEND = geninfo -q --no-checksum -b $(topdir) $(topdir)/src

LCOVOPTS = --no-checksum -q -b $(topdir) -d $(topdir)/src

COVSTART = lcov -c -i $(LCOVOPTS) -o app_base.info
COVEND = lcov -c $(LCOVOPTS) -o app_test.info
COVMERGE = lcov -q -a app_base.info -a app_test.info -o app_total.info
COVCLEAN = lcov -q -r app_total.info "/usr/*" -o app_totalr.info
COVPROC = genhtml --frames -q -o $(DOC)/lcov app_totalr.info

covstart:	FORCE
	@$(NQ)echo "coverage initialise"
	echo $(COVSTART)
	@$(Q)$(COVSTART)

covend:	FORCE
	@$(NQ)echo "coverage report"
	echo $(COVEND)
	echo $(COVPROC)
	@$(Q)$(COVEND)
	@$(Q)$(COVMERGE)
	@$(Q)$(COVCLEAN)
	@$(Q)$(COVPROC)
