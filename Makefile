# Copyright 2012 Peter Bašista
#
# This file is part of the rsgen
#
# rsgen is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# The name of the project
PNAME := rsgen

# The name of the shared library
LIBNAME := randomc

APREFIX := $(PNAME)

ARCHIVE_NC := $(APREFIX).tar
ARCHIVE_GZ := $(ARCHIVE_NC).gz
ARCHIVE_XZ := $(ARCHIVE_NC).xz

# we need to define some basic variables
CPP := c++

# File extensions
HDREXT := .h
SRCEXT := .cpp
OBJEXT := .o
DEPEXT := .d
LIBEXT := .so

LIBDIR := $(LIBNAME)
LIBDIRPATH := $$ORIGIN/$(LIBNAME)

LIBHDRDIR := $(LIBDIR)/h
LIBSRCDIR := $(LIBDIR)/src
LIBOBJDIR := $(LIBDIR)/obj
LIBDEPDIR := $(LIBDIR)/d
LNAME := $(LIBDIR)/lib$(LIBNAME)$(LIBEXT)

HDRDIR := h
SRCDIR := src
OBJDIR := obj
DEPDIR := d
ENAME := $(PNAME)

LIBCFLAGS := -I$(HDRDIR) -I$(LIBHDRDIR) -msse2

CFLAGS := -I$(HDRDIR) -I$(LIBHDRDIR)

LIBLIBFLAGS := -shared

LIBLIBS :=

# The version of the tar program present in the current system.
TAR_VERSION := $(shell tar --version | head -n 1 | cut -d ' ' -f 1)

# A flag indicating whether the xz compression utility is not available
XZ_UNAVAILABLE := $(shell hash xz 2>/dev/null || echo "COMMAND_UNAVAILABLE")

# Kernel name as returned by "uname -s"
KNAME := $(shell uname -s)

LIBFLAGS := -L$(LIBDIR) -Wl,-rpath,'$(LIBDIRPATH)'
# If we are on the Mac OS, we would like to link with the iconv
ifeq ($(KNAME),Darwin)
LIBS := -l$(LIBNAME) -liconv
else
LIBS := -l$(LIBNAME)
endif

AFLAGS := -std=gnu++98 -fpic -O3 -Wall -Wextra -Wconversion -pedantic -g

