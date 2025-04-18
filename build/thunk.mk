# Lazy assignment trick from http://cakoose.com/wiki/gnu_make_thunks

THUNK_EXPR = $$(eval $(1) := $$($(1)_GEN))$$($(1))
DEF_THUNK = $(eval $(1) = $(call THUNK_EXPR,$(1)))
