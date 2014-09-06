#############################################################################
#
# Makefile for RF24toTUN
#
# License: MIT
# Author:  Rei <devel@reixd.net>
# Date:    22.08.2014
#
# Description:
# ------------
# use make all and make install to install the examples
# You can change the install directory by editing the prefix line
#
prefix := /usr/local

# The recommended compiler flags for the Raspberry Pi
CCFLAGS=-Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -std=c++0x

# The needed libraries
LIBS=-lrf24-bcm -lrf24network -lboost_thread -lboost_system

# define all programs
PROGRAMS = rf24totun
SOURCES = rf24totun.cpp

all: ${PROGRAMS}

${PROGRAMS}: ${SOURCES}
	g++ ${CCFLAGS} -W -pedantic -Wall ${LIBS} $@.cpp -o $@

clean:
	rm -rf $(PROGRAMS)

install: all
	test -d $(prefix) || mkdir $(prefix)
	test -d $(prefix)/bin || mkdir $(prefix)/bin
	for prog in $(PROGRAMS); do \
	 install -m 0755 $$prog $(prefix)/bin; \
	done


.PHONY: install