LIBHEADERS := $(wildcard $(LIBHDRDIR)/*$(HDREXT))
LIBSOURCES := $(wildcard $(LIBSRCDIR)/*$(SRCEXT))
LIBOBJECTS := $(addprefix $(LIBOBJDIR)/,\
	$(notdir $(LIBSOURCES:$(SRCEXT)=$(OBJEXT))))
LIBDEPENDENCIES := $(addprefix $(LIBDEPDIR)/,\
	$(notdir $(LIBSOURCES:$(SRCEXT)=$(DEPEXT))))
LIBOTHERFILES := $(LIBDIR)/license.txt $(LIBDIR)/ran-instructions.pdf \
	$(LIBDIR)/readme.txt $(LIBDIR)/ex-ran.cpp $(LIBDIR)/rancombi.cpp \
	$(LIBDIR)/testirandomx.cpp

HEADERS := $(wildcard $(HDRDIR)/*$(HDREXT))
SOURCES := $(wildcard $(SRCDIR)/*$(SRCEXT))
OBJECTS := $(addprefix $(OBJDIR)/,\
	$(notdir $(SOURCES:$(SRCEXT)=$(OBJEXT))))
DEPENDENCIES := $(addprefix $(DEPDIR)/,\
	$(notdir $(SOURCES:$(SRCEXT)=$(DEPEXT))))
OTHERFILES := COPYING Makefile README

.PHONY: libclean clean distclean distgz distxz dist

# First and the default target

all: $(LIBDEPENDENCIES) $(LIBOBJDIR) $(LIBOBJECTS) $(LNAME) \
	$(DEPENDENCIES) $(OBJDIR) $(OBJECTS) $(ENAME)
	@echo "$(PNAME) has been made"

lib: $(LIBDEPENDENCIES) $(LIBOBJDIR) $(LIBOBJECTS) $(LNAME)
	@echo "library $(LIBNAME) has been made"

$(LIBDEPENDENCIES): $(LIBDEPDIR)/%$(DEPEXT): $(LIBSRCDIR)/%$(SRCEXT)
	@echo "DEP $@"
	@$(CPP) -MM -MT \
		'$@ $(addprefix $(LIBOBJDIR)/,\
		$(subst $(SRCEXT),$(OBJEXT),$(notdir $<)))' \
		$(LIBCFLAGS) $(AFLAGS) $< -o $@

$(DEPENDENCIES): $(DEPDIR)/%$(DEPEXT): $(SRCDIR)/%$(SRCEXT)
	@echo "DEP $@"
	@$(CPP) -MM -MT \
		'$@ $(addprefix $(OBJDIR)/,\
		$(subst $(SRCEXT),$(OBJEXT),$(notdir $<)))' \
		$(CFLAGS) $(AFLAGS) $< -o $@

include $(LIBDEPENDENCIES)

include $(DEPENDENCIES)

$(LIBOBJDIR):
	@echo "creating object directory for $(LIBNAME)"
	@mkdir $(LIBOBJDIR)

$(OBJDIR):
	@echo "creating object directory for $(PNAME)"
	@mkdir $(OBJDIR)

$(LIBOBJECTS):
	@echo "CPP $<"
	@$(CPP) -c $(LIBCFLAGS) $(AFLAGS) $< -o $@

$(OBJECTS):
	@echo "CPP $<"
	@$(CPP) -c $(CFLAGS) $(AFLAGS) $< -o $@

$(LNAME): $(LIBOBJECTS)
	@echo "LD $(LNAME)"
	@$(CPP) $(LIBLIBFLAGS) $(AFLAGS) $(LIBOBJECTS) $(LIBLIBS) -o $(LNAME)

$(ENAME): $(OBJECTS)
	@echo "LD $(ENAME)"
	@$(CPP) $(LIBFLAGS) $(AFLAGS) $(OBJECTS) $(LIBS) -o $(ENAME)

libclean:
	@rm -vf $(LIBDEPENDENCIES) $(LIBOBJECTS) $(LNAME)
	@echo "$(LIBNAME) cleaned"
clean:
	@rm -vf $(LIBDEPENDENCIES) $(LIBOBJECTS) $(LNAME) \
		$(DEPENDENCIES) $(OBJECTS) $(ENAME)
	@echo "$(PNAME) cleaned"
distclean:
	@rm -vf $(ARCHIVE_NC) $(ARCHIVE_GZ) $(ARCHIVE_XZ)
	@echo "distribution archives cleaned"

# If the available tar version is bsd tar, we have to change
# the syntax of the transformation command
ifeq ($(TAR_VERSION),bsdtar)
$(ARCHIVE_NC):
	@rm -rvf $(LIBDEPDIR).tmp $(DEPDIR).tmp
	@mv -v $(LIBDEPDIR) $(LIBDEPDIR).tmp
	@mv -v $(DEPDIR) $(DEPDIR).tmp
	@mkdir -vp $(LIBDEPDIR) $(DEPDIR)
	@echo "creating the non-compressed archive $(ARCHIVE_NC)"
	@tar -s '|^|$(APREFIX)/|' -cvf '$(ARCHIVE_NC)' \
		$(LIBHEADERS) $(LIBSOURCES) $(LIBDEPDIR) $(LIBOTHERFILES) \
		$(HEADERS) $(SOURCES) $(DEPDIR) $(OTHERFILES)
	@rmdir $(LIBDEPDIR) $(DEPDIR)
	@mv -v $(LIBDEPDIR).tmp $(LIBDEPDIR)
	@mv -v $(DEPDIR).tmp $(DEPDIR)
else
$(ARCHIVE_NC):
	@rm -rvf $(LIBDEPDIR).tmp $(DEPDIR).tmp
	@mv -v $(LIBDEPDIR) $(LIBDEPDIR).tmp
	@mv -v $(DEPDIR) $(DEPDIR).tmp
	@mkdir -vp $(LIBDEPDIR) $(DEPDIR)
	@echo "creating the non-compressed archive $(ARCHIVE_NC)"
	@tar --transform 's|^|$(APREFIX)/|' -cvf '$(ARCHIVE_NC)' \
		$(LIBHEADERS) $(LIBSOURCES) $(LIBDEPDIR) $(LIBOTHERFILES) \
		$(HEADERS) $(SOURCES) $(DEPDIR) $(OTHERFILES)
	@rmdir $(LIBDEPDIR) $(DEPDIR)
	@mv -v $(LIBDEPDIR).tmp $(LIBDEPDIR)
	@mv -v $(DEPDIR).tmp $(DEPDIR)
endif

$(ARCHIVE_GZ): $(ARCHIVE_NC)
	@echo "compressing the archive"
	@gzip -v '$(ARCHIVE_NC)'

$(ARCHIVE_XZ): $(ARCHIVE_NC)
	@echo "compressing the archive"
	@xz -v '$(ARCHIVE_NC)'

distgz: $(ARCHIVE_GZ)
	@echo "archive $(ARCHIVE_GZ) created"

distxz: $(ARCHIVE_XZ)
	@echo "archive $(ARCHIVE_XZ) created"

# If we do not have the xz compression utility,
# we would like to use the gzip instead
ifeq ($(XZ_UNAVAILABLE),COMMAND_UNAVAILABLE)
dist: distgz
else
dist: distxz
endif
