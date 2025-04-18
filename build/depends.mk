# Reverse the words of the first parameter.
reverse-words = $(eval reverse-words-result:=) \
$(foreach i,$(1),$(eval reverse-words-result := $(i) $(reverse-words-result))) \
$(reverse-words-result)

# Walk the whole dependency tree and return a flattened version.
flatten-dependencies = $(foreach i,$(1),$(i) $(call flatten-dependencies,$($(i)_DEPENDS)))

# Collect unique words, using only the last occurrence.
last-unique-words = $(eval last-unique-words-seen:=)\
$(foreach i,$(call reverse-words,$(1)),$(if $(filter $(i),$(last-unique-words-seen)),,$(eval last-unique-words-seen := $(i) $(last-unique-words-seen))))\
$(last-unique-words-seen)

# Like "flatten-dependencies", but remove duplicates, using only the
# last occurrence of each, which the linker needs to be able to
# resolve static library dependencies.
flat-depends = $(call last-unique-words,$(call flatten-dependencies,$($(1)_DEPENDS)))
