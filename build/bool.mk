# Several "GNU make" functions to evaluate boolean expressions.  Valid
# boolean values "y" (true) and "n" (false).

# Internal helper function: convert "y"/"n" to "y" / empty string.
bool_is_true = $(filter y,$(1))

# $(call bool_not,$(a))
bool_not = $(if $(call bool_is_true,$(1)),n,y)

# $(call bool_or,$(a),$(b))
bool_or = $(if $(or $(call bool_is_true,$(1)),$(call bool_is_true,$(2))),y,n)

# $(call bool_and,$(a),$(b))
bool_and = $(if $(and $(call bool_is_true,$(1)),$(call bool_is_true,$(2))),y,n)
