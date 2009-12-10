ifeq ($(COVERAGE),y)
FLAGS_COVERAGE := --coverage -g
else
FLAGS_COVERAGE :=
endif

# uses lcov
COVSTART = geninfo -q --no-checksum -b $(topdir) -i $(topdir)/src
COVEND = geninfo -q --no-checksum -b $(topdir) $(topdir)/src
COVPROC = genhtml --frames -q -o doc/lcov `find -iname '*\.gcda\.info'`

covstart:	FORCE
	@$(NQ)echo "coverage initialise"
	@$(Q)$(COVSTART)

cov:	FORCE
	@$(NQ)echo "coverage report"
	echo $(COVEND)
	echo $(COVPROC)
	@$(Q)$(COVEND)
	@$(Q)$(COVPROC)
