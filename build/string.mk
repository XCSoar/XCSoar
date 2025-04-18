# Make library providing string utility functions.

string_equals = $(if $(and $(findstring $(2),$(1)),$(findstring $(1),$(2))),y,n)
string_contains = $(if $(findstring $(2),$(1)),y,n)
