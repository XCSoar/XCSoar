# On Linux, we expect Boost to be installed already, and on all other
# targets, it is installed by thirdparty.py.  Neither needs special
# compiler/linker flags.

BOOST_LDLIBS =

# reduce Boost header bloat a bit
BOOST_CPPFLAGS = -DBOOST_NO_IOSTREAM -DBOOST_MATH_NO_LEXICAL_CAST
