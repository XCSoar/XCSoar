
clean: cleani cleancov FORCE
	@$(NQ)echo "cleaning all"
	$(Q)find ./ $(IGNORE) \( \
		   -name '*.[oa]' \
		-o -name '*.rsc' \
		-o -name '.*.d' \
		-o -name '*.*~' \
	\) -type f -print | xargs -r $(RM)
	$(Q)$(RM) $(TARGETS) gprof.out 
	$(Q)$(RM) src/task.a src/harness.a

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

