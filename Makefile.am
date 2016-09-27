ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = etherate

etherate_SOURCES = \
	etherate.cpp \
	etherate.h
	

# TODO: Should really list funcs.cpp as a seperate compile unit in the above but there are too many dependancies
# so we just list it as an EXTRA_DIST here instead:

EXTRA_DIST = defaults.cpp \
    funcs.cpp \
    tests.cpp

