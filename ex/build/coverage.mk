
# uses lcov

COVSTART = geninfo -q --no-checksum -b . -i src
COVEND = geninfo -q --no-checksum -b . src

covstart:	FORCE
	@$(NQ)echo "coverage initialise"
	@$(Q)$(COVSTART)

cov:	FORCE
	@$(NQ)echo "coverage report"
	@$(Q)$(COVEND)
	@$(Q)genhtml --frames -q -o doc/lcov `find -iname '*\.gcda\.info'`
